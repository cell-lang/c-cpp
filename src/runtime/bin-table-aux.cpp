#include "lib.h"


void bin_table_aux_init(BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_bit_map_init(&table_aux->bit_map);
  col_update_bit_map_init(&table_aux->another_bit_map);
  col_update_bit_map_init(&table_aux->full_deletion_map_1);
  col_update_bit_map_init(&table_aux->full_deletion_map_2);
  col_update_bit_map_init(&table_aux->insertion_map_1);
  col_update_bit_map_init(&table_aux->insertion_map_2);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

void bin_table_aux_reset(BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));
  col_update_bit_map_clear(&table_aux->full_deletion_map_1);
  col_update_bit_map_clear(&table_aux->full_deletion_map_2);
  col_update_bit_map_clear(&table_aux->insertion_map_1);
  col_update_bit_map_clear(&table_aux->insertion_map_2);
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_u64_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_aux_clear(BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void bin_table_aux_delete(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (bin_table_contains(table, arg1, arg2))
    queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void bin_table_aux_delete_1(BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void bin_table_aux_delete_2(BIN_TABLE_AUX *table_aux, uint32 arg2) {
  queue_u32_insert(&table_aux->deletions_2, arg2);
}

void bin_table_aux_insert(BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->insertions, pack_args(arg1, arg2));
}

////////////////////////////////////////////////////////////////////////////////

static void bin_table_aux_build_col_1_insertion_bitmap(BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *args_array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = unpack_arg1(args_array[i]);
      col_update_bit_map_set(bit_map, arg1, mem_pool);
    }
  }
}

static void bin_table_aux_build_col_2_insertion_bitmap(BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *args_array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg2 = unpack_arg2(args_array[i]);
      col_update_bit_map_set(bit_map, arg2, mem_pool);
    }
  }
}

