#include "lib.h"


void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_prepare(QUEUE_U32 *queue);
void queue_u32_reset(QUEUE_U32 *queue);

////////////////////////////////////////////////////////////////////////////////

void unary_table_aux_init(UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u32_init(&table_aux->deletions);
  queue_u32_init(&table_aux->insertions);
#ifndef NDEBUG
  table_aux->init_capacity = 0xFFFFFFFF;
#endif
  table_aux->clear = false;
}

uint32 unary_table_aux_insert(UNARY_TABLE_AUX *table_aux, uint32 elt) {
  queue_u32_insert(&table_aux->insertions, elt);
}

void unary_table_aux_delete(UNARY_TABLE_AUX *table_aux, uint32 elt) {
  queue_u32_insert(&table_aux->deletions, elt);
}

void unary_table_aux_clear(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  table_aux->init_capacity = table->capacity;
  table_aux->clear = true;
}

void unary_table_aux_apply(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    UNARY_TABLE_ITER iter;
    unary_table_iter_init(table, &iter);
    while (!unary_table_iter_is_out_of_range(&iter)) {
      uint32 surr = unary_table_iter_get(&iter);
      decr_rc(store, store_aux, surr);
    }
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 elt = array[i];
        if (unary_table_delete(table, elt))
          decr_rc(store, store_aux, elt);
      }
    }
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 elt = array[i];
      if (unary_table_insert(table, elt, mem_pool))
        incr_rc(store, elt);
    }
  }
}

void unary_table_aux_reset(UNARY_TABLE_AUX *table_aux) {
  queue_u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->deletions);
#ifndef NDEBUG
  table_aux->init_capacity = 0xFFFFFFFF;
#endif
  table_aux->clear = false;
}
