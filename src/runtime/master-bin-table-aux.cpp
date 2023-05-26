#include "lib.h"


bool is_locked(uint64 slot);

void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *, uint32 next_free);

bool master_bin_table_lock_surr(MASTER_BIN_TABLE *, uint32 surr);
bool master_bin_table_unlock_surr(MASTER_BIN_TABLE *, uint32 surr);

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *);

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_init(MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  table_aux->mem_pool = mem_pool;
  col_update_bit_map_init(&table_aux->batch_deletion_map_1);
  col_update_bit_map_init(&table_aux->batch_deletion_map_2);
  col_update_bit_map_init(&table_aux->insertion_map_1);
  col_update_bit_map_init(&table_aux->insertion_map_2);
  col_update_bit_map_init(&table_aux->surr_insert_map);
  col_update_bit_map_init(&table_aux->bit_map);
  col_update_bit_map_init(&table_aux->another_bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_3u32_init(&table_aux->insertions);
  queue_3u32_init(&table_aux->reinsertions);
  trns_map_surr_surr_surr_init(&table_aux->args_enc_surr_map);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  col_update_bit_map_clear(&table_aux->batch_deletion_map_1);
  col_update_bit_map_clear(&table_aux->batch_deletion_map_2);
  col_update_bit_map_clear(&table_aux->insertion_map_1);
  col_update_bit_map_clear(&table_aux->insertion_map_2);
  col_update_bit_map_clear(&table_aux->surr_insert_map);
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_3u32_reset(&table_aux->insertions);
  queue_3u32_reset(&table_aux->reinsertions);
  trns_map_surr_surr_surr_clear(&table_aux->args_enc_surr_map);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_partial_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));
  assert(table_aux->deletions.count == 0);
  assert(table_aux->deletions_1.count == 0);
  assert(table_aux->deletions_2.count == 0);
  assert(table_aux->insertions.count == 0);
  assert(table_aux->reinsertions.count == 0);
  assert(!table_aux->clear);

  trns_map_surr_surr_surr_clear(&table_aux->args_enc_surr_map);
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
    queue_3u32_insert(&table_aux->reinsertions, arg1, arg2, surr);
    return surr;
  }

  uint32 enc_surr_info = trns_map_surr_surr_surr_lookup(&table_aux->args_enc_surr_map, arg1, arg2);
  if (enc_surr_info != 0xFFFFFFFF) {
    surr = enc_surr_info >> 1;
    if ((enc_surr_info & 1) != 0) {
      queue_3u32_insert(&table_aux->insertions, arg1, arg2, surr);
      trns_map_surr_surr_surr_update(&table_aux->args_enc_surr_map, arg1, arg2, enc_surr_info & ~1);
      assert(trns_map_surr_surr_surr_lookup(&table_aux->args_enc_surr_map, arg1, arg2) == (surr << 1));
    }
    return surr;
  }

  surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
  table_aux->last_surr = surr;

  queue_3u32_insert(&table_aux->insertions, arg1, arg2, surr);
  trns_map_surr_surr_surr_insert_new(&table_aux->args_enc_surr_map, arg1, arg2, surr << 1);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
  if (surr != 0xFFFFFFFF)
    return surr;

  uint32 enc_surr_info = trns_map_surr_surr_surr_lookup(&table_aux->args_enc_surr_map, arg1, arg2);
  if (enc_surr_info != 0xFFFFFFFF)
    return enc_surr_info >> 1;

  surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
  table_aux->last_surr = surr;
  trns_map_surr_surr_surr_insert_new(&table_aux->args_enc_surr_map, arg1, arg2, (surr << 1) | 1);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_aux_lock_surrs(MASTER_BIN_TABLE *table, QUEUE_3U32 *reinsertions) {
  assert(reinsertions->count != 0);

  uint32 count = reinsertions->count;
  uint32 highest_surr = 0; // There's at least one surrogate, so zero as a default works
  TUPLE_3U32 *array = reinsertions->array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 surr = array[i].z;
    master_bin_table_lock_surr(table, surr);
    if (surr > highest_surr)
      highest_surr = surr;
  }
  return highest_surr;
}

void master_bin_table_aux_unlock_surrs(MASTER_BIN_TABLE *table, QUEUE_3U32 *reinsertions) {
  assert(reinsertions->count != 0);

  uint32 count = reinsertions->count;
  TUPLE_3U32 *array = reinsertions->array;
  for (uint32 i=0 ; i < count ; i++)
    master_bin_table_unlock_surr(table, array[i].z);
}

static void master_bin_table_aux_build_col_1_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(bit_map, insertions[i].x, mem_pool);
  }

  count = table_aux->reinsertions.count;
  if (count > 0) {
    TUPLE_3U32 *reinsertions = table_aux->reinsertions.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(bit_map, reinsertions[i].x, mem_pool);
  }
}

