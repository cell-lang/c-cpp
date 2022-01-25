#include "lib.h"


void unary_table_init(UNARY_TABLE *table, STATE_MEM_POOL *) {

}

void unary_table_aux_init(UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  table_aux->clear = false;
}

bool unary_table_contains(UNARY_TABLE *table, uint32 value) {
  return table->elements.count(value) > 0;
}

uint64 unary_table_size(UNARY_TABLE *table) {
  return table->elements.size();
}

uint32 unary_table_insert(UNARY_TABLE *table, STATE_MEM_POOL *, uint32 value) {
  table->elements.insert(value);
}

// void unary_table_delete(UNARY_TABLE *table, uint32 value) {
//   table->elements.erase(value);
// }

// void unary_table_clear(UNARY_TABLE *table) {
//   table->elements.clear()
// }

uint32 unary_table_queue_insert(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, uint32 value) {
  table_aux->insertions.insert(value);
}

void unary_table_queue_delete(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, uint32 value) {
  table_aux->deletions.insert(value);
}

void unary_table_queue_clear(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void unary_table_apply(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *) {
  if (table_aux->clear) {
    for (unordered_set<uint32>::iterator it = table->elements.begin() ; it != table->elements.end() ; it++)
      decr_rc(store, store_aux, *it);
    table->elements.clear();
  }
  else {
    for (unordered_set<uint32>::iterator it = table_aux->deletions.begin() ; it != table_aux->deletions.end() ; it++) {
      uint32 surr = *it;
      table->elements.erase(surr);
      decr_rc(store, store_aux, surr);
    }
  }

  for (unordered_set<uint32>::iterator it = table_aux->insertions.begin() ; it != table_aux->insertions.end() ; it++) {
    uint32 surr = *it;
    if (table->elements.count(surr) > 0) {
      table->elements.insert(surr);
      incr_rc(store, surr);
    }
  }
}

void unary_table_reset_aux(UNARY_TABLE_AUX *table_aux) {
  table_aux->insertions.clear();
  table_aux->deletions.clear();
  table_aux->clear = false;
}

OBJ unary_table_copy_to(UNARY_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *stream) {
  for (unordered_set<uint32>::iterator it = table->elements.begin() ; it != table->elements.end() ; it++) {
    uint32 surr = *it;
    OBJ value = surr_to_obj(store, surr);
    append(*stream, value);
  }
}

void unary_table_iter_init(UNARY_TABLE *table, UNARY_TABLE_ITER *iter) {
  iter->it = table->elements.begin();
  iter->end = table->elements.end();
}

void unary_table_iter_move_forward(UNARY_TABLE_ITER *iter) {
  assert(iter->it != iter->end);
  iter->it++;
}

bool unary_table_iter_is_out_of_range(UNARY_TABLE_ITER *iter) {
  return iter->it == iter->end;
}

uint32 unary_table_iter_get(UNARY_TABLE_ITER *iter) {
  return *iter->it;
}

void unary_table_write(WRITE_FILE_STATE *write_state, UNARY_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store) {
  UNARY_TABLE_ITER iter;
  unary_table_iter_init(table, &iter);
  uint32 count = unary_table_size(table);
  uint32 idx = 0;
  while (!unary_table_iter_is_out_of_range(&iter)) {
    uint32 surr = unary_table_iter_get(&iter);
    OBJ obj = surr_to_obj(store, surr);
    write_str(write_state, "\n    ");
    write_obj(write_state, obj);
    if (++idx != count)
      write_str(write_state, ",");
    unary_table_iter_move_forward(&iter);
  }
}
