#include "lib.h"


uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *table, uint32 last_idx);
void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *table, uint32 next_free);

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool);

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_init(MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_bit_map_init(&table_aux->bit_map);
  col_update_bit_map_init(&table_aux->another_bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_3u32_init(&table_aux->insertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));

  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_3u32_reset(&table_aux->insertions);
  table_aux->reserved_surrs.clear();
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void master_bin_table_aux_partial_reset(MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map) && !col_update_bit_map_is_dirty(&table_aux->another_bit_map));
  assert(table_aux->deletions.count == 0);
  assert(table_aux->deletions_1.count == 0);
  assert(table_aux->deletions_2.count == 0);
  assert(table_aux->insertions.count_ == 0);
  assert(!table_aux->clear);

  table_aux->reserved_surrs.clear();
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

  if (surr == 0xFFFFFFFF) {
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

    if (surr == 0xFFFFFFFF) {
      unordered_map<uint64, uint32> &reserved_surrs = table_aux->reserved_surrs;
      unordered_map<uint64, uint32>::iterator it = reserved_surrs.find(pack_args(arg1, arg2));
      if (it != reserved_surrs.end()) {
        surr = it->second;
        reserved_surrs.erase(it);
        assert(reserved_surrs.count(pack_args(arg1, arg2)) == 0);
      }
      else {
        surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
        table_aux->last_surr = surr;
      }
    }
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
  uint32 count = table_aux->insertions.count_;
  uint32 (*ptr)[3] = table_aux->insertions.array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 curr_arg1 = (*ptr)[0];
    uint32 curr_arg2 = (*ptr)[1];
    if (curr_arg1 == arg1 & curr_arg2 == arg2)
      return (*ptr)[2];
    ptr++;
  }

  uint64 args = pack_args(arg1, arg2);

  unordered_map<uint64, uint32>::iterator it = table_aux->reserved_surrs.find(pack_args(arg1, arg2));
  if (it != table_aux->reserved_surrs.end())
    return it->second;

  surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
  table_aux->last_surr = surr;
  table_aux->reserved_surrs[args] = surr;
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

static void master_bin_table_aux_build_col_1_insertion_bitmap(MASTER_BIN_TABLE_AUX *table_aux, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(bit_map->num_dirty == 0);

  uint32 count = table_aux->insertions.count_;
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
  assert(bit_map->num_dirty == 0);

  uint32 count = table_aux->insertions.count_;
  if (count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg2 = (*ptr)[1];
      col_update_bit_map_set(bit_map, arg2, mem_pool);
      ptr++;
    }
  }
}

