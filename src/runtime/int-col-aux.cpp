#include "lib.h"


void int_col_aux_init(INT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  col_update_status_map_init(&col_aux->status_map);
  queue_u32_init(&col_aux->deletions);
  queue_u32_i64_init(&col_aux->insertions);
  queue_u32_i64_init(&col_aux->updates);
  col_aux->clear = false;
}

void int_col_aux_reset(INT_COL_AUX *col_aux) {
  col_update_status_map_clear(&col_aux->status_map);
  queue_u32_reset(&col_aux->deletions);
  queue_u32_i64_reset(&col_aux->insertions);
  queue_u32_i64_reset(&col_aux->updates);
  col_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void int_col_aux_clear(INT_COL_AUX *col_aux) {
  col_aux->clear = true;
}

void int_col_aux_delete_1(INT_COL_AUX *col_aux, uint32 index) {
  queue_u32_insert(&col_aux->deletions, index);
}

void int_col_aux_insert(INT_COL_AUX *col_aux, uint32 index, int64 value) {
  queue_u32_i64_insert(&col_aux->insertions, index, value);
}

void int_col_aux_update(INT_COL_AUX *col_aux, uint32 index, int64 value) {
  queue_u32_i64_insert(&col_aux->updates, index, value);
}

////////////////////////////////////////////////////////////////////////////////

void int_col_aux_apply(INT_COL *col, INT_COL_AUX *col_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->clear) {
    INT_COL_ITER iter;
    int_col_iter_init(col, &iter);
    while (!int_col_iter_is_out_of_range(&iter)) {
      // assert(!is_blank(int_col_iter_get_value(&iter)));
      uint32 idx = int_col_iter_get_idx(&iter);
      decr_rc(store, store_aux, idx);
      int_col_iter_move_forward(&iter);
    }

    int_col_clear(col, mem_pool);
  }
  else {
    uint32 count = col_aux->deletions.count;
    if (count > 0) {
      uint32 *idxs = col_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 idx = idxs[i];
        if (int_col_delete(col, idx, mem_pool))
          decr_rc(store, store_aux, idx);
      }
    }
  }

  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    int64 *values = col_aux->updates.i64_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      int64 value = values[i];
      if (!int_col_contains_1(col, idx)) //## NOT TOTALLY SURE ABOUT THIS ONE
        incr_rc(store, idx);
      int_col_update(col, idx, value, mem_pool);
    }
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    int64 *values = col_aux->insertions.i64_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      int_col_insert(col, idx, values[i], mem_pool);
      incr_rc(store, idx);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

void int_col_aux_slave_apply(INT_COL *column, INT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  int_col_aux_apply(column, col_aux, null_incr_rc, null_decr_rc, NULL, NULL, mem_pool);
}

//////////////////////////////////////////////////////////////////////////////

