#include "lib.h"


void double_key_bin_table_aux_init(DOUBLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_status_map_init(&table_aux->col_1_status_map);
  col_update_status_map_init(&table_aux->col_2_status_map);
  col_update_bit_map_init(&table_aux->bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

void double_key_bin_table_aux_reset(DOUBLE_KEY_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  col_update_status_map_clear(&table_aux->col_1_status_map);
  col_update_status_map_clear(&table_aux->col_2_status_map);
  queue_u64_reset(&table_aux->deletions);
  queue_u64_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void double_key_bin_table_aux_clear(DOUBLE_KEY_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

static void double_key_bin_table_aux_delete_existing(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  assert(double_key_bin_table_contains(table, arg1, arg2));
  assert(col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1) == col_update_status_map_deleted_flag_is_set(&table_aux->col_2_status_map, arg2));

  if (!col_update_status_map_check_and_mark_deletion(&table_aux->col_1_status_map, arg1, table->mem_pool)) {
    col_update_status_map_check_and_mark_deletion(&table_aux->col_1_status_map, arg2, table->mem_pool);
    queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
  }
}

void double_key_bin_table_aux_delete(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (double_key_bin_table_contains(table, arg1, arg2))
    double_key_bin_table_aux_delete_existing(table, table_aux, arg1, arg2);
}

void double_key_bin_table_aux_delete_1(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  uint32 arg2 = double_key_bin_table_lookup_1(table, arg1);
  if (arg2 != 0xFFFFFFFF)
    double_key_bin_table_aux_delete_existing(table, table_aux, arg1, arg2);
}

void double_key_bin_table_aux_delete_2(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  uint32 arg1 = double_key_bin_table_lookup_2(table, arg2);
  if (arg1 != 0xFFFFFFFF)
    double_key_bin_table_aux_delete_existing(table, table_aux, arg1, arg2);
}

void double_key_bin_table_aux_insert(DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  queue_u64_insert(&table_aux->insertions, pack_args(arg1, arg2));
  col_update_status_map_check_and_mark_insertion(&table_aux->col_1_status_map, arg1, mem_pool);
  col_update_status_map_check_and_mark_insertion(&table_aux->col_2_status_map, arg2, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

void double_key_bin_table_aux_apply_deletions(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    uint32 count = double_key_bin_table_size(table);
    if (count > 0) {
      if (table_aux->insertions.count == 0) {
        if (remove1 != NULL)
          remove1(store1, 0xFFFFFFFF, mem_pool);
        if (remove2 != NULL)
          remove2(store2, 0xFFFFFFFF, mem_pool);
      }
      else {
        if (remove1 != NULL | remove2 != NULL) {
          uint32 left = table->count;
          uint32 *forward_array = table->forward_array;
          for (uint32 arg1=0 ; left > 0 ; arg1++) {
            assert(arg1 < table->forward_capacity);
            uint32 arg2 = forward_array[arg1];
            if (arg2 != 0xFFFFFFFF) {
              if (remove1 != NULL && !col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
                remove1(store1, arg1, mem_pool);
              if (remove2 != NULL && !col_update_status_map_inserted_flag_is_set(&table_aux->col_2_status_map, arg2))
                remove2(store2, arg2, mem_pool);
              left--;
            }
          }
        }
      }

      double_key_bin_table_clear(table, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0;
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (double_key_bin_table_delete(table, arg1, arg2)) {
          if (remove1 != NULL && (!has_insertions || !col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1)))
            remove1(store1, arg1, mem_pool);
          if (remove2 != NULL && (!has_insertions || !col_update_status_map_inserted_flag_is_set(&table_aux->col_2_status_map, arg2)))
            remove2(store2, arg2, mem_pool);
        }
      }
    }
  }
}

void double_key_bin_table_aux_apply_insertions(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      double_key_bin_table_insert(table, arg1, arg2, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void double_key_bin_table_aux_record_col_1_key_violation(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, bool between_new) {
  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

static void double_key_bin_table_aux_record_col_2_key_violation(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, bool between_new) {
  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

bool double_key_bin_table_aux_check_keys(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
    uint64 *ins_array = table_aux->insertions.array;

    // Checking for conflicting insertions
    if (ins_count > 1) {
      // Checking for insertions that conflict on the first argument
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg1 = unpack_arg1(args);
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, mem_pool)) {
          //## CHECK FIRST THAT THE CONFLICTING INSERTION DOES NOT HAVE THE SAME VALUE FOR THE SECOND ARGUMENT
          double_key_bin_table_aux_record_col_1_key_violation(table, table_aux, arg1, unpack_arg2(args), true);
          col_update_bit_map_clear(&table_aux->bit_map);
          return false;
        }
      }
      col_update_bit_map_clear(&table_aux->bit_map);

      // Checking for insertions that conflict on the second argument
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg2 = unpack_arg2(args);
        if (col_update_bit_map_check_and_set(&table_aux->bit_map, arg2, mem_pool)) {
          //## CHECK FIRST THAT THE CONFLICTING INSERTION DOES NOT HAVE THE SAME VALUE FOR THE FIRST ARGUMENT
          double_key_bin_table_aux_record_col_1_key_violation(table, table_aux, unpack_arg1(args), arg2, true);
          col_update_bit_map_clear(&table_aux->bit_map);
          return false;
        }
      }
      col_update_bit_map_clear(&table_aux->bit_map);
    }

    if (!table_aux->clear) {
      // Checking for conflicts between the new insertions and the preexisting tuples
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);

        if (double_key_bin_table_contains_1(table, arg1)) {
          if (!col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1)) {
            double_key_bin_table_aux_record_col_1_key_violation(table, table_aux, arg1, arg2, false);
            return false;
          }
        }

        if (double_key_bin_table_contains_2(table, arg2)) {
          if (!col_update_bit_map_is_set(&table_aux->bit_map, arg2)) {
            double_key_bin_table_aux_record_col_1_key_violation(table, table_aux, arg1, arg2, false);
            return false;
          }
        }
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void double_key_bin_table_aux_prepare(DOUBLE_KEY_BIN_TABLE_AUX *table_aux) {
  //## CHECK THAT THERE ARE NO REPEATED DELETIONS
  // queue_u64_sort_unique(&table_aux->deletions); // Needs to support unique_count(..)
  queue_u64_prepare(&table_aux->insertions);
}

bool double_key_bin_table_aux_contains(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint64 args = pack_args(arg1, arg2);

  if (col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
    if (col_update_status_map_inserted_flag_is_set(&table_aux->col_2_status_map, arg2))
      if (queue_u64_contains(&table_aux->insertions, args))
        return true;

  if (table_aux->clear)
    return false;

  if (!double_key_bin_table_contains(table, arg1, arg2))
    return false;

  assert(
    col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1) ==
    col_update_status_map_deleted_flag_is_set(&table_aux->col_2_status_map, arg2)
  );

  if (col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1))
    return false;

  return true;
}

bool double_key_bin_table_aux_contains_1(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
    return true;

  if (table_aux->clear)
    return false;

  if (!double_key_bin_table_contains_1(table, arg1))
    return false;

  if (col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1))
    return false;

  return true;
}

bool double_key_bin_table_aux_contains_2(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  if (col_update_status_map_inserted_flag_is_set(&table_aux->col_2_status_map, arg2))
    return true;

  if (table_aux->clear)
    return false;

  if (!double_key_bin_table_contains_2(table, arg2))
    return false;

  if (col_update_status_map_deleted_flag_is_set(&table_aux->col_2_status_map, arg2))
    return false;

  return true;
}

bool double_key_bin_table_aux_is_empty(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux) {
  if (table_aux->insertions.count > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = double_key_bin_table_size(table);
  if (size == 0)
    return true;

  if (table_aux->deletions.count == size) // Queued deletions are unique
    return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool double_key_bin_table_aux_check_foreign_key_unary_table_1_forward(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
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

bool double_key_bin_table_aux_check_foreign_key_unary_table_2_forward(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
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

bool double_key_bin_table_aux_check_foreign_key_unary_table_1_backward(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  if (table_aux->clear) {
    if (!unary_table_aux_is_empty(src_table, src_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 arg1 = unpack_arg1(args_array[i]);
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
        if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  return true;
}

bool double_key_bin_table_aux_check_foreign_key_unary_table_2_backward(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  if (table_aux->clear) {
    if (!unary_table_aux_is_empty(src_table, src_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint64 *args_array = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 arg2 = unpack_arg2(args_array[i]);
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_2_status_map, arg2))
        if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  return true;
}