void master_bin_table_aux_apply_deletions(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, STATE_MEM_POOL *mem_pool) {
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

  //## FROM HERE ON THIS IS BASICALLY IDENTICAL TO bin_table_aux_apply_deletions(..)

  if (table_aux->clear) {
    uint32 count = master_bin_table_size(table);
    if (count > 0) {
      if (table_aux->insertions.count_ == 0) {
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

      master_bin_table_clear(table, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count_ > 0;
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

    count = table_aux->deletions_1.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < count ; i++) {
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

        master_bin_table_delete_1(table, arg1);
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

    count = table_aux->deletions_2.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < count ; i++) {
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

        master_bin_table_delete_2(table, arg2);
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

    if (has_insertions && col_1_bit_map_built)
      col_update_bit_map_clear(col_1_bit_map);

    if (has_insertions && col_2_bit_map_built)
      col_update_bit_map_clear(col_2_bit_map);
  }
}

void master_bin_table_aux_apply_insertions(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 ins_count = table_aux->insertions.count_;
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
}

////////////////////////////////////////////////////////////////////////////////

static uint32 master_bin_table_aux_number_of_deletions_1(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT

  // assert(!queue_u32_contains(&table_aux->deletions_1, arg1));

  // unordered_map<uint32, unordered_set<uint32>> deletions;

  // uint32 num_dels = table_aux->deletions.count;
  // if (num_dels > 0) {
  //   uint64 *args_array = table_aux->deletions.array;
  //   for (uint32 i=0 ; i < num_dels ; i++) {
  //     uint64 args = args_array[i];
  //     uint32 arg1 = unpack_arg1(args);
  //     uint32 arg2 = unpack_arg2(args);
  //     assert(master_bin_table_contains(table, arg1, arg2));
  //     deletions[arg1].insert(arg2);
  //   }
  // }

  // uint32 num_dels_2 = table_aux->deletions_2.count;
  // if (num_dels_2 > 0) {
  //   uint32 *arg2s = table_aux->deletions_2.array;
  //   for (uint32 i=0 ; i < num_dels_2 ; i++) {
  //     uint32 arg2 = arg2s[i];
  //     MASTER_BIN_TABLE_ITER_2 iter;
  //     master_bin_table_iter_2_init(table, &iter, arg2);
  //     while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
  //       uint32 arg1 = master_bin_table_iter_2_get_1(&iter);
  //       deletions[arg1].insert(arg2);
  //       master_bin_table_iter_2_move_forward(&iter);
  //     }
  //   }
  // }

  // return deletions[arg1].size();
}

static uint32 master_bin_table_aux_number_of_deletions_2(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT

  // assert(!queue_u32_contains(&table_aux->deletions_2, arg2));

  // unordered_map<uint32, unordered_set<uint32>> deletions;

  // uint32 num_dels = table_aux->deletions.count;
  // if (num_dels > 0) {
  //   uint64 *args_array = table_aux->deletions.array;
  //   for (uint32 i=0 ; i < num_dels ; i++) {
  //     uint64 args = args_array[i];
  //     uint32 arg1 = unpack_arg1(args);
  //     uint32 arg2 = unpack_arg2(args);
  //     assert(master_bin_table_contains(table, arg1, arg2));
  //     deletions[arg2].insert(arg1);
  //   }
  // }

  // uint32 num_dels_1 = table_aux->deletions_1.count;
  // if (num_dels_1 > 0) {
  //   uint32 *arg1s = table_aux->deletions_1.array;
  //   for (uint32 i=0 ; i < num_dels_1 ; i++) {
  //     uint32 arg1 = arg1s[i];
  //     MASTER_BIN_TABLE_ITER_1 iter;
  //     master_bin_table_iter_1_init(table, &iter, arg1);
  //     while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
  //       uint32 arg2 = master_bin_table_iter_1_get_1(&iter);
  //       deletions[arg2].insert(arg1);
  //       master_bin_table_iter_1_move_forward(&iter);
  //     }
  //   }
  // }

  // return deletions[arg2].size();
}

static uint32 master_bin_table_aux_number_of_deletions(MASTER_BIN_TABLE *table, QUEUE_U64 *deletions, QUEUE_U32 *deletions_1, QUEUE_U32 *deletions_2) {
  unordered_set<uint64> unique_deletions;

  uint32 num_dels = deletions->count;
  if (num_dels > 0) {
    uint64 *args_array = deletions->array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(master_bin_table_contains(table, arg1, arg2));
      unique_deletions.insert(args);
    }
  }

  uint32 num_dels_1 = deletions_1->count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = deletions_1->array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];

      uint32 count1 = master_bin_table_count_1(table, arg1);
      uint32 read = 0;
      while (read < count1) {
        uint32 buffer[64];
        UINT32_ARRAY array1 = master_bin_table_range_restrict_1(table, arg1, read, buffer, 64);
        read += array1.size;
        for (uint32 j=0 ; j < array1.size ; j++) {
          uint32 arg2 = array1.array[j];

          unique_deletions.insert(pack_args(arg1, arg2));

        }
      }
    }
  }

  uint32 num_dels_2 = deletions_2->count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = deletions_2->array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];

      uint32 count2 = master_bin_table_count_2(table, arg2);
      uint32 read = 0;
      while (read < count2) {
        uint32 buffer[64];
        UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
        read += array2.size;
        for (uint32 i=0 ; i < array2.size ; i++) {
          uint32 arg1 = array2.array[i];

          unique_deletions.insert(pack_args(arg1, arg2));

        }
      }
    }
  }

  return unique_deletions.size();
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_aux_prepare(MASTER_BIN_TABLE_AUX *table_aux) {
  queue_u64_sort_unique(&table_aux->deletions); // Needs to support unique_count(..)
  queue_u32_prepare(&table_aux->deletions_1);
  queue_u32_prepare(&table_aux->deletions_2);
  queue_3u32_prepare(&table_aux->insertions);
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
  if (table_aux->insertions.count_ > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = master_bin_table_size(table);
  if (size == 0)
    return true;

  uint32 num_dels = queue_u64_unique_count(&table_aux->deletions);
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels > 0) {
    if (num_dels_1 > 0) {
      if (num_dels_2 > 0) {
        // NZ NZ NZ
        return master_bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
      }
      else {
        // NZ NZ Z
        return master_bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
      }
    }
    else {
      if (num_dels_2 > 0) {
        // NZ Z NZ
        return master_bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
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
        return master_bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
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
  uint32 num_ins = table_aux->insertions.count_;
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
  uint32 num_ins = table_aux->insertions.count_;
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
  uint32 num_ins = table_aux->insertions.count_;
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
  uint32 num_ins = table_aux->insertions.count_;
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
  uint32 num_ins = table_aux->insertions.count_;
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
  uint32 num_ins = table_aux->insertions.count_;
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
  if (table_aux->clear) {
    if (!unary_table_aux_is_empty(src_table, src_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
        if (!master_bin_table_aux_contains_1(table, table_aux, arg1)) { //## NOT THE MOST EFFICIENT WAY TO DO IT. SHOULD ONLY CHECK INSERTIONS/REINSERTIONS
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels > 0 | num_dels_2 > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL

    unordered_map<uint32, unordered_set<uint32>> deleted;
    unordered_set<uint32> inserted;

    if (num_dels > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint64 args = args_array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (master_bin_table_contains(table, arg1, arg2))
          deleted[arg1].insert(arg2);
      }
    }

    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];

        uint32 count2 = master_bin_table_count_2(table, arg2);
        uint32 read = 0;
        while (read < count2) {
          uint32 buffer[64];
          UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
          read += array2.size;
          for (uint32 j=0 ; j < array2.size ; j++) {
            uint32 arg1 = array2.array[j];

            deleted[arg1].insert(arg2);

          }
        }
      }
    }

    uint32 num_ins = table_aux->insertions.count_;
    if (num_ins > 0) {
      uint32 (*insertions)[3] = table_aux->insertions.array;
      for (uint32 i=0 ; i < num_ins ; i++) {
        uint32 arg1 = insertions[i][0];
        inserted.insert(arg1);
      }
    }

    for (unordered_map<uint32, unordered_set<uint32>>::iterator it = deleted.begin() ; it != deleted.end() ; it++) {
      uint32 arg1 = it->first;
      uint32 num_del = it->second.size();
      uint32 curr_num = master_bin_table_count_1(table, arg1);
      assert(num_del <= curr_num);
      if (num_del == curr_num && inserted.count(arg1) == 0) {
        if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_unary_table_2_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  if (table_aux->clear) {
    if (!unary_table_aux_is_empty(src_table, src_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
        if (!master_bin_table_aux_contains_2(table, table_aux, arg2)) { //## NOT THE MOST EFFICIENT WAY TO DO IT. SHOULD ONLY CHECK INSERTIONS/REINSERTIONS
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;

  if (num_dels > 0 | num_dels_1 > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL

    unordered_map<uint32, unordered_set<uint32>> deleted;
    unordered_set<uint32> inserted;

    if (num_dels > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint64 args = args_array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (master_bin_table_contains(table, arg1, arg2))
          deleted[arg2].insert(arg1);
      }
    }

    if (num_dels_1 > 0) {
      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels_1 ; i++) {
        uint32 arg1 = arg1s[i];

        uint32 count1 = master_bin_table_count_1(table, arg1);
        uint32 read = 0;
        while (read < count1) {
          uint32 buffer[64];
          UINT32_ARRAY array1 = master_bin_table_range_restrict_1(table, arg1, read, buffer, 64);
          read += array1.size;
          for (uint32 j=0 ; j < array1.size ; j++) {
            uint32 arg2 = array1.array[j];

            deleted[arg2].insert(arg1);

          }
        }
      }
    }

    uint32 num_ins = table_aux->insertions.count_;
    if (num_ins > 0) {
      uint32 (*insertions)[3] = table_aux->insertions.array;
      for (uint32 i=0 ; i < num_ins ; i++) {
        uint32 arg2 = insertions[i][1];
        inserted.insert(arg2);
      }
    }

    for (unordered_map<uint32, unordered_set<uint32>>::iterator it = deleted.begin() ; it != deleted.end() ; it++) {
      uint32 arg2 = it->first;
      uint32 num_del = it->second.size();
      uint32 curr_num = master_bin_table_count_2(table, arg2);
      assert(num_del <= curr_num);
      if (num_del == curr_num && inserted.count(arg2) == 0) {
        if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_slave_tern_table_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, BIN_TABLE *src_table, SLAVE_TERN_TABLE_AUX *src_table_aux) {
  if (table_aux->clear) {
    if (!bin_table_aux_is_empty(src_table, &src_table_aux->slave_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      if (surr != 0xFFFFFFFF)
        if (bin_table_aux_contains_1(src_table, &src_table_aux->slave_table_aux, surr)) {
          if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
            //## RECORD THE ERROR
            return false;
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
        //## HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array1 = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array1.size;
        uint32 *surrs = array1.array + array1.offset;
        for (uint32 j=0 ; j < array1.size ; j++) {
          uint32 surr = surrs[j];

          if (bin_table_aux_contains_1(src_table, &src_table_aux->slave_table_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
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
        uint32 buffer[64];
        UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
        read += array2.size;
        for (uint32 j=0 ; j < array2.size ; j++) {
          uint32 arg1 = array2.array[j];
          uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);

          if (bin_table_aux_contains_1(src_table, &src_table_aux->slave_table_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
          }

        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_obj_col_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, OBJ_COL *src_col, OBJ_COL_AUX *src_col_aux) {
  if (table_aux->clear) {
    if (!obj_col_aux_is_empty(src_col, src_col_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      if (surr != 0xFFFFFFFF)
        if (obj_col_aux_contains_1(src_col, src_col_aux, surr)) {
          if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
            //## RECORD THE ERROR
            return false;
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
        //## HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array1 = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array1.size;
        uint32 *surrs = array1.array + array1.offset;
        for (uint32 j=0 ; j < array1.size ; j++) {
          uint32 surr = surrs[j];

          if (obj_col_aux_contains_1(src_col, src_col_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
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
        uint32 buffer[64];
        UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
        read += array2.size;
        for (uint32 j=0 ; j < array2.size ; j++) {
          uint32 arg1 = array2.array[j];
          uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);

          if (obj_col_aux_contains_1(src_col, src_col_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
          }

        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_int_col_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, INT_COL *src_col, INT_COL_AUX *src_col_aux) {
  if (table_aux->clear) {
    if (!int_col_aux_is_empty(src_col, src_col_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      if (surr != 0xFFFFFFFF)
        if (int_col_aux_contains_1(src_col, src_col_aux, surr)) {
          if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
            //## RECORD THE ERROR
            return false;
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
        //## HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array1 = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array1.size;
        uint32 *surrs = array1.array + array1.offset;
        for (uint32 j=0 ; j < array1.size ; j++) {
          uint32 surr = surrs[j];

          if (int_col_aux_contains_1(src_col, src_col_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
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
        uint32 buffer[64];
        UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
        read += array2.size;
        for (uint32 j=0 ; j < array2.size ; j++) {
          uint32 arg1 = array2.array[j];
          uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);

          if (int_col_aux_contains_1(src_col, src_col_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
          }

        }
      }
    }
  }

  return true;
}

bool master_bin_table_aux_check_foreign_key_float_col_backward(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, FLOAT_COL *src_col, FLOAT_COL_AUX *src_col_aux) {
  if (table_aux->clear) {
    if (!float_col_aux_is_empty(src_col, src_col_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_1 == 0 & num_dels_2 == 0)
    return true;

  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
      if (surr != 0xFFFFFFFF)
        if (float_col_aux_contains_1(src_col, src_col_aux, surr)) {
          if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
            //## RECORD THE ERROR
            return false;
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
        //## HERE I ONLY NEED THE SURROGATES, NOT THE SECOND ARGUMENTS
        UINT32_ARRAY array1 = master_bin_table_range_restrict_1_with_surrs(table, arg1, read, buffer, 64);
        read += array1.size;
        uint32 *surrs = array1.array + array1.offset;
        for (uint32 j=0 ; j < array1.size ; j++) {
          uint32 surr = surrs[j];

          if (float_col_aux_contains_1(src_col, src_col_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
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
        uint32 buffer[64];
        UINT32_ARRAY array2 = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
        read += array2.size;
        for (uint32 j=0 ; j < array2.size ; j++) {
          uint32 arg1 = array2.array[j];
          uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);

          if (float_col_aux_contains_1(src_col, src_col_aux, surr)) {
            if (!queue_3u32_contains_3(&table_aux->insertions, surr)) {
              //## RECORD THE ERROR
              return false;
            }
          }

        }
      }
    }
  }

  return true;
}
