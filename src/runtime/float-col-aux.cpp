#include "lib.h"


bool is_float_col_null(double);

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_init(FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  col_update_status_map_init(&col_aux->status_map);
  queue_u32_init(&col_aux->deletions);
  queue_u32_double_init(&col_aux->insertions);
  queue_u32_double_init(&col_aux->updates);
  col_aux->clear = false;
}

void float_col_aux_reset(FLOAT_COL_AUX *col_aux) {
  col_update_status_map_clear(&col_aux->status_map);
  queue_u32_reset(&col_aux->deletions);
  queue_u32_double_reset(&col_aux->insertions);
  queue_u32_double_reset(&col_aux->updates);
  col_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_clear(FLOAT_COL_AUX *col_aux) {
  col_aux->clear = true;
}

void float_col_aux_delete_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index) {
  if (float_col_contains_1(col, index))
    queue_u32_insert(&col_aux->deletions, index);
}

void float_col_aux_insert(FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  queue_u32_double_insert(&col_aux->insertions, index, value);
}

void float_col_aux_update(FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  queue_u32_double_insert(&col_aux->updates, index, value);
}

////////////////////////////////////////////////////////////////////////////////

static void float_col_aux_build_insert_map(FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_status_map_is_dirty(&col_aux->status_map));

  uint32 count = col_aux->insertions.count;
  if (count != 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool);
  }

  count = col_aux->updates.count;
  if (count != 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool);
  }
}

void float_col_aux_apply_deletions(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store, STATE_MEM_POOL *mem_pool) {
  bool has_insertions_or_updates = col_aux->insertions.count > 0 || col_aux->updates.count > 0;
  bool status_map_built = !has_insertions_or_updates || col_update_status_map_is_dirty(&col_aux->status_map);

  if (col_aux->clear) {
    if (remove != NULL) {
      double *array = col->array;
      uint32 count = col->count;
      uint32 read = 0;
      for (uint32 idx=0 ; read < count ; idx++)
        if (!is_float_col_null(array[idx])) {
          read++;
          if (!status_map_built) {
            float_col_aux_build_insert_map(col_aux, mem_pool);
            status_map_built = true;
          }
          if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, idx))
            remove(store, idx, mem_pool);
        }
    }
    float_col_clear(col, mem_pool);
  }
  else {
    uint32 count = col_aux->deletions.count;
    if (count > 0) {
      uint32 *idxs = col_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 idx = idxs[i];
        if (float_col_delete(col, idx, mem_pool)) {
          if (remove != NULL) {
            if (!status_map_built) {
              float_col_aux_build_insert_map(col_aux, mem_pool);
              status_map_built = true;
            }
            if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, idx))
              remove(store, idx, mem_pool);
          }
        }
      }
    }
  }
}

void float_col_aux_apply_updates_and_insertions(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    double *values = col_aux->updates.float_array;
    for (uint32 i=0 ; i < count ; i++)
      float_col_update(col, idxs[i], values[i], mem_pool);
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    double *values = col_aux->insertions.float_array;
    for (uint32 i=0 ; i < count ; i++)
      float_col_insert(col, idxs[i], values[i], mem_pool);
  }
}

//////////////////////////////////////////////////////////////////////////////