void bin_table_aux_apply_deletions(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    uint32 count = bin_table_size(table);
    if (count > 0) {
      if (table_aux->insertions.count == 0) {
        if (remove1 != NULL)
          remove1(store1, 0xFFFFFFFF, mem_pool);
        if (remove2 != NULL)
          remove2(store2, 0xFFFFFFFF, mem_pool);
      }
      else {
        if (remove1 != NULL) {
          bin_table_aux_build_col_1_insertion_bitmap(table_aux, &table_aux->bit_map, mem_pool);
          uint32 read = 0;
          for (uint32 arg1=0 ; read < count ; arg1++) {
            uint32 count1 = bin_table_count_1(table, arg1);
            if (count1 > 0) {
              read += count1;
              if (!col_update_bit_map_is_set(&table_aux->bit_map, arg1))
                remove1(store1, arg1, mem_pool);
            }
          }
          col_update_bit_map_clear(&table_aux->bit_map);
        }

        if (remove2 != NULL) {
          bin_table_aux_build_col_2_insertion_bitmap(table_aux, &table_aux->bit_map, mem_pool);
          uint32 read = 0;
          for (uint32 arg2=0 ; read < count ; arg2++) {
            uint32 count2 = bin_table_count_2(table, arg2);
            if (count2 > 0) {
              read += count2;
              if (!col_update_bit_map_is_set(&table_aux->bit_map, arg2))
                remove2(store2, arg2, mem_pool);
            }
          }
          col_update_bit_map_clear(&table_aux->bit_map);
        }
      }

      bin_table_clear(table, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0;
    bool col_1_bit_map_built = !has_insertions;
    bool col_2_bit_map_built = !has_insertions;

    COL_UPDATE_BIT_MAP *col_1_bit_map = &table_aux->bit_map;
    COL_UPDATE_BIT_MAP *col_2_bit_map = remove1 == NULL ? &table_aux->bit_map : &table_aux->another_bit_map;

    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (bin_table_delete(table, arg1, arg2)) {
          if (remove1 != NULL && bin_table_count_1(table, arg1) == 0) {
            if (!col_1_bit_map_built) {
              bin_table_aux_build_col_1_insertion_bitmap(table_aux, col_1_bit_map, mem_pool);
              col_1_bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(col_1_bit_map, arg1))
              remove1(store1, arg1, mem_pool);
          }
          if (remove2 != NULL && bin_table_count_2(table, arg2) == 0) {
            if (!col_2_bit_map_built) {
              bin_table_aux_build_col_2_insertion_bitmap(table_aux, col_2_bit_map, mem_pool);
              col_2_bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(col_2_bit_map, arg2))
              remove2(store2, arg2, mem_pool);
          }
        }
      }
    }

    count = table_aux->deletions_1.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg1 = array[i];

        if (remove2 != NULL) {
          uint32 count1 = bin_table_count_1(table, arg1);
          uint32 read1 = 0;
          while (read1 < count1) {
            uint32 buffer[64];
            UINT32_ARRAY array1 = bin_table_range_restrict_1(table, arg1, read1, buffer, 64);
            read1 += array1.size;
            for (uint32 i1=0 ; i1 < array1.size ; i1++) {
              uint32 arg2 = array1.array[i1];
              assert(bin_table_count_2(table, arg2) > 0);
              if (bin_table_count_2(table, arg2) == 1) {
                if (!col_2_bit_map_built) {
                  bin_table_aux_build_col_2_insertion_bitmap(table_aux, col_2_bit_map, mem_pool);
                  col_2_bit_map_built = true;
                }
                if (!has_insertions || !col_update_bit_map_is_set(col_2_bit_map, arg2))
                  remove2(store2, arg2, mem_pool);
              }
            }
          }
        }

        bin_table_delete_1(table, arg1);
        assert(bin_table_count_1(table, arg1) == 0);
        if (remove1 != NULL) {
          if (!col_1_bit_map_built) {
            bin_table_aux_build_col_1_insertion_bitmap(table_aux, col_1_bit_map, mem_pool);
            col_1_bit_map_built = true;
          }
          if (!has_insertions || !col_update_bit_map_is_set(col_1_bit_map, arg1))
            remove1(store1, arg1, mem_pool);
        }
      }
    }

    count = table_aux->deletions_2.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg2 = array[i];

        if (remove1 != NULL) {
          uint32 count2 = bin_table_count_2(table, arg2);
          uint32 read2 = 0;
          while (read2 < count2) {
            uint32 buffer[64];
            UINT32_ARRAY array2 = bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
            read2 += array2.size;
            for (uint32 i2=0 ; i2 < array2.size ; i2++) {
              uint32 arg1 = array2.array[i2];
              assert(bin_table_count_1(table, arg1) > 0);
              if (bin_table_count_1(table, arg1) == 1) {
                if (!col_1_bit_map_built) {
                  bin_table_aux_build_col_1_insertion_bitmap(table_aux, col_1_bit_map, mem_pool);
                  col_1_bit_map_built = true;
                }
                if (!has_insertions || !col_update_bit_map_is_set(col_1_bit_map, arg1))
                  remove1(store1, arg1, mem_pool);
              }
            }
          }
        }

        bin_table_delete_2(table, arg2);
        assert(bin_table_count_2(table, arg2) == 0);
        if (remove2 != NULL) {
          if (!col_2_bit_map_built) {
            bin_table_aux_build_col_2_insertion_bitmap(table_aux, col_2_bit_map, mem_pool);
            col_2_bit_map_built = true;
          }
          if (!has_insertions || !col_update_bit_map_is_set(col_2_bit_map, arg2))
            remove2(store2, arg2, mem_pool);
        }
      }
    }

    if (has_insertions && col_1_bit_map_built)
      col_update_bit_map_clear(col_1_bit_map);

    if (has_insertions && col_2_bit_map_built)
      col_update_bit_map_clear(col_2_bit_map);
  }
}