static void int_col_aux_record_col_1_key_violation(INT_COL_AUX *col_aux, uint32 idx, int64 value, int64 other_value, bool between_new) {
//   //## BUG: Stores may contain only part of the value (id(5) -> 5)
//   Obj key = store.surrToValue(idx);
//   Obj[] tuple1 = new Obj[] {key, value};
//   Obj[] tuple2 = new Obj[] {key, otherValue};
//   return new KeyViolationException(relvarName, KeyViolationException.key_1, tuple1, tuple2, betweenNew);

  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

static void int_col_aux_record_col_1_key_violation(INT_COL *col, INT_COL_AUX *col_aux, uint32 idx, int64 value, bool between_new) {
  if (between_new) {
    uint32 count = col_aux->updates.count;
    uint32 *idxs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      if (idxs[i] == idx) {
        int_col_aux_record_col_1_key_violation(col_aux, idx, value, col_aux->updates.i64_array[i], between_new);
        return;
      }

    count = col_aux->insertions.count;
    idxs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      if (idxs[i] == idx) {
        int_col_aux_record_col_1_key_violation(col_aux, idx, value, col_aux->insertions.i64_array[i], between_new);
        return;
      }

    internal_fail();
  }
  else
    int_col_aux_record_col_1_key_violation(col_aux, idx, value, int_col_lookup(col, idx), between_new);
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool int_col_aux_check_key_1(INT_COL *col, INT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  assert(col_aux->status_map.bit_map.num_dirty == 0);

  uint32 num_insertions = col_aux->insertions.count;
  uint32 num_updates = col_aux->updates.count;

  if (num_insertions != 0 | num_updates != 0) {
    if (col_aux->clear) {
      if (num_insertions != 0) {
        uint32 *idxs = col_aux->insertions.u32_array;
        for (uint32 i=0 ; i < num_insertions ; i++)
          if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool)) {
            //## CLEAR?
            //## RECORD THE ERROR
            return false;
          }
      }

      if (num_updates != 0) {
        uint32 *idxs = col_aux->updates.u32_array;
        for (uint32 i=0 ; i < num_updates ; i++) {
          if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool)) {
            //## CLEAR?
            //## RECORD THE ERROR
            return false;
          }
        }
      }
    }
    else {
      uint32 num_deletes = col_aux->deletions.count;
      if (num_deletes != 0) {
        uint32 *idxs = col_aux->deletions.array;
        for (uint32 i=0 ; i < num_deletes ; i++)
          col_update_status_map_mark_deletion(&col_aux->status_map, idxs[i], mem_pool);
      }

      if (num_insertions != 0) {
        uint32 *idxs = col_aux->insertions.u32_array;
        for (uint32 i=0 ; i < num_insertions ; i++) {
          uint32 idx = idxs[i];
          //## COULD BE MADE MORE EFFICIENT IF WE MERGED THE .._deleted_flag_is_set(..) WITH .._check_and_mark_insertion(..)
          if (int_col_contains_1(col, idx) && !col_update_status_map_deleted_flag_is_set(&col_aux->status_map, idx)) {
            //## CLEAR?
            //## RECORD THE ERROR
            return false;
          }

          if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool)) {
            //## CLEAR?
            //## RECORD THE ERROR
            return false;
          }
        }
      }

      if (num_updates != 0) {
        uint32 *idxs = col_aux->updates.u32_array;
        for (uint32 i=0 ; i < num_updates ; i++) {
          if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool)) {
            //## CLEAR?
            //## RECORD THE ERROR
            return false;
          }
        }
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void int_col_aux_prepare(INT_COL_AUX *col_aux) {
  queue_u32_sort_unique(&col_aux->deletions); // May need to support unique_count(..)
  queue_u32_i64_prepare(&col_aux->insertions);
  queue_u32_i64_prepare(&col_aux->updates);
}

bool int_col_aux_contains_1(INT_COL *col, INT_COL_AUX *col_aux, uint32 index) {
  if (queue_u32_i64_contains_1(&col_aux->insertions, index))
    return true;

  if (queue_u32_i64_contains_1(&col_aux->updates, index))
    return true;

  if (col_aux->clear)
    return false;

  if (!int_col_contains_1(col, index))
    return false;

  return !queue_u32_sorted_contains(&col_aux->deletions, index);
}

bool int_col_aux_is_empty(INT_COL *col, INT_COL_AUX *col_aux) {
  if (col_aux->insertions.count > 0 || col_aux->updates.count > 0)
    return false;

  if (col_aux->clear)
    return true;

  uint32 size = int_col_size(col);
  if (size == 0)
    return true;

  return queue_u32_unique_count(&col_aux->deletions) == size;
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool int_col_aux_check_foreign_key_unary_table_1_forward(INT_COL *col, INT_COL_AUX *col_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = col_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *arg1s = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1s[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }

  uint32 num_updates = col_aux->updates.count;
  if (num_updates > 0) {
    uint32 *arg1s = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < num_updates ; i++) {
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1s[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }

  return true;
}

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool int_col_table_aux_check_foreign_key_master_bin_table_12_forward(INT_COL *col, INT_COL_AUX *col_aux, MASTER_BIN_TABLE *target_table, MASTER_BIN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = col_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *surrs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      if (!master_bin_table_aux_contains_surr(target_table, target_table_aux, surrs[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }

  uint32 num_updates = col_aux->updates.count;
  if (num_updates > 0) {
    uint32 *surrs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < num_updates ; i++) {
      if (!master_bin_table_aux_contains_surr(target_table, target_table_aux, surrs[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool int_col_aux_check_foreign_key_master_bin_table_surr_backward(INT_COL *col, INT_COL_AUX *col_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  if (col_aux->clear) {
    if (!master_bin_table_aux_is_empty(src_table, src_table_aux)) {
      //## BUG BUG BUG: WHAT IF THE TABLE IS CLEARED, BUT THEN IT'S INSERTED INTO?
      //## RECORD THE ERROR
      return false;
    }
  }

  uint32 num_dels = col_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *surrs = col_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 surr = surrs[i];
      if (master_bin_table_aux_contains_surr(src_table, src_table_aux, surr)) {
        if (!int_col_aux_contains_1(col, col_aux, surr)) { //## NOT THE MOST EFFICIENT WAY TO DO IT.
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  return true;
}