static void float_col_aux_record_col_1_key_violation(FLOAT_COL_AUX *col_aux, uint32 idx, double value, double other_value, bool between_new) {
//   //## BUG: Stores may contain only part of the value (id(5) -> 5)
//   Obj key = store.surrToValue(idx);
//   Obj[] tuple1 = new Obj[] {key, value};
//   Obj[] tuple2 = new Obj[] {key, otherValue};
//   return new KeyViolationException(relvarName, KeyViolationException.key_1, tuple1, tuple2, betweenNew);

  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

static void float_col_aux_record_col_1_key_violation(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 idx, double value, bool between_new) {
  if (between_new) {
    uint32 count = col_aux->updates.count;
    uint32 *idxs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      if (idxs[i] == idx) {
        float_col_aux_record_col_1_key_violation(col_aux, idx, value, col_aux->updates.float_array[i], between_new);
        return;
      }

    count = col_aux->insertions.count;
    idxs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      if (idxs[i] == idx) {
        float_col_aux_record_col_1_key_violation(col_aux, idx, value, col_aux->insertions.float_array[i], between_new);
        return;
      }

    internal_fail();
  }
  else
    float_col_aux_record_col_1_key_violation(col_aux, idx, value, float_col_lookup(col, idx), between_new);
}

//////////////////////////////////////////////////////////////////////////////

static bool float_col_aux_value_set_at_is(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  uint32 count = col_aux->insertions.count;
  uint32 *idxs = col_aux->insertions.u32_array;

  for (uint32 i=0 ; i < count ; i++)
    if (idxs[i] == index)
      return col_aux->insertions.float_array[i] == value;

  count = col_aux->updates.count;
  idxs = col_aux->updates.u32_array;
  for (uint32 i=0 ; i < count ; i++)
    if (idxs[i] == index)
      return col_aux->updates.float_array[i] == value;

  if (float_col_contains_1(col, index))
    return float_col_lookup(col, index) == value;

  internal_fail();
}

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool float_col_aux_check_key_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
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
        for (uint32 i=0 ; i < num_deletes ; i++) {
          uint32 idx = idxs[i];
          if (float_col_contains_1(col, idx))
            col_update_status_map_mark_deletion(&col_aux->status_map, idx, mem_pool);
        }
      }

      if (num_updates != 0) {
        uint32 *idxs = col_aux->updates.u32_array;
        for (uint32 i=0 ; i < num_updates ; i++) {
          uint32 idx = idxs[i];
          if (float_col_contains_1(col, idx))
            col_update_status_map_mark_deletion(&col_aux->status_map, idx, mem_pool);
        }
      }

      if (num_insertions != 0) {
        uint32 *idxs = col_aux->insertions.u32_array;
        for (uint32 i=0 ; i < num_insertions ; i++) {
          uint32 idx = idxs[i];
          //## COULD BE MADE MORE EFFICIENT IF WE MERGED THE .._deleted_flag_is_set(..) WITH .._check_and_mark_insertion(..)
          if (float_col_contains_1(col, idx) && !col_update_status_map_deleted_flag_is_set(&col_aux->status_map, idx))
            if (float_col_lookup(col, idx) != col_aux->insertions.float_array[i]) {
              //## CLEAR?
              //## RECORD THE ERROR
              return false;
            }

          if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idxs[i], mem_pool))
            if (!float_col_aux_value_set_at_is(col, col_aux, idx, col_aux->insertions.float_array[i])) {
              //## CLEAR?
              //## RECORD THE ERROR
              return false;
            }
        }
      }

      if (num_updates != 0) {
        uint32 *idxs = col_aux->updates.u32_array;
        for (uint32 i=0 ; i < num_updates ; i++) {
          uint32 idx = idxs[i];
          if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, idx, mem_pool)) {
            if (!float_col_aux_value_set_at_is(col, col_aux, idx, col_aux->updates.float_array[i])) {
              //## CLEAR?
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

//////////////////////////////////////////////////////////////////////////////

void float_col_aux_prepare(FLOAT_COL_AUX *col_aux) {
  queue_u32_sort_unique(&col_aux->deletions); // May need to support unique_count(..)
  queue_u32_double_prepare(&col_aux->insertions);
  queue_u32_double_prepare(&col_aux->updates);
}

bool float_col_aux_contains_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index) {
  if (queue_u32_double_contains_1(&col_aux->insertions, index))
    return true;

  if (queue_u32_double_contains_1(&col_aux->updates, index))
    return true;

  if (col_aux->clear)
    return false;

  if (!float_col_contains_1(col, index))
    return false;

  return !queue_u32_sorted_contains(&col_aux->deletions, index);
}

bool float_col_aux_is_empty(FLOAT_COL *col, FLOAT_COL_AUX *col_aux) {
  if (col_aux->insertions.count > 0 || col_aux->updates.count > 0)
    return false;

  if (col_aux->clear)
    return true;

  uint32 size = float_col_size(col);
  if (size == 0)
    return true;

  return queue_u32_unique_count(&col_aux->deletions) == size;
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool float_col_aux_check_foreign_key_unary_table_1_forward(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
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

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool float_col_aux_check_foreign_key_master_bin_table_forward(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, MASTER_BIN_TABLE *target_table, MASTER_BIN_TABLE_AUX *target_table_aux) {
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

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool float_col_aux_check_foreign_key_master_bin_table_backward(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
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
        if (!float_col_aux_contains_1(col, col_aux, surr)) { //## NOT THE MOST EFFICIENT WAY TO DO IT.
          //## RECORD THE ERROR
          return false;
        }
      }
    }
  }

  return true;
}