void bin_table_aux_apply_insertions(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      bin_table_insert(table, arg1, arg2, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void record_col_1_key_violation(BIN_TABLE *col, BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, bool between_new) {
  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

static void record_col_2_key_violation(BIN_TABLE *col, BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, bool between_new) {
  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

inline void bin_table_aux_build_col_1_del_bitmap(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 del_count = table_aux->deletions.count;
  uint32 del_1_count = table_aux->deletions_1.count;
  uint32 del_2_count = table_aux->deletions_2.count;

  if (del_count == 0 & del_1_count == 0 & del_2_count == 0)
    return;

  COL_UPDATE_BIT_MAP *bit_map = &table_aux->bit_map;

  if (del_count != 0) {
    uint64 *array = table_aux->deletions.array;
    for (uint32 i=0 ; i < del_count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      col_update_bit_map_set(bit_map, arg1, mem_pool);
    }
  }

  if (del_1_count != 0) {
    uint32 *array = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < del_1_count ; i++)
      col_update_bit_map_set(bit_map, array[i], mem_pool);
  }

  if (del_2_count > 0) {
    uint32 *array = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < del_2_count ; i++) {
      uint32 arg2 = array[i];

      uint32 count2 = bin_table_count_2(table, arg2);
      uint32 read2 = 0;
      while (read2 < count2) {
        uint32 buffer[64];
        UINT32_ARRAY array2 = bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
        read2 += array2.size;
        for (uint32 i2=0 ; i2 < array2.size ; i2++) {
          uint32 arg1 = array2.array[i2];
          col_update_bit_map_set(bit_map, arg1, mem_pool);
        }
      }
    }
  }
}

inline void bin_table_aux_build_col_2_del_bitmap(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 del_count = table_aux->deletions.count;
  uint32 del_1_count = table_aux->deletions_1.count;
  uint32 del_2_count = table_aux->deletions_2.count;

  if (del_count == 0 & del_1_count == 0 & del_2_count == 0)
    return;

  COL_UPDATE_BIT_MAP *bit_map = &table_aux->bit_map;

  if (del_count != 0) {
    uint64 *array = table_aux->deletions.array;
    for (uint32 i=0 ; i < del_count ; i++) {
      uint64 args = array[i];
      uint32 arg2 = unpack_arg2(args);
      col_update_bit_map_set(bit_map, arg2, mem_pool);
    }
  }

  if (del_1_count != 0) {
    uint32 *array = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < del_1_count ; i++) {
      uint32 arg1 = array[i];

      uint32 count1 = bin_table_count_1(table, arg1);
      uint32 read1 = 0;
      while (read1 < count1) {
        uint32 buffer[64];
        UINT32_ARRAY array1 = bin_table_range_restrict_1(table, arg1, read1, buffer, 64);
        read1 += array1.size;
        for (uint32 i1=0 ; i1 < array1.size ; i1++) {
          uint32 arg2 = array1.array[i1];
          col_update_bit_map_set(bit_map, arg2, mem_pool);
        }
      }
    }
  }

  if (del_2_count != 0) {
    uint32 *array = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < del_2_count ; i++)
      col_update_bit_map_set(bit_map, array[i], mem_pool);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_aux_check_key_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
    uint64 *ins_array = table_aux->insertions.array;

    // Checking for conflicting insertions
    if (ins_count > 1) {
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg1 = unpack_arg1(args);
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, mem_pool)) {
          //## CHECK FIRST THAT THE CONFLICTING INSERTION DOES NOT HAVE THE SAME VALUE FOR THE SECOND ARGUMENT
          //## RECORD THE ERROR
          col_update_bit_map_clear(&table_aux->bit_map);
          return false;
        }
      }
      col_update_bit_map_clear(&table_aux->bit_map);
    }

    if (!table_aux->clear) {
      // Checking for conflicts between the new insertions and the preexisting tuples
      bool deletion_bit_map_built = false;

      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg1 = unpack_arg1(args);
        if (bin_table_contains_1(table, arg1)) {
          if (!deletion_bit_map_built) {
            bin_table_aux_build_col_1_del_bitmap(table, table_aux, mem_pool);
            deletion_bit_map_built = true;
          }

          if (!col_update_bit_map_is_set(&table_aux->bit_map, arg1)) {
            //## RECORD THE ERROR
            col_update_bit_map_clear(&table_aux->bit_map);
            return false;
          }
        }
      }

      if (deletion_bit_map_built)
        col_update_bit_map_clear(&table_aux->bit_map);
    }
  }

  return true;
}