static void master_bin_table_aux_build_col_2_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(bit_map, insertions[i].y, mem_pool);
  }

  count = table_aux->reinsertions.count;
  if (count > 0) {
    TUPLE_3U32 *reinsertions = table_aux->reinsertions.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(bit_map, reinsertions[i].y, mem_pool);
  }
}

static void master_bin_table_aux_build_surr_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(bit_map, insertions[i].z, mem_pool);
  }
}

void master_bin_table_aux_apply_surrs_acquisition(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux) {
  assert(table->table.forward.count == table->table.backward.count);

  //## I'M ASSUMING THAT FOREIGN KEY CHECKS WILL BE ENOUGH TO AVOID THE
  //## UNUSED SURROGATE ALLOCATIONS, BUT I'M NOT SURE
  //## UPDATE AND REENABLE THIS CHECK
  // assert(trns_map_surr_surr_surr_is_empty(&table_aux->args_enc_surr_map));

  // Removing from the surrogates already reserved for newly inserted tuples
  // from the list of free ones, so we can append to that list while deleting
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
#ifndef NDEBUG
    if (table_aux->last_surr == 0xFFFFFFFF) {
      TUPLE_3U32 *insertions = table_aux->insertions.array;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = insertions[i].x;
        uint32 arg2 = insertions[i].y;
        uint32 surr = insertions[i].z;
        assert(master_bin_table_contains(table, arg1, arg2));
        assert(master_bin_table_lookup_surr(table, arg1, arg2) == surr);
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
  //## UPDATE AND REENABLE THIS CHECK
  // assert(trns_map_surr_surr_surr_is_empty(&table_aux->args_enc_surr_map));

  bool locks_applied = table_aux->reinsertions.count == 0;

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
        highest_locked_surr = master_bin_table_aux_lock_surrs(table, &table_aux->reinsertions);
        locks_applied = true;
      }

      master_bin_table_clear(table, highest_locked_surr, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0 || table_aux->reinsertions.count > 0;
    bool col_1_bit_map_built = !has_insertions ;
    bool col_2_bit_map_built = !has_insertions;

    COL_UPDATE_BIT_MAP *col_1_bit_map = &table_aux->bit_map;
    COL_UPDATE_BIT_MAP *col_2_bit_map = remove1 == NULL ? &table_aux->bit_map : &table_aux->another_bit_map;

    uint32 del_count = table_aux->deletions.count;
    uint32 del_1_count = table_aux->deletions_1.count;
    uint32 del_2_count = table_aux->deletions_2.count;

    if (del_count > 0 | del_1_count > 0 | del_2_count > 0)
      if (!locks_applied) {
        master_bin_table_aux_lock_surrs(table, &table_aux->reinsertions);
        locks_applied = true;
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

  if (locks_applied && table_aux->reinsertions.count > 0)
    master_bin_table_aux_unlock_surrs(table, &table_aux->reinsertions);

  assert(table->table.forward.count == table->table.backward.count);
}

void master_bin_table_aux_apply_insertions(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(table->table.forward.count == table->table.backward.count);

  uint32 ins_count = table_aux->insertions.count;
  if (ins_count > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < ins_count ; i++) {
      TUPLE_3U32 tuple = insertions[i];
      master_bin_table_insert_with_surr(table, tuple.x, tuple.y, tuple.z, mem_pool);
    }
  }

  uint32 reins_count = table_aux->reinsertions.count;
  if (reins_count > 0) {
    TUPLE_3U32 *reinsertions = table_aux->reinsertions.array;
    for (uint32 i=0 ; i < reins_count ; i++) {
      TUPLE_3U32 tuple = reinsertions[i];
      master_bin_table_insert_with_surr(table, tuple.x, tuple.y, tuple.z, mem_pool);
    }
  }

  assert(table->table.forward.count == table->table.backward.count);
}

////////////////////////////////////////////////////////////////////////////////

//## BAD BAD BAD: THIS IS VERY INEFFICIENT WHEN THIS METHOD IS CALLED REPEATEDLY
static uint32 master_bin_table_aux_number_of_deletions_1(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
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

bool master_bin_table_aux_was_batch_deleted_1(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  uint32 count = table_aux->deletions_1.count;
  if (count == 0)
    return false;
  if (!col_update_bit_map_is_dirty(&table_aux->batch_deletion_map_1)) {
    STATE_MEM_POOL *mem_pool = table_aux->mem_pool;
    uint32 *array = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(&table_aux->batch_deletion_map_1, array[i], mem_pool);
  }
  return col_update_bit_map_is_set(&table_aux->batch_deletion_map_1, arg1);
}

bool master_bin_table_aux_was_batch_deleted_2(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  uint32 count = table_aux->deletions_2.count;
  if (count == 0)
    return false;
  if (!col_update_bit_map_is_dirty(&table_aux->batch_deletion_map_2)) {
    STATE_MEM_POOL *mem_pool = table_aux->mem_pool;
    uint32 *array = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_bit_map_set(&table_aux->batch_deletion_map_2, array[i], mem_pool);
  }
  return col_update_bit_map_is_set(&table_aux->batch_deletion_map_2, arg2);
}

bool master_bin_table_aux_was_inserted_1(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins == 0)
    return false;

  if (!col_update_bit_map_is_dirty(&table_aux->insertion_map_1)) {
    STATE_MEM_POOL *mem_pool = table_aux->mem_pool;
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      col_update_bit_map_set(&table_aux->insertion_map_1, insertions[i].x, mem_pool);
  }

  return col_update_bit_map_is_set(&table_aux->insertion_map_1, arg1);
}

bool master_bin_table_aux_was_inserted_2(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins == 0)
    return false;

  if (!col_update_bit_map_is_dirty(&table_aux->insertion_map_2)) {
    STATE_MEM_POOL *mem_pool = table_aux->mem_pool;
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      col_update_bit_map_set(&table_aux->insertion_map_2, insertions[i].y, mem_pool);
  }

  return col_update_bit_map_is_set(&table_aux->insertion_map_2, arg2);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_aux_size(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux) {
  uint32 reins_count = table_aux->reinsertions.count;
  if (reins_count > 1) {
    assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));
    queue_3u32_deduplicate_by_3(&table_aux->reinsertions, &table_aux->bit_map, table_aux->mem_pool);
    reins_count = table_aux->reinsertions.count;
  }

  uint32 ins_count = table_aux->insertions.count + reins_count;

  if (table_aux->clear)
    return ins_count;

  uint32 dels_count = master_bin_table_aux_number_of_deletions(table, table_aux);
  return master_bin_table_size(table) - dels_count + ins_count;
}

