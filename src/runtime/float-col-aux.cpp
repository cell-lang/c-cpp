#include "lib.h"


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

void float_col_aux_delete_1(FLOAT_COL_AUX *col_aux, uint32 index) {
  queue_u32_insert(&col_aux->deletions, index);
}

void float_col_aux_insert(FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  queue_u32_double_insert(&col_aux->insertions, index, value);
}

void float_col_aux_update(FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  queue_u32_double_insert(&col_aux->updates, index, value);
}

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_apply(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->clear) {
    FLOAT_COL_ITER iter;
    float_col_iter_init(col, &iter);
    while (!float_col_iter_is_out_of_range(&iter)) {
      // assert(!is_blank(float_col_iter_get_value(&iter)));
      uint32 idx = float_col_iter_get_idx(&iter);
      decr_rc(store, store_aux, idx);
      float_col_iter_move_forward(&iter);
    }

    float_col_clear(col, mem_pool);
  }
  else {
    uint32 count = col_aux->deletions.count;
    if (count > 0) {
      uint32 *idxs = col_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 idx = idxs[i];
        if (float_col_delete(col, idx, mem_pool))
          decr_rc(store, store_aux, idx);
      }
    }
  }

  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    double *values = col_aux->updates.float_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      double value = values[i];
      if (!float_col_contains_1(col, idx)) //## NOT TOTALLY SURE ABOUT THIS ONE
        incr_rc(store, idx);
      float_col_update(col, idx, value, mem_pool);
    }
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    double *values = col_aux->insertions.float_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      float_col_insert(col, idx, values[i], mem_pool);
      incr_rc(store, idx);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_slave_apply(FLOAT_COL *column, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  float_col_aux_apply(column, col_aux, null_incr_rc, null_decr_rc, NULL, NULL, mem_pool);
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
        for (uint32 i=0 ; i < num_deletes ; i++)
          col_update_status_map_mark_deletion(&col_aux->status_map, idxs[i], mem_pool);
      }

      if (num_insertions != 0) {
        uint32 *idxs = col_aux->insertions.u32_array;
        for (uint32 i=0 ; i < num_insertions ; i++) {
          uint32 idx = idxs[i];
          //## COULD BE MADE MORE EFFICIENT IF WE MERGED THE .._deleted_flag_is_set(..) WITH .._check_and_mark_insertion(..)
          if (float_col_contains_1(col, idx) && !col_update_status_map_deleted_flag_is_set(&col_aux->status_map, idx)) {
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