bool bin_table_aux_check_key_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
    uint64 *ins_array = table_aux->insertions.array;

    // Checking for conflicting insertions
    if (ins_count > 1) {
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg2 = unpack_arg2(args);
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, arg2, mem_pool)) {
          //## CHECK FIRST THAT THE CONFLICTING INSERTION DOES NOT HAVE THE SAME VALUE FOR THE FIRST ARGUMENT
          //## RECORD THE ERROR
          col_update_bit_map_clear(&table_aux->bit_map);
          return false;
        }
      }
      col_update_bit_map_clear(&table_aux->bit_map);
    }

    if (!table_aux->clear) {
      // Checking for conflicts between the new insertions and the preexisting tuples
      bool deletion_bit_map_built = false;

      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg2 = unpack_arg2(args);
        if (bin_table_contains_2(table, arg2)) {
          if (!deletion_bit_map_built) {
            bin_table_aux_build_col_2_del_bitmap(table, table_aux, mem_pool);
            deletion_bit_map_built = true;
          }

          if (!col_update_bit_map_is_set(&table_aux->bit_map, arg2)) {
            //## RECORD THE ERROR
            col_update_bit_map_clear(&table_aux->bit_map);
            return false;
          }
        }
      }

      if (deletion_bit_map_built)
        col_update_bit_map_clear(&table_aux->bit_map);
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_aux_has_deletions(BIN_TABLE_AUX *table_aux) {
  return table_aux->deletions.count > 0 || table_aux->deletions_1.count > 0 || table_aux->deletions_2.count > 0;
}

// SO FAR THIS IS ONLY USED BY tern_table_aux_check_key_13(..) AND tern_table_aux_check_key_23(..),
// SO IT'S NOT SUPER IMPORTANT FOR PERFORMANCE.
bool bin_table_aux_was_deleted(BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (table_aux->clear)
    return true;

  //## TODO: IMPLEMENT FOR REAL

  uint32 count = table_aux->deletions_1.count;
  if (count > 0) {
    uint32 *array = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < count ; i++)
      if (array[i] == arg1)
        return true;
  }

  count = table_aux->deletions_2.count;
  if (count > 0) {
    uint32 *array = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < count ; i++)
      if (array[i] == arg2)
        return true;
  }

  count = table_aux->deletions.count;
  if (count > 0) {
    uint64 packed_args = pack_args(arg1, arg2);
    uint64 *array = table_aux->deletions.array;
    for (uint32 i=0 ; i < count ; i++)
      if (array[i] == packed_args)
        return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

static void bin_table_aux_build_full_deletion_map_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  STATE_MEM_POOL *mem_pool = table->mem_pool;

  uint32 dels_count = table_aux->deletions.count;
  uint32 dels_count_1 = table_aux->deletions_1.count;
  uint32 dels_count_2 = table_aux->deletions_2.count;

  if (dels_count_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < dels_count_1 ; i++)
      col_update_bit_map_set(&table_aux->full_deletion_map_1, arg1s[i], mem_pool);
  }

  if (dels_count > 0 || dels_count_2 > 0) {
    TRNS_MAP_SURR_U32 remaining;
    trns_map_surr_u32_init(&remaining, table->mem_pool);

    if (dels_count > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < dels_count ; i++) {
        uint64 args = args_array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        uint32 unstable_surr = bin_table_lookup_unstable_surr(table, arg1, arg2);
        if (unstable_surr != 0xFFFFFFFF) {
          if (!col_update_bit_map_check_and_set(&table_aux->bit_map, unstable_surr, mem_pool)) {
            uint32 remn_count = trns_map_surr_u32_lookup(&remaining, arg1, 0);
            if (remn_count == 0)
              remn_count = bin_table_count_1(table, arg1);
            assert(remn_count > 0 && remn_count != 0xFFFFFFFF);
            if (remn_count == 1) {
              // No more references left
              assert(!col_update_bit_map_is_set(&table_aux->full_deletion_map_1, arg1));
              col_update_bit_map_set(&table_aux->full_deletion_map_1, arg1, mem_pool);
#ifndef NDEBUG
              trns_map_surr_u32_set(&remaining, arg1, 0xFFFFFFFF);
#endif
            }
            else
              trns_map_surr_u32_set(&remaining, arg1, remn_count - 1);
          }
        }
      }
    }

    if (dels_count_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < dels_count_2 ; i++) {
        uint32 arg2 = arg2s[i];
        uint32 count2 = bin_table_count_2(table, arg2);
        uint32 read = 0;
        while (read < count2) {
          uint32 buffer[64];
          UINT32_ARRAY array = bin_table_range_restrict_2(table, arg2, read, buffer, 64);
          read += array.size;
          for (uint32 j=0 ; j < array.size ; j++) {
            uint32 arg1 = array.array[j];
            uint32 unstable_surr = bin_table_lookup_unstable_surr(table, arg1, arg2);
            assert(unstable_surr != 0xFFFFFFFF);
            if (!col_update_bit_map_check_and_set(&table_aux->bit_map, unstable_surr, mem_pool)) {
              uint32 remn_count = trns_map_surr_u32_lookup(&remaining, arg1, 0);
              if (remn_count == 0)
                remn_count = bin_table_count_1(table, arg1);
              assert(remn_count > 0 && remn_count != 0xFFFFFFFF);
              if (remn_count == 1) {
                // No more references left
                assert(!col_update_bit_map_is_set(&table_aux->full_deletion_map_1, arg1));
                col_update_bit_map_set(&table_aux->full_deletion_map_1, arg1, mem_pool);
#ifndef NDEBUG
                trns_map_surr_u32_set(&remaining, arg1, 0xFFFFFFFF);
#endif
              }
              else
                trns_map_surr_u32_set(&remaining, arg1, remn_count - 1);
            }
          }
        }
      }
    }

    col_update_bit_map_clear(&table_aux->bit_map);
  }
}

