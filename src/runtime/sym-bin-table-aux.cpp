#include "lib.h"


inline uint64 pack_sym_args(uint32 arg1, uint32 arg2) {
  return arg1 <= arg2 ? pack(arg1, arg2) : pack(arg2, arg1);
}

////////////////////////////////////////////////////////////////////////////////

void sym_bin_table_aux_init(SYM_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

void sym_bin_table_aux_reset(SYM_BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u64_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void sym_bin_table_aux_insert(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->insertions, pack_sym_args(arg1, arg2));
}

void sym_bin_table_aux_delete(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_sym_args(arg1, arg2));
}

void sym_bin_table_aux_delete_1(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void sym_bin_table_aux_clear(SYM_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void sym_bin_table_aux_apply(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    uint32 count = bin_table_size(table);
    uint32 read = 0;
    for (uint32 arg1=0 ; read < count ; arg1++) {
      uint32 count_1 = bin_table_count_1(table, arg1);
      if (count_1 > 0) {
        read += count_1;
        uint32 read_1 = 0;
        do {
          uint32 buffer[64];
          UINT32_ARRAY array = bin_table_range_restrict_1(table, arg1, read_1, buffer, 64);
          read_1 += array.size;
          for (uint32 i=0 ; i < array.size ; i++) {
            uint32 arg2 = array.array[i];

            decr_rc(store, store_aux, arg1);
            decr_rc(store, store_aux, arg2);

          }
        } while (read_1 < count_1);
      }
    }
    sym_bin_table_clear(table, mem_pool);
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = get_low_32(args);
        uint32 arg2 = get_high_32(args);
        assert(arg1 <= arg2);
        if (sym_bin_table_delete(table, arg1, arg2)) {
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

        uint32 count1 = bin_table_count_1(table, arg1);
        uint32 read1 = 0;
        while (read1 < count1) {
          uint32 buffer[64];
          UINT32_ARRAY array1 = bin_table_range_restrict_1(table, arg1, read1, buffer, 64);
          read1 += array1.size;
          for (uint32 i1=0 ; i1 < array1.size ; i1++) {
            uint32 arg2 = array1.array[i1];

            decr_rc(store, store_aux, arg1);
            decr_rc(store, store_aux, arg2);

          }
        }
        sym_bin_table_delete_1(table, arg1);
      }
    }
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = get_low_32(args);
      uint32 arg2 = get_high_32(args);
      assert(arg1 <= arg2);
      if (sym_bin_table_insert(table, arg1, arg2, mem_pool)) {
        incr_rc(store, arg1);
        incr_rc(store, arg2);
      }
    }
  }
}
