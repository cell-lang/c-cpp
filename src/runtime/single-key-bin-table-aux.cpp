#include "lib.h"


static void single_key_bin_table_aux_record_col_1_key_violation(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, bool between_new) {
  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_aux_init(SINGLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  col_update_status_map_init(&table_aux->col_1_status_map);
  col_update_bit_map_init(&table_aux->arg2_insertion_map);
  col_update_bit_map_init(&table_aux->arg2_deletion_map);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_u64_init(&table_aux->insertions);
  table_aux->unique_deletes_count = 0;
  table_aux->key_violation_detected = false;
  table_aux->clear = false;
}

void single_key_bin_table_aux_reset(SINGLE_KEY_BIN_TABLE_AUX *table_aux) {
  col_update_status_map_clear(&table_aux->col_1_status_map);
  col_update_bit_map_clear(&table_aux->arg2_insertion_map);
  col_update_bit_map_clear(&table_aux->arg2_deletion_map);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_u64_reset(&table_aux->insertions);
  table_aux->unique_deletes_count = 0;
  table_aux->key_violation_detected = false;
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_aux_clear(SINGLE_KEY_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void single_key_bin_table_aux_delete(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (single_key_bin_table_contains(table, arg1, arg2))
    if (!col_update_status_map_check_and_mark_deletion(&table_aux->col_1_status_map, arg1, table->mem_pool)) {
      queue_u32_insert(&table_aux->deletions_1, arg1);
      table_aux->unique_deletes_count++;
    }
}

void single_key_bin_table_aux_delete_1(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (single_key_bin_table_contains_1(table, arg1))
    if (!col_update_status_map_check_and_mark_deletion(&table_aux->col_1_status_map, arg1, table->mem_pool)) {
      queue_u32_insert(&table_aux->deletions_1, arg1);
      table_aux->unique_deletes_count++;
    }
}

void single_key_bin_table_aux_delete_2(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  uint32 count2 = single_key_bin_table_count_2(table, arg2);
  if (count2 > 0) {
    if (!col_update_bit_map_check_and_set(&table_aux->arg2_deletion_map, arg2, table->mem_pool)) {
      uint32 read2 = 0;
      do {
        uint32 buffer[64];
        UINT32_ARRAY array2 = single_key_bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
        read2 += array2.size;
        for (uint32 i2=0 ; i2 < array2.size ; i2++) {
          uint32 arg1 = array2.array[i2];
          if (!col_update_status_map_check_and_mark_deletion(&table_aux->col_1_status_map, arg1, table->mem_pool))
            table_aux->unique_deletes_count++;
        }
      } while (read2 < count2);

      queue_u32_insert(&table_aux->deletions_2, arg2);
    }
  }
}

void single_key_bin_table_aux_insert(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  if (col_update_status_map_check_and_mark_insertion(&table_aux->col_1_status_map, arg1, mem_pool)) {
    //## CHECK FIRST THAT THE CONFLICTING INSERTION DOES NOT HAVE THE SAME VALUE FOR THE SECOND ARGUMENT
    single_key_bin_table_aux_record_col_1_key_violation(table, table_aux, arg1, arg2, true);
    table_aux->key_violation_detected = true;
  }
  else
    queue_u64_insert(&table_aux->insertions, pack_args(arg1, arg2));
}

////////////////////////////////////////////////////////////////////////////////

inline bool single_key_bin_table_aux_arg2_insertion_map_has_been_built(SINGLE_KEY_BIN_TABLE_AUX *table_aux) {
  return table_aux->insertions.count == 0 || col_update_bit_map_is_dirty(&table_aux->arg2_insertion_map);
}

inline bool single_key_bin_table_aux_arg2_was_inserted(SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  assert(single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux));

  return col_update_bit_map_is_set(&table_aux->arg2_insertion_map, arg2);
}

static void single_key_bin_table_aux_build_col_2_insertion_bitmap(SINGLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(!single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *args_array = table_aux->insertions.array;
    COL_UPDATE_BIT_MAP *bit_map = &table_aux->arg2_insertion_map;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg2 = unpack_arg2(args_array[i]);
      col_update_bit_map_set(bit_map, arg2, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_aux_apply_deletions(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    uint32 count = single_key_bin_table_size(table);
    if (count > 0) {
      if (table_aux->insertions.count == 0) {
        if (remove1 != NULL)
          remove1(store1, 0xFFFFFFFF, mem_pool);
        if (remove2 != NULL)
          remove2(store2, 0xFFFFFFFF, mem_pool);
      }
      else {
        if (remove1 != NULL) {
          uint32 read = 0;
          for (uint32 arg1=0 ; read < count ; arg1++) {
            if (single_key_bin_table_contains_1(table, arg1)) {
              read++;
              if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
                remove1(store1, arg1, mem_pool);
            }
          }
        }

        if (remove2 != NULL) {
          single_key_bin_table_aux_build_col_2_insertion_bitmap(table_aux, mem_pool);
          uint32 read = 0;
          for (uint32 arg2=0 ; read < count ; arg2++) {
            uint32 count2 = single_key_bin_table_count_2(table, arg2);
            if (count2 > 0) {
              read += count2;
              if (!single_key_bin_table_aux_arg2_was_inserted(table_aux, arg2))
                remove2(store2, arg2, mem_pool);
            }
          }
        }
      }

      single_key_bin_table_clear(table, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0;

    uint32 count = table_aux->deletions_2.count;
    if (count > 0) {
      if (remove2 != NULL && !single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux))
        single_key_bin_table_aux_build_col_2_insertion_bitmap(table_aux, mem_pool);

      uint32 *array = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg2 = array[i];
        single_key_bin_table_delete_2(table, arg2);
        assert(single_key_bin_table_count_2(table, arg2) == 0);
        if (remove2 != NULL)
          if (!has_insertions || !single_key_bin_table_aux_arg2_was_inserted(table_aux, arg2))
            remove2(store2, arg2, mem_pool);
      }
    }

    count = table_aux->deletions_1.count;
    if (count > 0) {
      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg1 = arg1s[i];
        assert(single_key_bin_table_contains_1(table, arg1));
        uint32 arg2 = single_key_bin_table_lookup_1(table, arg1);
        assert(arg2 != 0xFFFFFFFF);
        single_key_bin_table_delete_1(table, arg1);
        assert(!single_key_bin_table_contains_1(table, arg1));
        assert(single_key_bin_table_count_1(table, arg1) == 0);
        if (remove1 != NULL) {
          if (!has_insertions || !col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
            remove1(store1, arg1, mem_pool);
        }
        if (remove2 != NULL && !single_key_bin_table_contains_2(table, arg2)) {
          if (!single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux))
            single_key_bin_table_aux_build_col_2_insertion_bitmap(table_aux, mem_pool);

          if (!has_insertions || !single_key_bin_table_aux_arg2_was_inserted(table_aux, arg2))
            remove2(store2, arg2, mem_pool);
        }
      }
    }
  }
}

void single_key_bin_table_aux_apply_insertions(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      single_key_bin_table_insert(table, arg1, arg2, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool single_key_bin_table_aux_check_key_1(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
    uint64 *ins_array = table_aux->insertions.array;

    // // Checking for conflicting insertions
    // if (ins_count > 1) {
    //   for (uint32 i=0 ; i < ins_count ; i++) {
    //     uint64 args = ins_array[i];
    //     uint32 arg1 = unpack_arg1(args);
    //     if (col_update_bit_map_check_and_set(&table_aux->bit_map, arg1, mem_pool)) {
    //       //## CHECK FIRST THAT THE CONFLICTING INSERTION DOES NOT HAVE THE SAME VALUE FOR THE SECOND ARGUMENT
    //       single_key_bin_table_aux_record_col_1_key_violation(table, table_aux, arg1, unpack_arg2(args), true);
    //       col_update_bit_map_clear(&table_aux->bit_map);
    //       return false;
    //     }
    //   }
    //   col_update_bit_map_clear(&table_aux->bit_map);
    // }

    // The above check is already being done in single_key_bin_table_aux_insert(..)
    if (table_aux->key_violation_detected)
      return false;

    if (!table_aux->clear) {
      // Checking for conflicts between the new insertions and the preexisting tuples
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint64 args = ins_array[i];
        uint32 arg1 = unpack_arg1(args);
        if (single_key_bin_table_contains_1(table, arg1)) {
          if (!col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1)) {
            uint32 arg2 = unpack_arg2(args);
            if (single_key_bin_table_lookup_1(table, arg1) != arg2) {
              single_key_bin_table_aux_record_col_1_key_violation(table, table_aux, arg1, arg2, false);
              return false;
            }
          }
        }
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_aux_prepare(SINGLE_KEY_BIN_TABLE_AUX *table_aux) {
  queue_u64_prepare(&table_aux->insertions);
}

bool single_key_bin_table_aux_contains(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
    if (queue_u64_contains(&table_aux->insertions, pack_args(arg1, arg2)))
      return true;

  if (table_aux->clear)
    return false;

  if (!single_key_bin_table_contains(table, arg1, arg2))
    return false;

  if (col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1))
    return false;

  return true;
}

bool single_key_bin_table_aux_contains_1(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  if (col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
    return true;

  if (table_aux->clear)
    return false;

  if (!single_key_bin_table_contains_1(table, arg1))
    return false;

  if (col_update_status_map_deleted_flag_is_set(&table_aux->col_1_status_map, arg1))
    return false;

  return true;
}

bool single_key_bin_table_aux_contains_2(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, uint32 arg2) {
  if (table_aux->insertions.count > 0) {
    if (!single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux))
      single_key_bin_table_aux_build_col_2_insertion_bitmap(table_aux, table->mem_pool);
    if (single_key_bin_table_aux_arg2_was_inserted(table_aux, arg2))
      return true;
  }

  if (table_aux->clear)
    return false;

  if (!single_key_bin_table_contains_2(table, arg2))
    return false;

  if (col_update_bit_map_is_set(&table_aux->deletions_2, arg2))
    return false;

  assert(queue_u64_count_2(&table_aux->deletions, arg2) <= single_key_bin_table_count_2(table, arg2));

  //## BAD BAD BAD: INEFFICIENT
  if (queue_u64_count_2(&table_aux->deletions, arg2) == single_key_bin_table_count_2(table, arg2))
    return false;

  return true;
}

bool single_key_bin_table_aux_is_empty(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux) {
  if (table_aux->insertions.count > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = single_key_bin_table_size(table);
  uint32 num_dels = table_aux->unique_deletes_count;
  assert(num_dels <= size);
  return size == num_dels;
}

////////////////////////////////////////////////////////////////////////////////

bool single_key_bin_table_aux_check_foreign_key_unary_table_1_forward(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
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

bool single_key_bin_table_aux_check_foreign_key_unary_table_2_forward(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
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

bool single_key_bin_table_aux_check_foreign_key_unary_table_1_backward(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  assert(!table_aux->key_violation_detected);

  if (table_aux->clear) {
    // If no key violation was detected, then all insertions are unique
    //## MAYBE I SHOULD BE CHECKING THIS
    uint32 count = table_aux->insertions.count;

    if (count > 0) {
      uint32 src_size = unary_table_aux_size(src_table, src_table_aux);
      if (src_size > count) {
        //## RECORD THE ERROR
        return false;
      }

      uint32 found = 0;
      uint64 *array = table_aux->insertions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg1 = unpack_arg1(array[i]);
        if (unary_table_aux_contains(src_table, src_table_aux, arg1))
          found++;
      }
      if (src_size > found) {
        //## RECORD THE ERROR
        return false;
      }
    }
    else if (!unary_table_aux_is_empty(src_table, src_table_aux)) {
      //## RECORD THE ERROR
      return false;
    }
  }
  else {
    uint32 num_dels = table_aux->deletions_1.count;
    if (num_dels > 0) {
      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint32 arg1 = arg1s[i];
        if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
          if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
            //## RECORD THE ERROR
            return false;
          }
      }
    }

    uint32 num_dels_2 = table_aux->deletions_2.count;
    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];
        uint32 count2 = single_key_bin_table_count_2(table, arg2);
        uint32 read2 = 0;
        while (read2 < count2) {
          uint32 buffer[64];
          UINT32_ARRAY array2 = single_key_bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
          read2 += array2.size;
          for (uint32 j=0 ; j < array2.size ; j++) {
            uint32 arg1 = array2.array[j];
            if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
              if (unary_table_aux_contains(src_table, src_table_aux, arg1)) {
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

bool single_key_bin_table_aux_check_foreign_key_master_bin_table_backward(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  assert(!table_aux->key_violation_detected);

  if (table_aux->clear) {
    // If no key violation was detected, then all insertions are unique
    //## MAYBE I SHOULD BE CHECKING THIS
    uint32 count = table_aux->insertions.count;

    if (count > 0) {
      uint32 src_size = master_bin_table_aux_size(src_table, src_table_aux);
      if (src_size > count) {
        //## RECORD THE ERROR
        return false;
      }

      uint32 found = 0;
      uint64 *array = table_aux->insertions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg1 = unpack_arg1(array[i]);
        if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1))
          found++;
      }
      if (src_size > found) {
        //## RECORD THE ERROR
        return false;
      }
    }
    else if (!master_bin_table_aux_is_empty(src_table, src_table_aux)) {
      //## RECORD THE ERROR
      return false;
    }
  }
  else {
    uint32 num_dels = table_aux->deletions_1.count;
    if (num_dels > 0) {
      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint32 arg1 = arg1s[i];
        if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
          if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1)) {
            //## RECORD THE ERROR
            return false;
          }
      }
    }

    uint32 num_dels_2 = table_aux->deletions_2.count;
    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];
        uint32 count2 = single_key_bin_table_count_2(table, arg2);
        uint32 read2 = 0;
        while (read2 < count2) {
          uint32 buffer[64];
          UINT32_ARRAY array2 = single_key_bin_table_range_restrict_2(table, arg2, read2, buffer, 64);
          read2 += array2.size;
          for (uint32 j=0 ; j < array2.size ; j++) {
            uint32 arg1 = array2.array[j];
            if (!col_update_status_map_inserted_flag_is_set(&table_aux->col_1_status_map, arg1))
              if (master_bin_table_aux_contains_surr(src_table, src_table_aux, arg1)) {
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

bool single_key_bin_table_aux_check_foreign_key_unary_table_2_backward(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  if (table_aux->clear) {
    // If no key violation was detected, then all insertions are unique
    //## MAYBE I SHOULD BE CHECKING THIS
    uint32 count = table_aux->insertions.count;

    if (count > 0) {
      uint32 src_size = unary_table_aux_size(src_table, src_table_aux);
      if (src_size > count) {
        //## RECORD THE ERROR
        return false;
      }

      uint32 found = 0;
      uint64 *array = table_aux->insertions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg2 = unpack_arg2(array[i]);
        if (unary_table_aux_contains(src_table, src_table_aux, arg2))
          found++;
      }
      if (src_size > found) {
        //## RECORD THE ERROR
        return false;
      }
    }
    else if (!unary_table_aux_is_empty(src_table, src_table_aux)) {
      //## RECORD THE ERROR
      return false;
    }
  }
  else {
    uint32 num_dels_2 = table_aux->deletions_2.count;
    if (num_dels_2 > 0) {
      uint32 *arg2s = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < num_dels_2 ; i++) {
        uint32 arg2 = arg2s[i];
        if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
          if (!single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux))
            single_key_bin_table_aux_build_col_2_insertion_bitmap(table_aux, table->mem_pool);

          if (!single_key_bin_table_aux_arg2_was_inserted(table_aux, arg2)) {
            //## RECORD THE ERROR
            return false;
          }
        }
      }
    }

    uint32 num_dels = table_aux->deletions_1.count;
    if (num_dels > 0) {
      TRNS_MAP_SURR_U32 remaining;
      trns_map_surr_u32_init(&remaining, table->mem_pool);

      uint32 *arg1s = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint32 arg1 = arg1s[i];
        uint32 arg2 = single_key_bin_table_lookup_1(table, arg1);
        assert(arg2 != 0xFFFFFFFF);

        if (unary_table_aux_contains(src_table, src_table_aux, arg2)) {
          if (!single_key_bin_table_aux_arg2_insertion_map_has_been_built(table_aux))
            single_key_bin_table_aux_build_col_2_insertion_bitmap(table_aux, table->mem_pool);

          if (!single_key_bin_table_aux_arg2_was_inserted(table_aux, arg2)) {
            uint32 count = trns_map_surr_u32_lookup(&remaining, arg2, 0);
            if (count == 0)
              count = single_key_bin_table_count_2(table, arg2);
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

  return true;
}