static void bin_table_aux_build_full_deletion_map_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  STATE_MEM_POOL *mem_pool = table->mem_pool;

  uint32 dels_count = table_aux->deletions.count;
  uint32 dels_count_1 = table_aux->deletions_1.count;
  uint32 dels_count_2 = table_aux->deletions_2.count;

  if (dels_count_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < dels_count_2 ; i++)
      col_update_bit_map_set(&table_aux->full_deletion_map_2, arg2s[i], mem_pool);
  }

  if (dels_count > 0 || dels_count_1 > 0) {
    TRNS_MAP_SURR_U32 remaining;
    trns_map_surr_u32_init(&remaining, table->mem_pool);

    if (dels_count > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < dels_count ; i++) {
        uint64 args = args_array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        uint32 unstable_surr = bin_table_lookup_unstable_surr(table, arg1, arg2);
        if (unstable_surr != 0xFFFFFFFF) {
          if (!col_update_bit_map_check_and_set(&table_aux->bit_map, unstable_surr, mem_pool)) {
            uint32 remn_count = trns_map_surr_u32_lookup(&remaining, arg2, 0);
            if (remn_count == 0)
              remn_count = bin_table_count_2(table, arg2);
            assert(remn_count > 0 && remn_count != 0xFFFFFFFF);
            if (remn_count == 1) {
              // No more references left
              assert(!col_update_bit_map_is_set(&table_aux->full_deletion_map_2, arg2));
              col_update_bit_map_set(&table_aux->full_deletion_map_2, arg2, mem_pool);
#ifndef NDEBUG
              trns_map_surr_u32_set(&remaining, arg2, 0xFFFFFFFF);
#endif
            }
            else
              trns_map_surr_u32_set(&remaining, arg2, remn_count - 1);
          }
        }
      }
    }

    if (dels_count_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < dels_count_2 ; i++) {
        uint32 arg2 = arg2s[i];
        uint32 count2 = bin_table_count_2(table, arg2);
        uint32 read = 0;
        while (read < count2) {
          uint32 buffer[64];
          UINT32_ARRAY array = bin_table_range_restrict_2(table, arg2, read, buffer, 64);
          read += array.size;
          for (uint32 j=0 ; j < array.size ; j++) {
            uint32 arg1 = array.array[j];
            uint32 unstable_surr = bin_table_lookup_unstable_surr(table, arg1, arg2);
            assert(unstable_surr != 0xFFFFFFFF);
            if (!col_update_bit_map_check_and_set(&table_aux->bit_map, unstable_surr, mem_pool)) {
              uint32 remn_count = trns_map_surr_u32_lookup(&remaining, arg1, 0);
              if (remn_count == 0)
                remn_count = bin_table_count_1(table, arg1);
              assert(remn_count > 0 && remn_count != 0xFFFFFFFF);
              if (remn_count == 1) {
                // No more references left
                assert(!col_update_bit_map_is_set(&table_aux->full_deletion_map_2, arg1));
                col_update_bit_map_set(&table_aux->full_deletion_map_2, arg1, mem_pool);
#ifndef NDEBUG
                trns_map_surr_u32_set(&remaining, arg1, 0xFFFFFFFF);
#endif
              }
              else
                trns_map_surr_u32_set(&remaining, arg1, remn_count - 1);
            }
          }
        }
      }
    }

    col_update_bit_map_clear(&table_aux->bit_map);
  }
}

