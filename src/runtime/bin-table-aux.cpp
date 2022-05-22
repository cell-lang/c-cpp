#include "lib.h"


void bin_table_aux_init(BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_bit_map_init(&table_aux->bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

void bin_table_aux_reset(BIN_TABLE_AUX *table_aux) {
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

void bin_table_aux_apply(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    BIN_TABLE_ITER iter;
    bin_table_iter_init(table, &iter);
    while (!bin_table_iter_is_out_of_range(&iter)) {
      uint32 arg1 = bin_table_iter_get_1(&iter);
      uint32 arg2 = bin_table_iter_get_2(&iter);
      decr_rc_1(store_1, store_aux_1, arg1);
      decr_rc_2(store_2, store_aux_2, arg2);
      bin_table_iter_move_forward(&iter);
    }
    bin_table_clear(table, mem_pool);
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (bin_table_delete(table, arg1, arg2)) {
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
        BIN_TABLE_ITER_1 iter;
        bin_table_iter_1_init(table, &iter, arg1);
        while (!bin_table_iter_1_is_out_of_range(&iter)) {
          uint32 arg2 = bin_table_iter_1_get_1(&iter);
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
          bin_table_iter_1_move_forward(&iter);
        }
        bin_table_delete_1(table, arg1);
      }
    }

    count = table_aux->deletions_2.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg2 = array[i];
        BIN_TABLE_ITER_2 iter;
        bin_table_iter_2_init(table, &iter, arg2);
        while (!bin_table_iter_2_is_out_of_range(&iter)) {
          uint32 arg1 = bin_table_iter_2_get_1(&iter);
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
          bin_table_iter_2_move_forward(&iter);
        }
        bin_table_delete_2(table, arg2);
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
      if (bin_table_insert(table, arg1, arg2, mem_pool)) {
        incr_rc_1(store_1, arg1);
        incr_rc_2(store_2, arg2);
      }
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
      if (bin_table_contains_2(table, arg2)) {
        BIN_TABLE_ITER_2 iter;
        bin_table_iter_2_init(table, &iter, arg2);
        assert(!bin_table_iter_2_is_out_of_range(&iter));
        do {
          uint32 arg1 = bin_table_iter_2_get_1(&iter);
          col_update_bit_map_set(bit_map, arg1, mem_pool);
        } while (!bin_table_iter_2_is_out_of_range(&iter));
      }
    }
  }
}

inline void bin_table_aux_build_col_2_del_bitmap(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
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
      if (bin_table_contains_1(table, arg1)) {
        BIN_TABLE_ITER_1 iter;
        bin_table_iter_1_init(table, &iter, arg1);
        assert(!bin_table_iter_1_is_out_of_range(&iter));
        do {
          uint32 arg2 = bin_table_iter_1_get_1(&iter);
          col_update_bit_map_set(bit_map, arg2, mem_pool);
        } while (!bin_table_iter_1_is_out_of_range(&iter));
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

// SO FAR THIS IS ONLY USED BY tern_table_aux_check_key_13(..) AND tern_table_aux_check_key_23(..),
// SO IT'S NOT SUPER IMPORTANT FOR PERFORMANCE.
bool bin_table_aux_was_deleted(BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (table_aux->clear)
    return true;

  //## IMPLEMENT FOR REAL

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

static uint32 bin_table_aux_number_of_deletions_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1) {
  assert(!queue_u32_contains(&table_aux->deletions_1, arg1));

  unordered_map<uint32, unordered_set<uint32>> deletions;

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(bin_table_contains(table, arg1, arg2));
      deletions[arg1].insert(arg2);
    }
  }

  uint32 num_dels_2 = table_aux->deletions_2.count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = table_aux->deletions_2.array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      BIN_TABLE_ITER_2 iter;
      bin_table_iter_2_init(table, &iter, arg2);
      while (!bin_table_iter_2_is_out_of_range(&iter)) {
        uint32 arg1 = bin_table_iter_2_get_1(&iter);
        deletions[arg1].insert(arg2);
        bin_table_iter_2_move_forward(&iter);
      }
    }
  }

  return deletions[arg1].size();
}

static uint32 bin_table_aux_number_of_deletions_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg2) {
  assert(!queue_u32_contains(&table_aux->deletions_2, arg2));

  unordered_map<uint32, unordered_set<uint32>> deletions;

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(bin_table_contains(table, arg1, arg2));
      deletions[arg2].insert(arg1);
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      BIN_TABLE_ITER_1 iter;
      bin_table_iter_1_init(table, &iter, arg1);
      while (!bin_table_iter_1_is_out_of_range(&iter)) {
        uint32 arg2 = bin_table_iter_1_get_1(&iter);
        deletions[arg2].insert(arg1);
        bin_table_iter_1_move_forward(&iter);
      }
    }
  }

  return deletions[arg2].size();
}

