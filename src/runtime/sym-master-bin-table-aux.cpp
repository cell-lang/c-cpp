#include "lib.h"


uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *table, uint32 last_idx);
void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *table, uint32 next_free);

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool);
bool master_bin_table_lock_surr(MASTER_BIN_TABLE *table, uint32 surr);

////////////////////////////////////////////////////////////////////////////////

inline void sort_args(uint32 &arg1, uint32 &arg2) {
  if (arg1 > arg2) {
    uint32 tmp = arg1;
    arg1 = arg2;
    arg2 = tmp;
  }
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_init(SYM_MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->insertions);
  queue_u32_init(&table_aux->reinsertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void sym_master_bin_table_aux_reset(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->reinsertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_clear(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void sym_master_bin_table_aux_delete(SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void sym_master_bin_table_aux_delete_1(SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

uint32 sym_master_bin_table_aux_insert(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);
  uint32 surr = sym_master_bin_table_lookup_surrogate(table, arg1, arg2);

  if (surr != 0xFFFFFFFF) {
    queue_3u32_insert(&table_aux->reinsertions, arg1, arg2, surr);
    return surr;
  }

  uint32 count_times_3 = table_aux->insertions.count;
  if (count_times_3 > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL
    uint32 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count_times_3 ; i += 3) {
      uint32 curr_arg1 = array[i];
      uint32 curr_arg2 = array[i + 1];
      if (curr_arg1 == arg1 & curr_arg2 == arg2) {
        surr = array[i + 2];
        break;
      }
    }
  }

  if (surr == 0xFFFFFFFF)
    surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);

  table_aux->last_surr = surr;
  queue_3u32_insert(&table_aux->insertions, arg1, arg2, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 sym_master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);
  uint32 surr = sym_master_bin_table_lookup_surrogate(table, arg1, arg2);

  if (surr != 0xFFFFFFFF)
    return surr;

  //## BAD BAD BAD: IMPLEMENT FOR REAL
  uint32 count_times_3 = table_aux->insertions.count;
  uint32 *array = table_aux->insertions.array;
  for (uint32 i=0 ; i < count_times_3 ; i += 3) {
    uint32 curr_arg1 = array[i];
    uint32 curr_arg2 = array[i + 1];
    if (curr_arg1 == arg1 & curr_arg2 == arg2)
      return array[i + 2];
  }

  //## BUG BUG BUG: NOT SURE THIS CANNOT HAPPEN
  //## TRY TO TRICK THE CODE ALL THE WAY HERE
  internal_fail();
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_apply(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  uint32 reins_count_times_3 = table_aux->reinsertions.count;
  uint32 ins_count_times_3 = table_aux->insertions.count;

  uint32 locked_surrs_count = 0;
  int32 max_locked_surr = -1;

  // Locking the surrogates for the tuples that are reinserted, so that they aren't added back
  // to the list of free ones. Otherwise we would have to do a linear search to find them.
  if (reins_count_times_3 > 0) {
    uint32 *array = table_aux->reinsertions.array;
    for (uint32 i=2 ; i < reins_count_times_3 ; i += 3) {
      uint32 surr = array[i];
      if (master_bin_table_lock_surr(table, surr)) {
        locked_surrs_count++;
        assert(((uint32)((int32) surr)) == surr);
        if (surr > max_locked_surr)
          max_locked_surr = surr;
      }
    }
  }

  // Removing from the surrogates already reserved for newly inserted tuples
  // from the list of free ones, so we can append to that list while deleting
  if (ins_count_times_3 != 0) {
    assert(table_aux->last_surr != 0xFFFFFFFF);
    uint32 next_surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
    master_bin_table_set_next_free_surr(table, next_surr);
  }

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

  if (reins_count_times_3 > 0) {
    uint32 *array = table_aux->reinsertions.array;
    for (uint32 i=0 ; i < ins_count_times_3 ; ) {
      uint32 arg1 = array[i++];
      uint32 arg2 = array[i++];
      uint32 surr = array[i++];
      if (master_bin_table_insert_with_surr(table, arg1, arg2, surr, mem_pool)) {
        incr_rc(store, arg1);
        incr_rc(store, arg2);
      }
    }
  }

  if (ins_count_times_3 > 0) {
    uint32 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < ins_count_times_3 ; ) {
      uint32 arg1 = array[i++];
      uint32 arg2 = array[i++];
      uint32 surr = array[i++];
      if (master_bin_table_insert_with_surr(table, arg1, arg2, surr, mem_pool)) {
        incr_rc(store, arg1);
        incr_rc(store, arg2);
      }
    }
  }
}