////////////////////////////////////////////////////////////////////////////////

static bool bin_table_aux_was_fully_deleted_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1) {
  assert(bin_table_contains_1(table, arg1));

  uint32 dels_count = table_aux->deletions.count;
  uint32 dels_count_1 = table_aux->deletions_1.count;
  uint32 dels_count_2 = table_aux->deletions_2.count;

  //## WOULD IT BE BETTER TO PERFORM THIS CHECK BEFORE CALLING THIS METHOD
  //## AND REPLACE THE CHECK HERE WITH AN ASSERTION?
  if (dels_count == 0 && dels_count_1 == 0 && dels_count_2 == 0)
    return false;

  if (!col_update_bit_map_is_dirty(&table_aux->full_deletion_map_1))
    bin_table_aux_build_full_deletion_map_1(table, table_aux);

  return col_update_bit_map_is_set(&table_aux->full_deletion_map_1, arg1);
}

static bool bin_table_aux_was_fully_deleted_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg2) {
  assert(bin_table_contains_2(table, arg2));

  uint32 dels_count = table_aux->deletions.count;
  uint32 dels_count_1 = table_aux->deletions_1.count;
  uint32 dels_count_2 = table_aux->deletions_2.count;

  //## WOULD IT BE BETTER TO PERFORM THIS CHECK BEFORE CALLING THIS METHOD
  //## AND REPLACE THE CHECK HERE WITH AN ASSERTION?
  if (dels_count == 0 && dels_count_1 == 0 && dels_count_2 == 0)
    return false;

  if (!col_update_bit_map_is_dirty(&table_aux->full_deletion_map_2))
    bin_table_aux_build_full_deletion_map_2(table, table_aux);

  return col_update_bit_map_is_set(&table_aux->full_deletion_map_2, arg2);
}

static bool bin_table_aux_was_inserted_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins == 0)
    return false;

  if (!col_update_bit_map_is_dirty(&table_aux->insertion_map_1)) {
    STATE_MEM_POOL *mem_pool = table->mem_pool;
    uint64 *args_array = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      col_update_bit_map_set(&table_aux->insertion_map_1, unpack_arg1(args_array[i]), mem_pool);
  }

  return col_update_bit_map_is_set(&table_aux->insertion_map_1, arg1);
}

static bool bin_table_aux_was_inserted_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg2) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins == 0)
    return false;

  if (!col_update_bit_map_is_dirty(&table_aux->insertion_map_2)) {
    STATE_MEM_POOL *mem_pool = table->mem_pool;
    uint64 *args_array = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      col_update_bit_map_set(&table_aux->insertion_map_2, unpack_arg2(args_array[i]), mem_pool);
  }

  return col_update_bit_map_is_set(&table_aux->insertion_map_2, arg2);
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_aux_prepare(BIN_TABLE_AUX *table_aux) {

}

bool bin_table_aux_contains_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (bin_table_aux_was_inserted_1(table, table_aux, arg1))
    return true;

  if (table_aux->clear)
    return false;

  if (!bin_table_contains_1(table, arg1))
    return false;

  return !bin_table_aux_was_fully_deleted_1(table, table_aux, arg1);
}

