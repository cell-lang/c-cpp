#include "lib.h"


void unary_table_init(UNARY_TABLE *table, STATE_MEM_POOL *) {

}

void unary_table_init_aux(UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  table_aux->clear = false;
}

bool unary_table_contains(UNARY_TABLE *table, uint32 value) {
  return table->elements.count(value) > 0;
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

void unary_table_apply(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  if (table_aux->clear) {
    table->elements.clear();
  }
  else {
    for (unordered_set<uint32>::iterator it = table_aux->deletions.begin() ; it != table_aux->deletions.end() ; it++)
      table->elements.erase(*it);
  }

  for (unordered_set<uint32>::iterator it = table_aux->insertions.begin() ; it != table_aux->insertions.end() ; it++)
    table->elements.insert(*it);
}

void unary_table_reset(UNARY_TABLE_AUX *table_aux) {
  table_aux->insertions.clear();
  table_aux->deletions.clear();
  table_aux->clear = false;
}

OBJ unary_table_copy_to(UNARY_TABLE *table, OBJ_STORE *store, STREAM *stream) {
  for (unordered_set<uint32>::iterator it = table->elements.begin() ; it != table->elements.end() ; it++) {
    uint32 surr = *it;
    OBJ value = surr_to_value(store, surr);
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
