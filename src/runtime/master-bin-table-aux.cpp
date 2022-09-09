#include "lib.h"


bool is_locked(uint64 slot);

uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *, uint32 last_idx);
void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *, uint32 next_free);

bool master_bin_table_lock_surr(MASTER_BIN_TABLE *, uint32 surr);
bool master_bin_table_unlock_surr(MASTER_BIN_TABLE *, uint32 surr);

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *);

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_init(MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  table_aux->mem_pool = mem_pool;
  col_update_bit_map_init(&table_aux->bit_map);
  col_update_bit_map_init(&table_aux->another_bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_3u32_init(&table_aux->insertions);
  queue_u32_init(&table_aux->locked_surrs);
  trns_map_surr_surr_surr_init(&table_aux->reserved_surrs);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_3u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->locked_surrs);
  trns_map_surr_surr_surr_clear(&table_aux->reserved_surrs);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_partial_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));
  assert(table_aux->deletions.count == 0);
  assert(table_aux->deletions_1.count == 0);
  assert(table_aux->deletions_2.count == 0);
  assert(table_aux->insertions.count == 0);
  assert(table_aux->locked_surrs.count == 0); //## NOT AT ALL SURE ABOUT THIS ONE, RESETTING IT ANYWAY DOWN BELOW
  assert(!table_aux->clear);

  queue_u32_reset(&table_aux->locked_surrs); //## MAYBE IT'S NOT NECESSARY? IT'S HERE JUST IN CASE.
  trns_map_surr_surr_surr_clear(&table_aux->reserved_surrs);
  table_aux->last_surr = 0xFFFFFFFF;
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_clear(MASTER_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void master_bin_table_aux_delete(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (master_bin_table_contains(table, arg1, arg2))
    queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void master_bin_table_aux_delete_1(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void master_bin_table_aux_delete_2(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  queue_u32_insert(&table_aux->deletions_2, arg2);
}

uint32 master_bin_table_aux_insert(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
  if (surr != 0xFFFFFFFF) {
    queue_u32_insert(&table_aux->locked_surrs, surr);
    return surr;
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 curr_arg1 = (*ptr)[0];
      uint32 curr_arg2 = (*ptr)[1];
      if (curr_arg1 == arg1 & curr_arg2 == arg2) {
        surr = (*ptr)[2];
        return surr;
      }
      ptr++;
    }
  }

  surr = trns_map_surr_surr_surr_extract(&table_aux->reserved_surrs, arg1, arg2);
  if (surr == 0xFFFFFFFF) {
    surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
    table_aux->last_surr = surr;
  }

  queue_3u32_insert(&table_aux->insertions, arg1, arg2, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
  if (surr != 0xFFFFFFFF)
    return surr;

  //## BAD BAD BAD: IMPLEMENT FOR REAL
  uint32 count = table_aux->insertions.count;
  uint32 (*ptr)[3] = table_aux->insertions.array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 curr_arg1 = (*ptr)[0];
    uint32 curr_arg2 = (*ptr)[1];
    if (curr_arg1 == arg1 & curr_arg2 == arg2)
      return (*ptr)[2];
    ptr++;
  }

  surr = trns_map_surr_surr_surr_lookup(&table_aux->reserved_surrs, arg1, arg2);
  if (surr != 0xFFFFFFFF)
    return surr;

  surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
  table_aux->last_surr = surr;
  trns_map_surr_surr_surr_insert_new(&table_aux->reserved_surrs, arg1, arg2, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_aux_lock_surrs(MASTER_BIN_TABLE *table, QUEUE_U32 *locked_surrs) {
  assert(locked_surrs->count != 0);

  uint32 count = locked_surrs->count;
  uint32 highest_surr = 0; // There's at least one surrogate, so zero as a default works
  uint32 *array = locked_surrs->array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 surr = array[i];
    master_bin_table_lock_surr(table, surr);
    if (surr > highest_surr)
      highest_surr = surr;
  }
  return highest_surr;
}

void master_bin_table_aux_unlock_surrs(MASTER_BIN_TABLE *table, QUEUE_U32 *locked_surrs) {
  assert(locked_surrs->count != 0);

  uint32 count = locked_surrs->count;
  uint32 *array = locked_surrs->array;
  for (uint32 i=0 ; i < count ; i++)
    master_bin_table_unlock_surr(table, array[i]);
}

static void master_bin_table_aux_build_col_1_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = (*ptr)[0];
      col_update_bit_map_set(bit_map, arg1, mem_pool);
      ptr++;
    }
  }
}

