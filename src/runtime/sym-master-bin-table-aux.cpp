#include "lib.h"

void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_prepare(QUEUE_U32 *queue);
void queue_u32_reset(QUEUE_U32 *queue);
bool queue_u32_contains(QUEUE_U32 *queue, uint32 value);

////////////////////////////////////////////////////////////////////////////////

void queue_u64_init(QUEUE_U64 *);
void queue_u64_insert(QUEUE_U64 *, uint64);
void queue_u64_prepare(QUEUE_U64 *);
void queue_u64_flip_words(QUEUE_U64 *);
void queue_u64_reset(QUEUE_U64 *);
bool queue_u64_contains(QUEUE_U64 *, uint64);

////////////////////////////////////////////////////////////////////////////////

inline uint32 unpack_arg1(uint64 args) {
  return (uint32) (args >> 32);
}

inline uint32 unpack_arg2(uint64 args) {
  return (uint32) args;
}

inline uint64 pack_args(uint32 arg1, uint32 arg2) {
  uint64 args = (((uint64) arg1) << 32) | arg2;
  assert(unpack_arg1(args) == arg1);
  assert(unpack_arg2(args) == arg2);
  return args;
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_init(SYM_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

void sym_master_bin_table_aux_reset(SYM_BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u64_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_clear(SYM_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void sym_master_bin_table_aux_delete(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void sym_master_bin_table_aux_delete_1(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void sym_master_bin_table_aux_insert(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->insertions, pack_args(arg1, arg2));

}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_apply(MASTER_BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    MASTER_BIN_TABLE_ITER iter;
    sym_master_bin_table_iter_init(table, &iter);
    while (!sym_master_bin_table_iter_is_out_of_range(&iter)) {
      uint32 arg1 = sym_master_bin_table_iter_get_1(&iter);
      uint32 arg2 = sym_master_bin_table_iter_get_2(&iter);
      decr_rc(store, store_aux, arg1);
      decr_rc(store, store_aux, arg2);
      sym_master_bin_table_iter_move_forward(&iter);
    }
    sym_master_bin_table_clear(table, mem_pool);
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (sym_master_bin_table_delete(table, arg1, arg2)) {
          decr_rc(store, store_aux, arg1);
          decr_rc(store, store_aux, arg2);
        }
      }
    }

    count = table_aux->deletions_1.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg1 = array[i];
        SYM_MASTER_BIN_TABLE_ITER_1 iter;
        sym_master_bin_table_iter_1_init(table, &iter, arg1);
        while (!sym_master_bin_table_iter_1_is_out_of_range(&iter)) {
          uint32 arg2 = sym_master_bin_table_iter_1_get_1(&iter);
          decr_rc(store, store_aux, arg1);
          decr_rc(store, store_aux, arg2);
          sym_master_bin_table_iter_1_move_forward(&iter);
        }
        sym_master_bin_table_delete_1(table, arg1);
      }
    }
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      if (sym_master_bin_table_insert(table, arg1, arg2, mem_pool)) {
        incr_rc(store, arg1);
        incr_rc(store, arg2);
      }
    }
  }
}