bool master_bin_table_aux_contains(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 enc_surr_info = trns_map_surr_surr_surr_lookup(&table_aux->args_enc_surr_map, arg1, arg2);
  if (enc_surr_info != 0xFFFFFFFF)
    return true;

  uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
  assert(master_bin_table_contains(table, arg1, arg2) == (surr != 0xFFFFFFFF));
  if (surr == 0xFFFFFFFF)
    return false;

  if (table_aux->clear)
    return false;

  if (master_bin_table_aux_was_batch_deleted_1(table_aux, arg1))
    return false;

  if (master_bin_table_aux_was_batch_deleted_2(table_aux, arg2))
    return false;

  uint64 args = pack_args(arg1, arg2);
  if (queue_u64_contains(&table_aux->deletions, args))
    return false;

  return true;
}

bool master_bin_table_aux_contains_1(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (master_bin_table_aux_was_inserted_1(table_aux, arg1))
    return true;

  if (!master_bin_table_contains_1(table, arg1))
    return false;

  if (table_aux->clear)
    return false;

  if (master_bin_table_aux_was_batch_deleted_1(table_aux, arg1))
    return false;

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_2 == 0)
    return true;

  return master_bin_table_aux_number_of_deletions_1(table, table_aux, arg1) < master_bin_table_count_1(table, arg1);
}

bool master_bin_table_aux_contains_2(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  if (master_bin_table_aux_was_inserted_2(table_aux, arg2))
    return true;

  if (!master_bin_table_contains_2(table, arg2))
    return false;

  if (table_aux->clear)
    return false;

  if (master_bin_table_aux_was_batch_deleted_2(table_aux, arg2))
    return false;

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;

  if (num_dels == 0 & num_dels_1 == 0)
    return true;

  return master_bin_table_aux_number_of_deletions_2(table, table_aux, arg2) < master_bin_table_count_2(table, arg2);
}

bool master_bin_table_aux_contains_surr(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 surr) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    if (!col_update_bit_map_is_dirty(&table_aux->surr_insert_map)) {
      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;
      TUPLE_3U32 *insertions = table_aux->insertions.array;
      for (uint32 i=0 ; i < num_ins ; i++)
        col_update_bit_map_set(&table_aux->surr_insert_map, insertions[i].z, mem_pool);
    }
    if (col_update_bit_map_is_set(&table_aux->surr_insert_map, surr))
      return true;
  }

  if (table_aux->clear)
    return false;

  if (!master_bin_table_contains_surr(table, surr))
    return false;

  uint32 arg1 = master_bin_table_get_arg_1(table, surr);
  uint32 arg2 = master_bin_table_get_arg_2(table, surr);

  if (master_bin_table_aux_was_batch_deleted_1(table_aux, arg1))
    return false;

  if (master_bin_table_aux_was_batch_deleted_2(table_aux, arg2))
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

  queue_u64_deduplicate(&table_aux->deletions);
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
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 arg1 = insertions[i].x;
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1))
        return false;
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_unary_table_2_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 arg2 = insertions[i].y;
      if (!unary_table_aux_contains(target_table, target_table_aux, arg2))
        return false;
    }
  }
  return true;
}