static void master_bin_table_aux_build_col_2_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg2 = (*ptr)[1];
      col_update_bit_map_set(bit_map, arg2, mem_pool);
      ptr++;
    }
  }
}

static void master_bin_table_aux_build_surr_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr = (*ptr)[2];
      col_update_bit_map_set(bit_map, surr, mem_pool);
      ptr++;
    }
  }
}

void master_bin_table_aux_apply_surrs_acquisition(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux) {
  assert(table->table.forward.count == table->table.backward.count);

  //## I'M ASSUMING THAT FOREIGN KEY CHECKS WILL BE ENOUGH TO AVOID THE
  //## UNUSED SURROGATE ALLOCATIONS, BUT I'M NOT SURE
  assert(trns_map_surr_surr_surr_is_empty(&table_aux->reserved_surrs));

  // Removing from the surrogates already reserved for newly inserted tuples
  // from the list of free ones, so we can append to that list while deleting
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
#ifndef NDEBUG
    if (table_aux->last_surr == 0xFFFFFFFF) {
      uint32 (*ptr)[3] = table_aux->insertions.array;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = (*ptr)[0];
        uint32 arg2 = (*ptr)[1];
        uint32 surr = (*ptr)[2];
        assert(master_bin_table_contains(table, arg1, arg2));
        assert(master_bin_table_lookup_surr(table, arg1, arg2) == surr);
        ptr++;
      }
    }
#endif
    uint32 next_surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
    master_bin_table_set_next_free_surr(table, next_surr);
  }

  assert(table->table.forward.count == table->table.backward.count);
}