bool bin_table_aux_contains_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg2) {
  if (bin_table_aux_was_inserted_2(table, table_aux, arg2))
    return true;

  if (table_aux->clear)
    return false;

  if (!bin_table_contains_2(table, arg2))
    return false;

  return !bin_table_aux_was_fully_deleted_2(table, table_aux, arg2);
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_aux_check_foreign_key_unary_table_1_forward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint64 *args = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 arg1 = unpack_arg1(args[i]);
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool bin_table_aux_check_foreign_key_unary_table_2_forward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint64 *args = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 arg2 = unpack_arg2(args[i]);
      if (!unary_table_aux_contains(target_table, target_table_aux, arg2)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_aux_check_foreign_key_unary_table_1_backward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint64 *args_array = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = unpack_arg1(args_array[i]);
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, table->mem_pool))
          if (unary_table_aux_contains(src_table, src_table_aux, arg1))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);
      assert(found <= unary_table_aux_size(src_table, src_table_aux));
      bool ok = found == unary_table_aux_size(src_table, src_table_aux);
      if (!ok) {
        //## TODO: RECORD THE ERROR
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
        if (!bin_table_aux_was_inserted_1(table, table_aux, arg1)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 arg1 = unpack_arg1(args_array[i]);
      if (unary_table_aux_contains(src_table, src_table_aux, arg1))
        if (!bin_table_aux_contains_1(table, table_aux, arg1)) {
          //## TODO: RECORD THE ERROR
          return false;
        }
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 count2 = bin_table_count_2(table, arg2);
      uint32 read2 = 0;
      while (read2 < count2) {
        uint32 buffer[64];
        UINT32_ARRAY array2 = bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
        read2 += array2.size;
        for (uint32 i2=0 ; i2 < array2.size ; i2++) {
          uint32 arg1 = array2.array[i2];
          if (unary_table_aux_contains(src_table, src_table_aux, arg1))
            if (!bin_table_aux_contains_1(table, table_aux, arg1)) {
              //## TODO: RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}

bool bin_table_aux_check_foreign_key_master_bin_table_backward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint64 *args_array = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = unpack_arg1(args_array[i]);
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, table->mem_pool))
          if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);
      assert(found <= master_bin_table_aux_size(src_table, src_table_aux));
      bool ok = found == master_bin_table_aux_size(src_table, src_table_aux);
      if (!ok) {
        //## TODO: RECORD THE ERROR
      }
      return ok;
    }
    else {
      bool src_is_empty = master_bin_table_aux_is_empty(src_table, src_table_aux);
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
      if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1)) {
        if (!bin_table_aux_was_inserted_1(table, table_aux, arg1)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 arg1 = unpack_arg1(args_array[i]);
      if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1))
        if (!bin_table_aux_contains_1(table, table_aux, arg1)) {
          //## TODO: RECORD THE ERROR
          return false;
        }
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      uint32 count2 = bin_table_count_2(table, arg2);
      uint32 read2 = 0;
      while (read2 < count2) {
        uint32 buffer[64];
        UINT32_ARRAY array2 = bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
        read2 += array2.size;
        for (uint32 i2=0 ; i2 < array2.size ; i2++) {
          uint32 arg1 = array2.array[i2];
          if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1))
            if (!bin_table_aux_contains_1(table, table_aux, arg1)) {
              //## TODO: RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}

bool bin_table_aux_check_foreign_key_unary_table_2_backward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count > 0) {
      uint64 *args_array = table_aux->insertions.array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg2 = unpack_arg2(args_array[i]);
        if (!col_update_bit_map_check_and_set(&table_aux->bit_map, arg2, table->mem_pool))
          if (unary_table_aux_contains(src_table, src_table_aux, arg2))
            found++;
      }
      col_update_bit_map_clear(&table_aux->bit_map);
      assert(found <= unary_table_aux_size(src_table, src_table_aux));
      bool ok = found == unary_table_aux_size(src_table, src_table_aux);
      if (!ok) {
        //## TODO: RECORD THE ERROR
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

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
        if (!bin_table_aux_was_inserted_2(table, table_aux, arg2)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 arg2 = unpack_arg2(args_array[i]);
      if (unary_table_aux_contains(src_table, src_table_aux, arg2))
        if (!bin_table_aux_contains_2(table, table_aux, arg2)) {
          //## TODO: RECORD THE ERROR
          return false;
        }
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      uint32 count1 = bin_table_count_1(table, arg1);
      uint32 read1 = 0;
      while (read1 < count1) {
        uint32 buffer[64];
        UINT32_ARRAY array = bin_table_range_restrict_1(table, arg1, read1, buffer, 64);
        read1 += array.size;
        for (uint32 i1=0 ; i1 < array.size ; i1++) {
          uint32 arg2 = array.array[i1];
          if (unary_table_aux_contains(src_table, src_table_aux, arg2))
            if (!bin_table_aux_contains_2(table, table_aux, arg2)) {
              //## TODO: RECORD THE ERROR
              return false;
            }
        }
      }
    }
  }

  return true;
}
