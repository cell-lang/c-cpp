#include "lib.h"


uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *table, uint32 last_idx);
void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *table, uint32 next_free);

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool);

bool master_bin_table_lock_surr(MASTER_BIN_TABLE *table, uint32 surr);
bool master_bin_table_unlock_surr(MASTER_BIN_TABLE *table, uint32 surr);
bool master_bin_table_slot_is_locked(MASTER_BIN_TABLE *table, uint32 surr);

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

void master_bin_table_aux_init(MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_u32_init(&table_aux->reinsertions);
  queue_u32_init(&table_aux->insertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_u32_reset(&table_aux->reinsertions);
  queue_u32_reset(&table_aux->insertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_clear(MASTER_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void master_bin_table_aux_delete(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void master_bin_table_aux_delete_1(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void master_bin_table_aux_delete_2(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  queue_u32_insert(&table_aux->deletions_2, arg2);
}

uint32 master_bin_table_aux_insert(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr = master_bin_table_lookup_surrogate(table, arg1, arg2);

  if (surr != 0xFFFFFFFF) {
    queue_u32_insert(&table_aux->reinsertions, surr);
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

uint32 master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr = master_bin_table_lookup_surrogate(table, arg1, arg2);
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
  //## ACTUALLY THIS CAN HAPPEN, IF slave_tern_table_aux_insert(..) IS CALLED WITHOUT A CORRESPONDING TUPLE IN THE MASTER TABLE
  //## IMPLEMENT IMPLEMENT IMPLEMENT
  //## MAYBE IT WOULD BE BETTER TO KEEP THIS METHOD AS IS AND IMPLEMENT A SEPARATE master_bin_table_aux_lookup_surr_forced(..)
  internal_fail();
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_apply(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, STATE_MEM_POOL *mem_pool) {
  // Removing from the surrogates already reserved for newly inserted tuples
  // from the list of free ones, so we can append to that list while deleting
  uint32 ins_count = table_aux->insertions.count / 3;
  if (ins_count != 0) {
    assert(table_aux->last_surr != 0xFFFFFFFF);
    uint32 next_surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
    master_bin_table_set_next_free_surr(table, next_surr);
  }

  // Locking the surrogates for the tuples that are reinserted, so that they aren't added back
  // to the list of free ones. Otherwise we would have to do a linear search to find them.
  uint32 reins_count = table_aux->reinsertions.count;
  if (reins_count > 0) {
    //## THIS IS PROBABLY TOTALLY USELESS. REMOVE AFTER TESTING
    for (uint32 i=0 ; i < reins_count ; i++) {
      uint32 surr = table_aux->reinsertions.array[i];
      assert(((uint32)((int32) surr)) == surr);
    }

    if (table_aux->clear) {
      //## FOR MAXIMUM EFFICIENCY, THIS SHOULD BE HANDLED DIFFERENTLY

      // Locking all tuples that are supposed to be reinserted, so as to avoid deleting them in the first place
      uint32 *surrs = table_aux->reinsertions.array;
      for (uint32 i=0 ; i < reins_count ; i++)
        master_bin_table_lock_surr(table, surrs[i]);

      MASTER_BIN_TABLE_ITER iter;
      master_bin_table_iter_init(table, &iter);
      while (!master_bin_table_iter_is_out_of_range(&iter)) {
        uint32 surr = master_bin_table_iter_get_surr(&iter);
        if (!master_bin_table_slot_is_locked(table, surr)) {
          uint32 arg1 = master_bin_table_iter_get_1(&iter);
          uint32 arg2 = master_bin_table_iter_get_2(&iter);
          master_bin_table_delete(table, arg1, arg2);
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
        }
        master_bin_table_iter_move_forward(&iter);
      }

      // Unlocking all reinserted tuples
      for (uint32 i=0 ; i < reins_count ; i++)
        master_bin_table_unlock_surr(table, surrs[i]);
    }
    else {
      uint32 dels_count = table_aux->deletions.count;
      uint32 dels_1_count = table_aux->deletions_1.count;
      uint32 dels_2_count = table_aux->deletions_2.count;

      if (dels_count != 0 | dels_1_count != 0 | dels_2_count != 0) {
        // Locking all tuples that are supposed to be reinserted, so as to avoid deleting them in the first place
        uint32 *surrs = table_aux->reinsertions.array;
        for (uint32 i=0 ; i < reins_count ; i++)
          master_bin_table_lock_surr(table, surrs[i]);

        if (dels_count > 0) {
          uint64 *array = table_aux->deletions.array;
          for (uint32 i=0 ; i < dels_count ; i++) {
            uint64 args = array[i];
            uint32 arg1 = unpack_arg1(args);
            uint32 arg2 = unpack_arg2(args);
            uint32 surr = master_bin_table_lookup_surrogate(table, arg1, arg2);
            if (surr != 0xFFFFFFFF && !master_bin_table_slot_is_locked(table, surr)) {
              bool found = master_bin_table_delete(table, arg1, arg2);
              assert(found);
              decr_rc_1(store_1, store_aux_1, arg1);
              decr_rc_2(store_2, store_aux_2, arg2);
            }
          }
        }

        if (dels_1_count > 0) {
          uint32 *array = table_aux->deletions_1.array;
          for (uint32 i=0 ; i < dels_1_count ; i++) {
            uint32 arg1 = array[i];
            MASTER_BIN_TABLE_ITER_1 iter;
            master_bin_table_iter_1_init(table, &iter, arg1);
            while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
              uint32 surr = master_bin_table_iter_1_get_surr(&iter);
              if (!master_bin_table_slot_is_locked(table, surr)) {
                uint32 arg2 = master_bin_table_iter_1_get_1(&iter);
                bool found = master_bin_table_delete(table, arg1, arg2);
                assert(found);
                decr_rc_1(store_1, store_aux_1, arg1);
                decr_rc_2(store_2, store_aux_2, arg2);
              }
              master_bin_table_iter_1_move_forward(&iter);
            }
          }
        }

        if (dels_2_count > 0) {
          uint32 *array = table_aux->deletions_2.array;
          for (uint32 i=0 ; i < dels_2_count ; i++) {
            uint32 arg2 = array[i];
            MASTER_BIN_TABLE_ITER_2 iter;
            master_bin_table_iter_2_init(table, &iter, arg2);
            while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
              uint32 surr = master_bin_table_iter_2_get_surr(&iter);
              if (!master_bin_table_slot_is_locked(table, surr)) {
                uint32 arg1 = master_bin_table_iter_2_get_1(&iter);
                bool found = master_bin_table_delete(table, arg1, arg2);
                assert(found);
                decr_rc_1(store_1, store_aux_1, arg1);
                decr_rc_2(store_2, store_aux_2, arg2);
              }
              master_bin_table_iter_2_move_forward(&iter);
            }
          }
        }

        // Unlocking all reinserted tuples
        for (uint32 i=0 ; i < reins_count ; i++)
          master_bin_table_unlock_surr(table, surrs[i]);
      }
    }
  }
  else {
    if (table_aux->clear) {
      MASTER_BIN_TABLE_ITER iter;
      master_bin_table_iter_init(table, &iter);
      while (!master_bin_table_iter_is_out_of_range(&iter)) {
        uint32 arg1 = master_bin_table_iter_get_1(&iter);
        uint32 arg2 = master_bin_table_iter_get_2(&iter);
        decr_rc_1(store_1, store_aux_1, arg1);
        decr_rc_2(store_2, store_aux_2, arg2);
        master_bin_table_iter_move_forward(&iter);
      }
      master_bin_table_clear(table, mem_pool);
    }
    else {
      uint32 count = table_aux->deletions.count;
      if (count > 0) {
        uint64 *array = table_aux->deletions.array;
        for (uint32 i=0 ; i < count ; i++) {
          uint64 args = array[i];
          uint32 arg1 = unpack_arg1(args);
          uint32 arg2 = unpack_arg2(args);
          if (master_bin_table_delete(table, arg1, arg2)) {
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
          }
        }
      }

      count = table_aux->deletions_1.count;
      if (count > 0) {
        uint32 *array = table_aux->deletions_1.array;
        for (uint32 i=0 ; i < count ; i++) {
          uint32 arg1 = array[i];
          MASTER_BIN_TABLE_ITER_1 iter;
          master_bin_table_iter_1_init(table, &iter, arg1);
          while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
            uint32 arg2 = master_bin_table_iter_1_get_1(&iter);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
            master_bin_table_iter_1_move_forward(&iter);
          }
          master_bin_table_delete_1(table, arg1);
        }
      }

      count = table_aux->deletions_2.count;
      if (count > 0) {
        uint32 *array = table_aux->deletions_2.array;
        for (uint32 i=0 ; i < count ; i++) {
          uint32 arg2 = array[i];
          MASTER_BIN_TABLE_ITER_2 iter;
          master_bin_table_iter_2_init(table, &iter, arg2);
          while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
            uint32 arg1 = master_bin_table_iter_2_get_1(&iter);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
            master_bin_table_iter_2_move_forward(&iter);
          }
          master_bin_table_delete_2(table, arg2);
        }
      }
    }
  }

  if (ins_count > 0) {
    uint32 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < ins_count ; i++) {
      uint32 idx = 3 * i;
      uint32 arg1 = array[idx];
      uint32 arg2 = array[idx + 1];
      uint32 surr = array[idx + 2];
      if (master_bin_table_insert_with_surr(table, arg1, arg2, surr, mem_pool)) {
        incr_rc_1(store_1, arg1);
        incr_rc_2(store_2, arg2);
      }
    }
  }
}