void master_bin_table_aux_apply_deletions(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, STATE_MEM_POOL *mem_pool) {
  assert(table->table.forward.count == table->table.backward.count);

  //## I'M ASSUMING THAT FOREIGN KEY CHECKS WILL BE ENOUGH TO AVOID THE
  //## UNUSED SURROGATE ALLOCATIONS, BUT I'M NOT SURE
  assert(trns_map_surr_surr_surr_is_empty(&table_aux->reserved_surrs));

  bool locks_applied = table_aux->locked_surrs.count == 0;

  //## FROM HERE ON THIS IS BASICALLY IDENTICAL TO bin_table_aux_apply_deletions(..). The most important difference is the locking

  if (table_aux->clear) {
    uint32 count = master_bin_table_size(table);
    if (count > 0) {
      if (table_aux->insertions.count == 0) {
        if (remove1 != NULL)
          remove1(store1, 0xFFFFFFFF, mem_pool);
        if (remove2 != NULL)
          remove2(store2, 0xFFFFFFFF, mem_pool);
      }
      else {
        if (remove1 != NULL) {
          master_bin_table_aux_build_col_1_insertion_bitmap(table_aux, &table_aux->bit_map, mem_pool);
          uint32 read = 0;
          for (uint32 arg1=0 ; read < count ; arg1++) {
            uint32 count1 = master_bin_table_count_1(table, arg1);
            if (count1 > 0) {
              read += count1;
              if (!col_update_bit_map_is_set(&table_aux->bit_map, arg1))
                remove1(store1, arg1, mem_pool);
            }
          }
          col_update_bit_map_clear(&table_aux->bit_map);
        }

        if (remove2 != NULL) {
          master_bin_table_aux_build_col_2_insertion_bitmap(table_aux, &table_aux->bit_map, mem_pool);
          uint32 read = 0;
          for (uint32 arg2=0 ; read < count ; arg2++) {
            uint32 count2 = master_bin_table_count_2(table, arg2);
            if (count2 > 0) {
              read += count2;
              if (!col_update_bit_map_is_set(&table_aux->bit_map, arg2))
                remove2(store2, arg2, mem_pool);
            }
          }
          col_update_bit_map_clear(&table_aux->bit_map);
        }
      }

      uint32 highest_locked_surr = 0xFFFFFFFF;
      if (!locks_applied) {
        highest_locked_surr = master_bin_table_aux_lock_surrs(table, &table_aux->locked_surrs);
        locks_applied = true;
      }

      master_bin_table_clear(table, highest_locked_surr, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0;
    bool col_1_bit_map_built = !has_insertions;
    bool col_2_bit_map_built = !has_insertions;

    COL_UPDATE_BIT_MAP *col_1_bit_map = &table_aux->bit_map;
    COL_UPDATE_BIT_MAP *col_2_bit_map = remove1 == NULL ? &table_aux->bit_map : &table_aux->another_bit_map;

    uint32 del_count = table_aux->deletions.count;
    uint32 del_1_count = table_aux->deletions_1.count;
    uint32 del_2_count = table_aux->deletions_2.count;

    if (del_count > 0 | del_1_count > 0 | del_2_count > 0) {
      if (!locks_applied) {
        master_bin_table_aux_lock_surrs(table, &table_aux->locked_surrs);
        locks_applied = true;
      }
    }

    if (del_count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < del_count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (master_bin_table_delete(table, arg1, arg2)) {
          if (remove1 != NULL && master_bin_table_count_1(table, arg1) == 0) {
            if (!col_1_bit_map_built) {
              master_bin_table_aux_build_col_1_insertion_bitmap(table_aux, col_1_bit_map, mem_pool);
              col_1_bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(col_1_bit_map, arg1))
              remove1(store1, arg1, mem_pool);
          }
          if (remove2 != NULL && master_bin_table_count_2(table, arg2) == 0) {
            if (!col_2_bit_map_built) {
              master_bin_table_aux_build_col_2_insertion_bitmap(table_aux, col_2_bit_map, mem_pool);
              col_2_bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(col_2_bit_map, arg2))
              remove2(store2, arg2, mem_pool);
          }
        }
      }
    }

    if (del_1_count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < del_1_count ; i++) {
        uint32 arg1 = array[i];

        if (remove2 != NULL) {
          uint32 count1 = master_bin_table_count_1(table, arg1);
          uint32 read1 = 0;
          while (read1 < count1) {
            uint32 buffer[64];
            UINT32_ARRAY array1 = master_bin_table_range_restrict_1(table, arg1, read1, buffer, 64);
            read1 += array1.size;
            for (uint32 i1=0 ; i1 < array1.size ; i1++) {
              uint32 arg2 = array1.array[i1];
              assert(master_bin_table_count_2(table, arg2) > 0);
              if (master_bin_table_count_2(table, arg2) == 1) {
                if (!col_2_bit_map_built) {
                  master_bin_table_aux_build_col_2_insertion_bitmap(table_aux, col_2_bit_map, mem_pool);
                  col_2_bit_map_built = true;
                }
                if (!has_insertions || !col_update_bit_map_is_set(col_2_bit_map, arg2))
                  remove2(store2, arg2, mem_pool);
              }
            }
          }
        }

        if (master_bin_table_delete_1(table, arg1) > 0) {
          assert(master_bin_table_count_1(table, arg1) == 0);
          if (remove1 != NULL) {
            if (!col_1_bit_map_built) {
              master_bin_table_aux_build_col_1_insertion_bitmap(table_aux, col_1_bit_map, mem_pool);
              col_1_bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(col_1_bit_map, arg1))
              remove1(store1, arg1, mem_pool);
          }

        }
      }
    }

    if (del_2_count > 0) {
      uint32 *array = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < del_2_count ; i++) {
        uint32 arg2 = array[i];

        if (remove1 != NULL) {
          uint32 count2 = master_bin_table_count_2(table, arg2);
          uint32 read2 = 0;
          while (read2 < count2) {
            uint32 buffer[64];
            UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
            read2 += array2.size;
            for (uint32 i2=0 ; i2 < array2.size ; i2++) {
              uint32 arg1 = array2.array[i2];
              assert(master_bin_table_count_1(table, arg1) > 0);
              if (master_bin_table_count_1(table, arg1) == 1) {
                if (!col_1_bit_map_built) {
                  master_bin_table_aux_build_col_1_insertion_bitmap(table_aux, col_1_bit_map, mem_pool);
                  col_1_bit_map_built = true;
                }
                if (!has_insertions || !col_update_bit_map_is_set(col_1_bit_map, arg1))
                  remove1(store1, arg1, mem_pool);
              }
            }
          }
        }

        if (master_bin_table_delete_2(table, arg2) > 0) {
          assert(master_bin_table_count_2(table, arg2) == 0);
          if (remove2 != NULL) {
            if (!col_2_bit_map_built) {
              master_bin_table_aux_build_col_2_insertion_bitmap(table_aux, col_2_bit_map, mem_pool);
              col_2_bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(col_2_bit_map, arg2))
              remove2(store2, arg2, mem_pool);
          }
        }
      }
    }

    if (has_insertions && col_1_bit_map_built)
      col_update_bit_map_clear(col_1_bit_map);

    if (has_insertions && col_2_bit_map_built)
      col_update_bit_map_clear(col_2_bit_map);
  }

  if (locks_applied && table_aux->locked_surrs.count > 0)
    master_bin_table_aux_unlock_surrs(table, &table_aux->locked_surrs);

  assert(table->table.forward.count == table->table.backward.count);
}