static uint32 bin_table_aux_number_of_deletions(BIN_TABLE *table, QUEUE_U64 *deletions, QUEUE_U32 *deletions_1, QUEUE_U32 *deletions_2) {
  unordered_set<uint64> unique_deletions;

  uint32 num_dels = deletions->count;
  if (num_dels > 0) {
    uint64 *args_array = deletions->array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(bin_table_contains(table, arg1, arg2));
      unique_deletions.insert(args);
    }
  }

  uint32 num_dels_1 = deletions_1->count;
  if (num_dels_1 > 0) {
    uint32 *arg1s = deletions_1->array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 arg1 = arg1s[i];
      BIN_TABLE_ITER_1 iter;
      bin_table_iter_1_init(table, &iter, arg1);
      while (!bin_table_iter_1_is_out_of_range(&iter)) {
        uint32 arg2 = bin_table_iter_1_get_1(&iter);
        unique_deletions.insert(pack_args(arg1, arg2));
        bin_table_iter_1_move_forward(&iter);
      }
    }
  }

  uint32 num_dels_2 = deletions_2->count;
  if (num_dels_2 > 0) {
    uint32 *arg2s = deletions_2->array;
    for (uint32 i=0 ; i < num_dels_2 ; i++) {
      uint32 arg2 = arg2s[i];
      BIN_TABLE_ITER_2 iter;
      bin_table_iter_2_init(table, &iter, arg2);
      while (!bin_table_iter_2_is_out_of_range(&iter)) {
        uint32 arg1 = bin_table_iter_2_get_1(&iter);
        unique_deletions.insert(pack_args(arg1, arg2));
        bin_table_iter_2_move_forward(&iter);
      }
    }
  }

  return unique_deletions.size();
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_aux_prepare(BIN_TABLE_AUX *table_aux) {
  queue_u64_sort_unique(&table_aux->deletions); // Needs to support unique_count(..)
  queue_u32_prepare(&table_aux->deletions_1);
  queue_u32_prepare(&table_aux->deletions_2);
  queue_u64_prepare(&table_aux->insertions);
}

bool bin_table_aux_contains(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 args = pack_args(arg1, arg2);

  if (queue_u64_contains(&table_aux->insertions, args))
    return true;

  if (table_aux->clear)
    return false;

  if (!bin_table_contains(table, arg1, arg2))
    return false;

  if (queue_u32_contains(&table_aux->deletions_1, arg1))
    return false;

  if (queue_u32_contains(&table_aux->deletions_2, arg2))
    return false;

  if (queue_u64_contains(&table_aux->deletions, args))
    return false;

  return true;
}

bool bin_table_aux_contains_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (queue_u64_contains_1(&table_aux->insertions, arg1))
    return true;

  if (table_aux->clear)
    return false;

  if (!bin_table_contains_1(table, arg1))
    return false;

  if (queue_u32_contains(&table_aux->deletions_1, arg1))
    return false;

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels == 0 & num_dels_2 == 0)
    return true;

  return bin_table_aux_number_of_deletions_1(table, table_aux, arg1) < bin_table_count_1(table, arg1);
}

bool bin_table_aux_contains_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg2) {
  if (queue_u64_contains_2(&table_aux->insertions, arg2))
    return true;

  if (table_aux->clear)
    return false;

  if (!bin_table_contains_2(table, arg2))
    return false;

  if (queue_u32_contains(&table_aux->deletions_2, arg2))
    return false;

  uint32 num_dels = table_aux->deletions.count;
  uint32 num_dels_1 = table_aux->deletions_1.count;

  if (num_dels == 0 & num_dels_1 == 0)
    return true;

  return bin_table_aux_number_of_deletions_2(table, table_aux, arg2) < bin_table_count_2(table, arg2);
}

