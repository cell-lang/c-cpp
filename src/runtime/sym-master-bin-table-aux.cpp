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
  queue_3u32_init(&table_aux->insertions);
  queue_u32_init(&table_aux->reinsertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void sym_master_bin_table_aux_reset(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_3u32_reset(&table_aux->insertions);
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
  uint32 surr = sym_master_bin_table_lookup_surr(table, arg1, arg2);

  if (surr != 0xFFFFFFFF) {
    queue_u32_insert(&table_aux->reinsertions, surr);
    return surr;
  }

  uint32 count = table_aux->insertions.count_;
  if (count > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 curr_arg1 = (*ptr)[0];
      uint32 curr_arg2 = (*ptr)[1];
      if (curr_arg1 == arg1 & curr_arg2 == arg2) {
        surr = (*ptr)[2];
        break;
      }
      ptr++;
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
  uint32 surr = sym_master_bin_table_lookup_surr(table, arg1, arg2);

  if (surr != 0xFFFFFFFF)
    return surr;

  //## BAD BAD BAD: IMPLEMENT FOR REAL
  uint32 count = table_aux->insertions.count_;
  uint32 (*ptr)[3] = table_aux->insertions.array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 curr_arg1 = (*ptr)[0];
    uint32 curr_arg2 = (*ptr)[1];
    if (curr_arg1 == arg1 & curr_arg2 == arg2)
      return (*ptr)[2];
    ptr++;
  }

  //## BUG BUG BUG: NOT SURE THIS CANNOT HAPPEN
  //## TRY TO TRICK THE CODE ALL THE WAY HERE
  internal_fail();
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_apply(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  //## I'M ASSUMING THAT FOREIGN KEY CHECKS WILL BE ENOUGH TO AVOID THE
  //## UNUSED SURROGATE ALLOCATIONS, BUT I'M NOT SURE
  assert(table_aux->reserved_surrs.empty());

  // Removing from the surrogates already reserved for newly inserted tuples
  // from the list of free ones, so we can append to that list while deleting
  uint32 ins_count = table_aux->insertions.count_;
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

      uint32 count = master_bin_table_size(table);
      uint32 read = 0;
      uint64 *slots = table->slots;
      for (uint32 surr=0 ; read < count ; surr++) {
        uint64 slot = slots[surr];
        if (!master_bin_table_slot_is_empty(slot)) {
          read++;
          if (!is_locked(slot)) {
            uint32 arg1 = unpack_arg1(slot);
            uint32 arg2 = unpack_arg2(slot);

            master_bin_table_delete(table, arg1, arg2);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);

          }
        }
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
            uint32 surr = master_bin_table_lookup_possibly_locked_surr(table, arg1, arg2);
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

          uint32 max_count_1 = 0;
          for (uint32 i=0 ; i < dels_1_count ; i++) {
            uint32 arg1 = array[i];
            uint32 count1 = master_bin_table_count_1(table, arg1);
            if (count1 > max_count_1)
              max_count_1 = count1;
          }

          if (max_count_1 > 0) {
            uint32 buffer[512];
            uint32 *arg2s = max_count_1 <= 256 ? buffer : new_uint32_array(2 * max_count_1);
            uint32 *surrs = arg2s + max_count_1;

            for (uint32 i=0 ; i < dels_1_count ; i++) {
              uint32 arg1 = array[i];
              uint32 count1 = master_bin_table_restrict_1(table, arg1, arg2s, surrs);
              for (uint32 j=0 ; j < count1 ; j++) {
                uint32 surr = surrs[j];
                if (!master_bin_table_slot_is_locked(table, surr)) {
                  uint32 arg2 = arg2s[j];

                  bool found = master_bin_table_delete(table, arg1, arg2);
                  assert(found);
                  decr_rc_1(store_1, store_aux_1, arg1);
                  decr_rc_2(store_2, store_aux_2, arg2);

                }
              }
            }
          }
        }

        if (dels_2_count > 0) {
          uint32 *array = table_aux->deletions_2.array;

          uint32 max_count_2 = 0;
          for (uint32 i=0 ; i < dels_2_count ; i++) {
            uint32 arg2 = array[i];
            uint32 count2 = master_bin_table_count_2(table, arg2);
            if (count2 > max_count_2)
              max_count_2 = count2;
          }

          if (max_count_2 > 0) {
            uint32 buffer[256];
            uint32 *arg1s = max_count_2 <= 256 ? buffer : new_uint32_array(max_count_2);

            for (uint32 i=0 ; i < dels_2_count ; i++) {
              uint32 arg2 = array[i];
              uint32 count2 = master_bin_table_restrict_2(table, arg2, arg1s);
              for (uint32 j=0 ; j < count2 ; j++) {
                uint32 arg1 = arg1s[j];
                uint32 surr = master_bin_table_lookup_possibly_locked_surr(table, arg1, arg2);
                if (!master_bin_table_slot_is_locked(table, surr)) {

                  bool found = master_bin_table_delete(table, arg1, arg2);
                  assert(found);
                  decr_rc_1(store_1, store_aux_1, arg1);
                  decr_rc_2(store_2, store_aux_2, arg2);

                }
              }
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
      uint32 count = master_bin_table_size(table);
      uint32 read = 0;
      uint64 *slots = table->slots;
      for (uint32 surr=0 ; read < count ; surr++) {
        uint64 slot = slots[surr];
        if (!master_bin_table_slot_is_empty(slot)) {
          read++;
          uint32 arg1 = unpack_arg1(slot);
          uint32 arg2 = unpack_arg2(slot);

          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);

        }
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

          uint32 count1 = master_bin_table_count_1(table, arg1);
          uint32 read = 0;
          while (read < count1) {
            uint32 buffer[64];
            UINT32_ARRAY array1 = master_bin_table_range_restrict_1(table, arg1, read, buffer, 64);
            read += array1.size;
            for (uint32 i=0 ; i < array1.size ; i++) {
              uint32 arg2 = array1.array[i];

              decr_rc_1(store_1, store_aux_1, arg1);
              decr_rc_2(store_2, store_aux_2, arg2);

            }
          }
          master_bin_table_delete_1(table, arg1);
        }
      }

      count = table_aux->deletions_2.count;
      if (count > 0) {
        uint32 *array = table_aux->deletions_2.array;
        for (uint32 i=0 ; i < count ; i++) {
          uint32 arg2 = array[i];

          uint32 count2 = master_bin_table_count_2(table, arg2);
          uint32 read = 0;
          while (read < count2) {
            uint32 buffer[64];
            UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
            read += array2.size;
            for (uint32 i=0 ; i < array2.size ; i++) {
              uint32 arg1 = array2.array[i];

              decr_rc_1(store_1, store_aux_1, arg1);
              decr_rc_2(store_2, store_aux_2, arg2);

            }
          }
          master_bin_table_delete_2(table, arg2);
        }
      }
    }
  }

  if (ins_count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < ins_count ; i++) {
      uint32 arg1 = (*ptr)[0];
      uint32 arg2 = (*ptr)[1];
      uint32 surr = (*ptr)[2];
      if (master_bin_table_insert_with_surr(table, arg1, arg2, surr, mem_pool)) {
        incr_rc_1(store_1, arg1);
        incr_rc_2(store_2, arg2);
      }
      ptr++;
    }
  }
}