void master_bin_table_aux_apply_insertions(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(table->table.forward.count == table->table.backward.count);

  uint32 ins_count = table_aux->insertions.count;
  if (ins_count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < ins_count ; i++) {
      uint32 arg1 = (*ptr)[0];
      uint32 arg2 = (*ptr)[1];
      uint32 surr = (*ptr)[2];
      master_bin_table_insert_with_surr(table, arg1, arg2, surr, mem_pool);
      ptr++;
    }
  }

  assert(table->table.forward.count == table->table.backward.count);
}

////////////////////////////////////////////////////////////////////////////////

//## BAD BAD BAD: THIS IS VERY INEFFICIENT WHEN THIS METHOD IS CALLED REPEATEDLY
static uint32 master_bin_table_aux_number_of_deletions_1(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  assert(!queue_u32_contains(&table_aux->deletions_1, arg1));
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 num_unique_deletions = 0;

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      if (unpack_arg1(args) == arg1) {
        uint32 arg2 = unpack_arg2(args);
        assert(master_bin_table_contains(table, arg1, arg2));
        uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
        assert(surr != 0xFFFFFFFF);
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, surr, table_aux->mem_pool))
          num_unique_deletions++;
      }
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2s[i]);
      if (surr != 0xFFFFFFFF)
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, surr, table_aux->mem_pool))
          num_unique_deletions++;
    }
  }

  col_update_bit_map_clear(&table_aux->bit_map);

  return num_unique_deletions;
}

//## BAD BAD BAD: THIS IS VERY INEFFICIENT WHEN THIS METHOD IS CALLED REPEATEDLY
static uint32 master_bin_table_aux_number_of_deletions_2(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  assert(!queue_u32_contains(&table_aux->deletions_2, arg2));
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 num_unique_deletions = 0;

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      if (unpack_arg2(args) == arg2) {
        uint32 arg1 = unpack_arg1(args);
        assert(master_bin_table_contains(table, arg1, arg2));
        uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
        assert(surr != 0xFFFFFFFF);
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, table_aux->mem_pool))
          num_unique_deletions++;
      }
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 surr = master_bin_table_lookup_surr(table, arg1s[i], arg2);
      if (surr != 0xFFFFFFFF)
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, surr, table_aux->mem_pool))
          num_unique_deletions++;
    }
  }

  col_update_bit_map_clear(&table_aux->bit_map);

  return num_unique_deletions;
}

static uint32 master_bin_table_aux_number_of_deletions(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 num_unique_deletions = 0;

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(master_bin_table_contains(table, arg1, arg2));
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      if (col_update_bit_map_check_and_set(&table_aux->bit_map, surr, table_aux->mem_pool))
        num_unique_deletions++;
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];

      uint32 count1 = master_bin_table_count_1(table, arg1);
      uint32 read = 0;
      while (read < count1) {
        uint32 buffer[128];
        UINT32_ARRAY array1 = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array1.size;
        uint32 *surrs = array1.array + array1.offset;
        for (uint32 j=0 ; j < array1.size ; j++)
          if (col_update_bit_map_check_and_set(&table_aux->bit_map, surrs[j], table_aux->mem_pool))
            num_unique_deletions++;
      }
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];

      uint32 count2 = master_bin_table_count_2(table, arg2);
      uint32 read = 0;
      while (read < count2) {
        uint32 buffer[128];
        UINT32_ARRAY array2 = master_bin_table_range_restrict_2_with_surrs(table, arg2, read, buffer, 64);
        read += array2.size;
        uint32 *surrs = array2.array + array2.offset;
        for (uint32 j=0 ; j < array2.size ; j++)
          if (col_update_bit_map_check_and_set(&table_aux->bit_map, surrs[j], table_aux->mem_pool))
            num_unique_deletions++;
      }
    }
  }

  col_update_bit_map_clear(&table_aux->bit_map);

  return num_unique_deletions;
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_prepare(MASTER_BIN_TABLE_AUX *table_aux) {
  queue_u32_prepare(&table_aux->deletions_1);
  queue_u32_prepare(&table_aux->deletions_2);
  queue_3u32_prepare(&table_aux->insertions);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_aux_size(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux) {
  if (table_aux->clear)
    return table_aux->insertions.count;

  uint32 dels_count = master_bin_table_aux_number_of_deletions(table, table_aux);
  return master_bin_table_size(table) - dels_count + table_aux->insertions.count;
}

bool master_bin_table_aux_contains(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (queue_3u32_contains_12(&table_aux->insertions, arg1, arg2))
    return true;

  uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
  assert(master_bin_table_contains(table, arg1, arg2) == (surr != 0xFFFFFFFF));

  if (surr == 0xFFFFFFFF)
    return false;

  if (table_aux->clear)
    return false;

  if (queue_u32_contains(&table_aux->deletions_1, arg1))
    return false;

  if (queue_u32_contains(&table_aux->deletions_2, arg2))
    return false;

  uint64 args = pack_args(arg1, arg2);
  if (queue_u64_contains(&table_aux->deletions, args))
    return false;

  return true;
}

bool master_bin_table_aux_contains_1(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (queue_3u32_contains_1(&table_aux->insertions, arg1))
    return true;

  if (!master_bin_table_contains_1(table, arg1))
    return false;

  if (table_aux->clear)
    return false;

  if (queue_u32_contains(&table_aux->deletions_1, arg1))
    return false;

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_2 == 0)
    return true;

  return master_bin_table_aux_number_of_deletions_1(table, table_aux, arg1) < master_bin_table_count_1(table, arg1);
}