bool bin_table_aux_is_empty(BIN_TABLE *table, BIN_TABLE_AUX *table_aux) {
  if (table_aux->insertions.count > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = bin_table_size(table);
  if (size == 0)
    return true;

  uint32 num_dels = queue_u64_unique_count(&table_aux->deletions);
  uint32 num_dels_1 = table_aux->deletions_1.count;
  uint32 num_dels_2 = table_aux->deletions_2.count;

  if (num_dels > 0) {
    if (num_dels_1 > 0) {
      if (num_dels_2 > 0) {
        // NZ NZ NZ
        return bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
      }
      else {
        // NZ NZ Z
        return bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
      }
    }
    else {
      if (num_dels_2 > 0) {
        // NZ Z NZ
        return bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
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
        return bin_table_aux_number_of_deletions(table, &table_aux->deletions, &table_aux->deletions_1, &table_aux->deletions_2) == size;
      }
      else {
        // Z NZ Z
        uint32 total_num_dels = 0;
        uint32 *arg1s = table_aux->deletions_1.array;
        for (uint32 i=0 ; i < num_dels_1 ; i++)
          total_num_dels += bin_table_count_1(table, arg1s[i]);
        return total_num_dels == size;
      }
    }
    else {
      if (num_dels_2 > 0) {
        // Z Z NZ
        uint32 total_num_dels = 0;
        uint32 *arg2s = table_aux->deletions_2.array;
        for (uint32 i=0 ; i < num_dels_2 ; i++)
          total_num_dels += bin_table_count_2(table, arg2s[i]);
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
        if (!bin_table_aux_contains_1(table, table_aux, arg1)) { //## NOT THE MOST EFFICIENT WAY TO DO IT. SHOULD ONLY CHECK INSERTIONS
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
        if (bin_table_contains(table, arg1, arg2))
          deleted[arg1].insert(arg2);
      }
    }

    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];
        if (bin_table_contains_2(table, arg2)) {
          BIN_TABLE_ITER_2 iter;
          bin_table_iter_2_init(table, &iter, arg2);
          while (!bin_table_iter_2_is_out_of_range(&iter)) {
            uint32 arg1 = bin_table_iter_2_get_1(&iter);
            deleted[arg1].insert(arg2);
            bin_table_iter_2_move_forward(&iter);
          }
        }
      }
    }

    uint32 num_ins = table_aux->insertions.count / 3;
    if (num_ins > 0) {
      uint64 *args_array = table_aux->insertions.array;
      for (uint32 i=0 ; i < num_ins ; i++) {
        uint32 arg1 = unpack_arg1(args_array[i]);
        inserted.insert(arg1);
      }
    }

    for (unordered_map<uint32, unordered_set<uint32>>::iterator it = deleted.begin() ; it != deleted.end() ; it++) {
      uint32 arg1 = it->first;
      uint32 num_del = it->second.size();
      uint32 curr_num = bin_table_count_1(table, arg1);
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

bool bin_table_aux_check_foreign_key_master_bin_table_backward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  if (table_aux->clear) {
    if (!master_bin_table_aux_is_empty(src_table, src_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels_1 = table_aux->deletions_1.count;
  if (num_dels_1 > 0) {
    uint32 *surrs = table_aux->deletions_1.array;
    for (uint32 i=0 ; i < num_dels_1 ; i++) {
      uint32 surr = surrs[i];
      if (master_bin_table_aux_contains_surr(src_table, src_table_aux, surr)) {
        if (!bin_table_aux_contains_1(table, table_aux, surr)) { //## NOT THE MOST EFFICIENT WAY TO DO IT. SHOULD ONLY CHECK INSERTIONS
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
        if (bin_table_contains(table, arg1, arg2))
          deleted[arg1].insert(arg2);
      }
    }

    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];
        if (bin_table_contains_2(table, arg2)) {
          BIN_TABLE_ITER_2 iter;
          bin_table_iter_2_init(table, &iter, arg2);
          while (!bin_table_iter_2_is_out_of_range(&iter)) {
            uint32 arg1 = bin_table_iter_2_get_1(&iter);
            deleted[arg1].insert(arg2);
            bin_table_iter_2_move_forward(&iter);
          }
        }
      }
    }

    uint32 num_ins = table_aux->insertions.count / 3;
    if (num_ins > 0) {
      uint64 *args_array = table_aux->insertions.array;
      for (uint32 i=0 ; i < num_ins ; i++) {
        uint32 arg1 = unpack_arg1(args_array[i]);
        inserted.insert(arg1);
      }
    }

    for (unordered_map<uint32, unordered_set<uint32>>::iterator it = deleted.begin() ; it != deleted.end() ; it++) {
      uint32 surr = it->first;
      uint32 num_del = it->second.size();
      uint32 curr_num = bin_table_count_1(table, surr);
      assert(num_del <= curr_num);
      if (num_del == curr_num && inserted.count(surr) == 0) {
        if (master_bin_table_aux_contains_surr(src_table, src_table_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  return true;
}

bool bin_table_aux_check_foreign_key_unary_table_2_backward(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
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
        if (!bin_table_aux_contains_2(table, table_aux, arg2)) { //## NOT THE MOST EFFICIENT WAY TO DO IT. SHOULD ONLY CHECK INSERTIONS
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
        if (bin_table_contains(table, arg1, arg2))
          deleted[arg2].insert(arg1);
      }
    }

    if (num_dels_1 > 0) {
      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels_1 ; i++) {
        uint32 arg1 = arg1s[i];
        if (bin_table_contains_1(table, arg1)) {
          BIN_TABLE_ITER_1 iter;
          bin_table_iter_1_init(table, &iter, arg1);
          while (!bin_table_iter_1_is_out_of_range(&iter)) {
            uint32 arg2 = bin_table_iter_1_get_1(&iter);
            deleted[arg2].insert(arg1);
            bin_table_iter_1_move_forward(&iter);
          }
        }
      }
    }

    uint32 num_ins = table_aux->insertions.count / 3;
    if (num_ins > 0) {
      uint64 *args_array = table_aux->insertions.array;
      for (uint32 i=0 ; i < num_ins ; i++) {
        uint32 arg2 = unpack_arg2(args_array[i]);
        inserted.insert(arg2);
      }
    }

    for (unordered_map<uint32, unordered_set<uint32>>::iterator it = deleted.begin() ; it != deleted.end() ; it++) {
      uint32 arg2 = it->first;
      uint32 num_del = it->second.size();
      uint32 curr_num = bin_table_count_2(table, arg2);
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