//## BAD BAD BAD: THE FOLLOWING FOUR METHODS ARE NEARLY IDENTICAL

bool master_bin_table_aux_check_foreign_key_slave_tern_table_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, BIN_TABLE *target_table, SLAVE_TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i].z;
      if (!bin_table_aux_contains_1(target_table, &target_table_aux->slave_table_aux, surr))
        return false;
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_obj_col_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, OBJ_COL *target_col, OBJ_COL_AUX *target_col_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i].z;
      if (!obj_col_aux_contains_1(target_col, target_col_aux, surr))
        return false;
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_int_col_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, INT_COL *target_col, INT_COL_AUX *target_col_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i].z;
      if (!int_col_aux_contains_1(target_col, target_col_aux, surr))
        return false;
    }
  }
  return true;
}

bool master_bin_table_aux_check_foreign_key_float_col_forward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, FLOAT_COL *target_col, FLOAT_COL_AUX *target_col_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    TUPLE_3U32 *insertions = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = insertions[i].z;
      if (!float_col_aux_contains_1(target_col, target_col_aux, surr))
        return false;
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
      if (src_size > ins_count)
        return false;

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      TUPLE_3U32 *insertions = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = insertions[i].x;
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, mem_pool))
          if (unary_table_aux_contains(src_table, src_table_aux, arg1))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool ok = found == src_size;
      return ok;
    }
    else {
      bool src_is_empty = unary_table_aux_is_empty(src_table, src_table_aux);
      return src_is_empty;
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
        if (!master_bin_table_aux_was_inserted_1(table_aux, arg1))
          return false;
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
    trns_map_surr_u32_init(&remaining);

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
              if (count == 1)
                return false; // No more references left
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
                  if (count1 == 1)
                    return false; // No more references left
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
      if (src_size > ins_count)
        return false;

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      TUPLE_3U32 *insertions = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg2 = insertions[i].y;
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg2, mem_pool))
          if (unary_table_aux_contains(src_table, src_table_aux, arg2))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = unary_table_aux_is_empty(src_table, src_table_aux);
      return src_is_empty;
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
        if (!master_bin_table_aux_was_inserted_2(table_aux, arg2))
          return false;
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
    trns_map_surr_u32_init(&remaining);

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
              if (count == 1)
                return false; // No more references left
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
                  if (count2 == 1)
                    return false; // No more references left
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
      if (src_size > ins_count)
        return false;

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      TUPLE_3U32 *insertions = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i].z;
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = slave_tern_table_aux_is_empty(src_table, src_table_aux);
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
        if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr))
          return false;
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
            if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr))
              return false;
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
            if (slave_tern_table_aux_contains_surr(src_table, src_table_aux, surr))
              return false;
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
      if (src_size > ins_count)
        return false;

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      TUPLE_3U32 *insertions = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i].z;
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (obj_col_aux_contains_1(src_col, src_col_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = obj_col_aux_is_empty(src_col, src_col_aux);
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
        if (obj_col_aux_contains_1(src_col, src_col_aux, surr))
          return false;
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
            if (obj_col_aux_contains_1(src_col, src_col_aux, surr))
              return false;
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
            if (obj_col_aux_contains_1(src_col, src_col_aux, surr))
              return false;
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
      if (src_size > ins_count)
        return false;

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      TUPLE_3U32 *insertions = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i].z;
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (int_col_aux_contains_1(src_col, src_col_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = int_col_aux_is_empty(src_col, src_col_aux);
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
        if (int_col_aux_contains_1(src_col, src_col_aux, surr))
          return false;
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
            if (int_col_aux_contains_1(src_col, src_col_aux, surr))
              return false;
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
            if (int_col_aux_contains_1(src_col, src_col_aux, surr))
              return false;
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
      if (src_size > ins_count)
        return false;

      STATE_MEM_POOL *mem_pool = table_aux->mem_pool;

      TUPLE_3U32 *insertions = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 surr = insertions[i].z;
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, surr, mem_pool))
          if (float_col_aux_contains_1(src_col, src_col_aux, surr))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      assert(found <= src_size);
      bool all_src_elts_found = found == src_size;
      return all_src_elts_found;
    }
    else {
      bool src_is_empty = float_col_aux_is_empty(src_col, src_col_aux);
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
        if (float_col_aux_contains_1(src_col, src_col_aux, surr))
          return false;
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
            if (float_col_aux_contains_1(src_col, src_col_aux, surr))
              return false;
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
            if (float_col_aux_contains_1(src_col, src_col_aux, surr))
              return false;
        }
      }
    }
  }

  return true;
}