bool master_bin_table_aux_contains_2(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  if (queue_3u32_contains_2(&table_aux->insertions, arg2))
    return true;

  if (!master_bin_table_contains_2(table, arg2))
    return false;

  if (table_aux->clear)
    return false;

  if (queue_u32_contains(&table_aux->deletions_2, arg2))
    return false;

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;

  if (num_dels == 0 & num_dels_1 == 0)
    return true;

  return master_bin_table_aux_number_of_deletions_2(table, table_aux, arg2) < master_bin_table_count_2(table, arg2);
}

bool master_bin_table_aux_contains_surr(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 surr) {
  if (queue_3u32_contains_3(&table_aux->insertions, surr))
    return true;

  if (table_aux->clear)
    return false;

  if (!master_bin_table_contains_surr(table, surr))
    return false;

  uint32 arg1 = master_bin_table_get_arg_1(table, surr);
  uint32 arg2 = master_bin_table_get_arg_2(table, surr);

  if (queue_u32_contains(&table_aux->deletions_1, arg1))
    return false;

  if (queue_u32_contains(&table_aux->deletions_2, arg2))
    return false;

  uint64 args = pack_args(arg1, arg2);
  if (queue_u64_contains(&table_aux->deletions, args))
    return false;

  return true;
}

bool master_bin_table_aux_is_empty(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux) {
  if (table_aux->insertions.count > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = master_bin_table_size(table);
  if (size == 0)
    return true;

  queue_u64_remove_duplicates(&table_aux->deletions);
  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels > 0) {
    if (num_dels_1 > 0) {
      if (num_dels_2 > 0) {
        // NZ NZ NZ
        return master_bin_table_aux_number_of_deletions(table, table_aux) == size;
      }
      else {
        // NZ NZ Z
        return master_bin_table_aux_number_of_deletions(table, table_aux) == size;
      }
    }
    else {
      if (num_dels_2 > 0) {
        // NZ Z NZ
        return master_bin_table_aux_number_of_deletions(table, table_aux) == size;
      }
      else {
        // NZ Z Z
        return num_dels == size; //## BUG BUG BUG (WHY? CAN'T REMEMBER)
      }
    }
  }
  else {
    if (num_dels_1 > 0) {
      if (num_dels_2 > 0) {
        // Z NZ NZ
        return master_bin_table_aux_number_of_deletions(table, table_aux) == size;
      }
      else {
        // Z NZ Z
        uint32 total_num_dels = 0;
        uint32 *arg1s = table_aux->deletions_1.array;
        for (uint32 i=0 ; i < num_dels_1 ; i++)
          total_num_dels += master_bin_table_count_1(table, arg1s[i]);
        return total_num_dels == size;
      }
    }
    else {
      if (num_dels_2 > 0) {
        // Z Z NZ
        uint32 total_num_dels = 0;
        uint32 *arg2s = table_aux->deletions_2.array;
        for (uint32 i=0 ; i < num_dels_2 ; i++)
          total_num_dels += master_bin_table_count_2(table, arg2s[i]);
        return total_num_dels == size;
      }
      else {
        // Z Z Z
        return false;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool master_bin_table_aux_check_foreign_key_unary_table_1_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 (*insertions)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 arg1 = insertions[i][0];
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_unary_table_2_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 (*insertions)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 arg2 = insertions[i][1];
      if (!unary_table_aux_contains(target_table, target_table_aux, arg2)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

//## BAD BAD BAD: THE FOLLOWING FOUR METHODS ARE NEARLY IDENTICAL

bool master_bin_table_aux_check_foreign_key_slave_tern_table_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, BIN_TABLE *target_table, SLAVE_TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 (*insertions)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i][2];
      if (!bin_table_aux_contains_1(target_table, &target_table_aux->slave_table_aux, surr)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_obj_col_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, OBJ_COL *target_col, OBJ_COL_AUX *target_col_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 (*insertions)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i][2];
      if (!obj_col_aux_contains_1(target_col, target_col_aux, surr)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_int_col_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, INT_COL *target_col, INT_COL_AUX *target_col_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 (*insertions)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i][2];
      if (!int_col_aux_contains_1(target_col, target_col_aux, surr)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_float_col_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, FLOAT_COL *target_col, FLOAT_COL_AUX *target_col_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 (*insertions)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i][2];
      if (!float_col_aux_contains_1(target_col, target_col_aux, surr)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

// unary(X) -> binary(X, _)
bool master_bin_table_aux_check_foreign_key_unary_table_1_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = unary_table_aux_size(src_table, src_table_aux);
      if (src_size > ins_count) {
        //## RECORD THE ERROR
        return false;
      }

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      uint32 (*insertions)[3] = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = insertions[i][0];
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, mem_pool))
          if (unary_table_aux_contains(src_table, src_table_aux, arg1))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool ok = found == src_size;
      if (!ok) {
        //## RECORD THE ERROR
      }
      return ok;
    }
    else {
      bool src_is_empty = unary_table_aux_is_empty(src_table, src_table_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
        if (!queue_3u32_contains_1(&table_aux->insertions, arg1)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels > 0 | num_dels_2 > 0) {
    STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

    if (table_aux->insertions.count > 0)
      master_bin_table_aux_build_col_1_insertion_bitmap(table_aux, &table_aux->bit_map, mem_pool);

    COL_UPDATE_BIT_MAP *surr_deleted_bit_map = &table_aux->another_bit_map;

    TRNS_MAP_SURR_U32 remaining;
    trns_map_surr_u32_init(&remaining, mem_pool);

    if (num_dels > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint64 args = args_array[i];
        uint32 arg1 = unpack_arg1(args);
        if (!col_update_bit_map_is_set(&table_aux->bit_map, arg1)) {
          if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
            uint32 arg2 = unpack_arg2(args);
            assert(master_bin_table_contains(table, arg1, arg2));
            uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
            if (!col_update_bit_map_check_and_set(&table_aux->another_bit_map, surr, mem_pool)) {
              uint32 count = trns_map_surr_u32_lookup(&remaining, arg1, 0);
              if (count == 0)
                count = master_bin_table_count_1(table, arg1);
              assert(count > 0);
              if (count == 1) {
                // No more references left
                //## RECORD THE ERROR
                return false;
              }
              else
                trns_map_surr_u32_set(&remaining, arg1, count - 1);
            }
          }
        }
      }
    }

    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];

        uint32 count2 = master_bin_table_count_2(table, arg2);
        uint32 read = 0;
        while (read < count2) {
          uint32 buffer[128];
          UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(table, arg2, read, buffer, 64);
          read += array.size;
          uint32 *surrs = array.array + array.offset;
          for (uint32 j=0 ; j < array.size ; j++) {
            uint32 arg1 = array.array[j];
            if (!col_update_bit_map_is_set(&table_aux->bit_map, arg1)) {
              if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
                if (!col_update_bit_map_check_and_set(&table_aux->another_bit_map, surrs[j], mem_pool)) {
                  uint32 count1 = trns_map_surr_u32_lookup(&remaining, arg1, 0);
                  if (count1 == 0)
                    count1 = master_bin_table_count_1(table, arg1);
                  assert(count1 > 0);
                  if (count1 == 1) {
                    // No more references left
                    //## RECORD THE ERROR
                    return false;
                  }
                  else
                    trns_map_surr_u32_set(&remaining, arg1, count1 - 1);
                }
              }
            }
          }
        }
      }
    }

    col_update_bit_map_clear(&table_aux->bit_map);
    col_update_bit_map_clear(&table_aux->another_bit_map);
  }

  return true;
}

