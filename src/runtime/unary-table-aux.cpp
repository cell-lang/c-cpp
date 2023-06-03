#include "lib.h"


void unary_table_aux_init(UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_status_map_init(&table_aux->status_map);
  queue_u32_init(&table_aux->deletions);
  queue_u32_init(&table_aux->insertions);
  table_aux->clear = false;
  table_aux->reinsertions_count = 0;
}

void unary_table_aux_reset(UNARY_TABLE_AUX *table_aux) {
  col_update_status_map_clear(&table_aux->status_map);
  queue_u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->deletions);
  table_aux->clear = false;
  table_aux->reinsertions_count = 0;
}

////////////////////////////////////////////////////////////////////////////////

uint32 unary_table_aux_insert(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, uint32 elt, STATE_MEM_POOL *mem_pool) {
  if (!col_update_status_map_check_and_mark_insertion(&table_aux->status_map, elt, mem_pool))
    if (!unary_table_contains(table, elt))
      queue_u32_insert(&table_aux->insertions, elt);
    else
      table_aux->reinsertions_count++;
}

void unary_table_aux_delete(UNARY_TABLE_AUX *table_aux, uint32 elt, STATE_MEM_POOL *mem_pool) {
  if (!col_update_status_map_check_and_mark_deletion(&table_aux->status_map, elt, mem_pool))
    queue_u32_insert(&table_aux->deletions, elt);
}

void unary_table_aux_clear(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

////////////////////////////////////////////////////////////////////////////////

void unary_table_aux_apply_deletions(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    if (table_aux->insertions.count == 0 && table_aux->reinsertions_count == 0) {
      if (remove != NULL)
        remove(store, 0xFFFFFFFF, mem_pool);
      unary_table_clear(table);
    }
    else {
      uint32 left = table->count;
      uint64 *bitmap = table->bitmap;
      for (uint32 word_idx=0 ; left > 0 ; word_idx++) {
        uint64 word = bitmap[word_idx];
        for (uint32 bit_idx=0 ; word != 0 ; bit_idx++) {
          if (word & 1 != 0) {
            left--;
            uint32 elt = 64 * word_idx + bit_idx;
            if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt)) {
              bool found = unary_table_delete(table, elt);
              assert(found);
              if (remove != NULL)
                remove(store, elt, mem_pool);
            }
          }
          word >>= 1;
        }
      }
    }
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 elt = array[i];
        if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt)) {
          bool found = unary_table_delete(table, elt);
          assert(found);
          if (remove != NULL)
            remove(store, elt, mem_pool);
        }
      }
    }
  }
}

void unary_table_aux_apply_insertions(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 elt = array[i];
      bool was_new = unary_table_insert(table, elt, mem_pool);
      assert(was_new);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

uint32 unary_table_aux_size(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  if (table_aux->clear)
    return table_aux->insertions.count + table_aux->reinsertions_count;

  uint32 dels_count = table_aux->deletions.count;
  uint32 size = unary_table_size(table) - dels_count + table_aux->insertions.count;

  // Uncounting the elements that were deleted and reinserted
  //## BAD BAD BAD: THIS SHOULD BE COMPUTED AS YOU GO, OR OTHERWISE THE RESULT SHOULD BE CACHED
  if (dels_count > 0 && table_aux->reinsertions_count > 0) {
    uint32 *elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < dels_count ; i++)
      if (col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elts[i]))
        dels_count++;
  }

  return size;
}

bool unary_table_aux_contains(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, uint32 elt) {
  if (col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
    return true;

  if (table_aux->clear || col_update_status_map_deleted_flag_is_set(&table_aux->status_map, elt))
    return false;

  return unary_table_contains(table, elt);
}

bool unary_table_aux_is_empty(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  if (table_aux->insertions.count > 0 | table_aux->reinsertions_count > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = table->count;
  if (size == 0)
    return true;

  uint32 num_dels = table_aux->deletions.count;
  assert(num_dels <= size);
  return num_dels == size;
}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_aux_check_foreign_key_unary_table_forward(UNARY_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!unary_table_aux_contains(target_table, target_table_aux, new_elts[i]))
        return false;
  }
  return true;
}

bool unary_table_aux_check_foreign_key_slave_tern_table_3_forward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *target_table, SLAVE_TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!bin_table_aux_contains_2(target_table, &target_table_aux->slave_table_aux, new_elts[i]))
        return false;
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_3_forward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *target_table, TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!tern_table_aux_contains_3(target_table, target_table_aux, new_elts[i]))
        return false;
  }
  return true;
}

bool unary_table_aux_check_foreign_key_semisym_tern_table_3_forward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *target_table, SEMISYM_TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!semisym_tern_table_aux_contains_3(target_table, target_table_aux, new_elts[i]))
        return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_aux_check_foreign_key_unary_table_backward(UNARY_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (unary_table_aux_contains(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (bin_table_aux_contains_1(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (bin_table_aux_contains_2(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_single_key_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, SINGLE_KEY_BIN_TABLE *src_table, SINGLE_KEY_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (single_key_bin_table_aux_contains_1(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_single_key_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, SINGLE_KEY_BIN_TABLE *src_table, SINGLE_KEY_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (single_key_bin_table_aux_contains_2(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_double_key_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, DOUBLE_KEY_BIN_TABLE *src_table, DOUBLE_KEY_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (double_key_bin_table_aux_contains_1(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_double_key_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, DOUBLE_KEY_BIN_TABLE *src_table, DOUBLE_KEY_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (double_key_bin_table_aux_contains_2(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_sym_bin_table_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, SYM_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (sym_bin_table_aux_contains_1(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_master_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (master_bin_table_aux_contains_1(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_master_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (master_bin_table_aux_contains_2(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_obj_col_1_backward(UNARY_TABLE_AUX *table_aux, OBJ_COL *src_col, OBJ_COL_AUX *src_col_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (obj_col_aux_contains_1(src_col, src_col_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_int_col_1_backward(UNARY_TABLE_AUX *table_aux, INT_COL *src_col, INT_COL_AUX *src_col_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (int_col_aux_contains_1(src_col, src_col_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_float_col_1_backward(UNARY_TABLE_AUX *table_aux, FLOAT_COL *src_col, FLOAT_COL_AUX *src_col_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (float_col_aux_contains_1(src_col, src_col_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_slave_tern_table_3_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, SLAVE_TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (bin_table_aux_contains_2(src_table, &src_table_aux->slave_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_1_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (tern_table_aux_contains_1(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_2_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (tern_table_aux_contains_2(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_3_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (tern_table_aux_contains_3(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_semisym_tern_table_3_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, SEMISYM_TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 elt = del_elts[i];
      if (!col_update_status_map_inserted_flag_is_set(&table_aux->status_map, elt))
        if (semisym_tern_table_aux_contains_3(src_table, src_table_aux, elt))
          return false;
    }
  }
  return true;
}