// unary(Y) -> binary(_, Y)
bool master_bin_table_aux_check_foreign_key_unary_table_2_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = unary_table_aux_size(src_table, src_table_aux);
      if (src_size > ins_count) {
        //## RECORD THE ERROR
        return false;
      }

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      uint32 (*insertions)[3] = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg2 = insertions[i][1];
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg2, mem_pool))
          if (unary_table_aux_contains(src_table, src_table_aux, arg2))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      if (!all_src_elts_found) {
        //## RECORD THE ERROR
      }
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = unary_table_aux_is_empty(src_table, src_table_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
        if (!queue_3u32_contains_2(&table_aux->insertions, arg2)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;

  if (num_dels > 0 | num_dels_1 > 0) {
    STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

    if (table_aux->insertions.count > 0)
      master_bin_table_aux_build_col_2_insertion_bitmap(table_aux, &table_aux->bit_map, mem_pool);

    COL_UPDATE_BIT_MAP *surr_deleted_bit_map = &table_aux->another_bit_map;

    TRNS_MAP_SURR_U32 remaining;
    trns_map_surr_u32_init(&remaining, mem_pool);

    if (num_dels > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint64 args = args_array[i];
        uint32 arg2 = unpack_arg2(args);
        if (!col_update_bit_map_is_set(&table_aux->bit_map, arg2)) {
          if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
            uint32 arg1 = unpack_arg1(args);
            assert(master_bin_table_contains(table, arg2, arg1));
            uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
            if (!col_update_bit_map_check_and_set(&table_aux->another_bit_map, surr, mem_pool)) {
              uint32 count = trns_map_surr_u32_lookup(&remaining, arg2, 0);
              if (count == 0)
                count = master_bin_table_count_2(table, arg2);
              assert(count > 0);
              if (count == 1) {
                // No more references left
                //## RECORD THE ERROR
                return false;
              }
              trns_map_surr_u32_set(&remaining, arg2, count - 1);
            }
          }
        }
      }
    }

    if (num_dels_1 > 0) {
      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels_1 ; i++) {
        uint32 arg1 = arg1s[i];

        uint32 count1 = master_bin_table_count_1(table, arg1);
        uint32 read = 0;
        while (read < count1) {
          uint32 buffer[128];
          UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
          read += array.size;
          uint32 *surrs = array.array + array.offset;
          for (uint32 j=0 ; j < array.size ; j++) {
            uint32 arg2 = array.array[j];
            if (!col_update_bit_map_is_set(&table_aux->bit_map, arg2)) {
              if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
                if (!col_update_bit_map_check_and_set(&table_aux->another_bit_map, surrs[j], mem_pool)) {
                  uint32 count2 = trns_map_surr_u32_lookup(&remaining, arg2, 0);
                  if (count2 == 0)
                    count2 = master_bin_table_count_2(table, arg2);
                  assert(count2 > 0);
                  if (count2 == 1) {
                    // No more references left
                    //## RECORD THE ERROR
                    return false;
                  }
                  trns_map_surr_u32_set(&remaining, arg2, count2 - 1);
                }
              }
            }
          }
        }
      }
    }

    col_update_bit_map_clear(&table_aux->bit_map);
    col_update_bit_map_clear(&table_aux->another_bit_map);
  }

  return true;
}

// ternary(X, Y, _) -> binary(X, Y)
bool master_bin_table_aux_check_foreign_key_slave_tern_table_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, BIN_TABLE *src_table, SLAVE_TERN_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = slave_tern_table_aux_size(src_table, src_table_aux);
      if (src_size > ins_count) {
        //## RECORD THE ERROR
        return false;
      }

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      uint32 (*insertions)[3] = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i][2];
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      if (!all_src_elts_found) {
        //## RECORD THE ERROR
      }
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = slave_tern_table_aux_is_empty(src_table, src_table_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

  COL_UPDATE_BIT_MAP *surrs_ins_bit_map = &table_aux->bit_map;
  if (table_aux->insertions.count > 0)
    master_bin_table_aux_build_surr_insertion_bitmap(table_aux, surrs_ins_bit_map, mem_pool);

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(master_bin_table_contains(table, arg1, arg2));
      //## BAD BAD BAD: SINCE WE CHECK THAT THE TUPLE EXISTS BEFORE ADDING IT
      //## TO THE INSERTION LIST, WE MAY AS WELL LOOKUP THE SURROGATE THEN
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      assert(surr != 0xFFFFFFFF);
      if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
        if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      uint32 count1 = master_bin_table_count_1(table, arg1);
      uint32 read = 0;
      while (read < count1) {
        uint32 buffer[128];
        //## HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 count2 = master_bin_table_count_2(table, arg2);
      uint32 read = 0;
      while (read < count2) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(table, arg2, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}

// ternary(X, Y, _) -> binary(X, Y)
bool master_bin_table_aux_check_foreign_key_obj_col_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, OBJ_COL *src_col, OBJ_COL_AUX *src_col_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = obj_col_aux_size(src_col, src_col_aux);
      if (src_size > ins_count) {
        //## RECORD THE ERROR
        return false;
      }

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      uint32 (*insertions)[3] = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i][2];
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (obj_col_aux_contains_1(src_col, src_col_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      if (!all_src_elts_found) {
        //## RECORD THE ERROR
      }
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = obj_col_aux_is_empty(src_col, src_col_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

  COL_UPDATE_BIT_MAP *surrs_ins_bit_map = &table_aux->bit_map;
  if (table_aux->insertions.count > 0)
    master_bin_table_aux_build_surr_insertion_bitmap(table_aux, surrs_ins_bit_map, mem_pool);

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(master_bin_table_contains(table, arg1, arg2));
      //## BAD BAD BAD: SINCE WE CHECK THAT THE TUPLE EXISTS BEFORE ADDING IT
      //## TO THE INSERTION LIST, WE MAY AS WELL LOOKUP THE SURROGATE THEN
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      assert(surr != 0xFFFFFFFF);
      if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
        if (obj_col_aux_contains_1(src_col, src_col_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      uint32 count1 = master_bin_table_count_1(table, arg1);
      uint32 read = 0;
      while (read < count1) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (obj_col_aux_contains_1(src_col, src_col_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 count2 = master_bin_table_count_2(table, arg2);
      uint32 read = 0;
      while (read < count2) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(table, arg2, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (obj_col_aux_contains_1(src_col, src_col_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_int_col_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, INT_COL *src_col, INT_COL_AUX *src_col_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = int_col_aux_size(src_col, src_col_aux);
      if (src_size > ins_count) {
        //## RECORD THE ERROR
        return false;
      }

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      uint32 (*insertions)[3] = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i][2];
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (int_col_aux_contains_1(src_col, src_col_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      if (!all_src_elts_found) {
        //## RECORD THE ERROR
      }
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = int_col_aux_is_empty(src_col, src_col_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

  COL_UPDATE_BIT_MAP *surrs_ins_bit_map = &table_aux->bit_map;
  if (table_aux->insertions.count > 0)
    master_bin_table_aux_build_surr_insertion_bitmap(table_aux, surrs_ins_bit_map, mem_pool);

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(master_bin_table_contains(table, arg1, arg2));
      //## BAD BAD BAD: SINCE WE CHECK THAT THE TUPLE EXISTS BEFORE ADDING IT
      //## TO THE INSERTION LIST, WE MAY AS WELL LOOKUP THE SURROGATE THEN
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      assert(surr != 0xFFFFFFFF);
      if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
        if (int_col_aux_contains_1(src_col, src_col_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      uint32 count1 = master_bin_table_count_1(table, arg1);
      uint32 read = 0;
      while (read < count1) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (int_col_aux_contains_1(src_col, src_col_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 count2 = master_bin_table_count_2(table, arg2);
      uint32 read = 0;
      while (read < count2) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(table, arg2, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (int_col_aux_contains_1(src_col, src_col_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_float_col_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, FLOAT_COL *src_col, FLOAT_COL_AUX *src_col_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = float_col_aux_size(src_col, src_col_aux);
      if (src_size > ins_count) {
        //## RECORD THE ERROR
        return false;
      }

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      uint32 (*insertions)[3] = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i][2];
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (float_col_aux_contains_1(src_col, src_col_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      if (!all_src_elts_found) {
        //## RECORD THE ERROR
      }
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = float_col_aux_is_empty(src_col, src_col_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

  COL_UPDATE_BIT_MAP *surrs_ins_bit_map = &table_aux->bit_map;
  if (table_aux->insertions.count > 0)
    master_bin_table_aux_build_surr_insertion_bitmap(table_aux, surrs_ins_bit_map, mem_pool);

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(master_bin_table_contains(table, arg1, arg2));
      //## BAD BAD BAD: SINCE WE CHECK THAT THE TUPLE EXISTS BEFORE ADDING IT
      //## TO THE INSERTION LIST, WE MAY AS WELL LOOKUP THE SURROGATE THEN
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      assert(surr != 0xFFFFFFFF);
      if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
        if (float_col_aux_contains_1(src_col, src_col_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      uint32 count1 = master_bin_table_count_1(table, arg1);
      uint32 read = 0;
      while (read < count1) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (float_col_aux_contains_1(src_col, src_col_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 count2 = master_bin_table_count_2(table, arg2);
      uint32 read = 0;
      while (read < count2) {
        uint32 buffer[128];
        //## BAD BAD BAD: HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(table, arg2, read, buffer, 64);
        read += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 j=0 ; j < array.size ; j++) {
          uint32 surr = surrs[j];
          if (!col_update_bit_map_is_set(surrs_ins_bit_map, surr))
            if (float_col_aux_contains_1(src_col, src_col_aux, surr)) {
              //## RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}
