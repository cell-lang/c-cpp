#include "lib.h"


bool is_float_col_null(double);

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_init(FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  col_aux->mem_pool = mem_pool;
  col_update_status_map_init(&col_aux->status_map);
  queue_u32_init(&col_aux->deletions);
  queue_u32_double_init(&col_aux->insertions);
  queue_u32_double_init(&col_aux->updates);
  col_aux->undeleted_inserts = 0;
  col_aux->clear = false;
  col_aux->has_conflicts = false;
}

void float_col_aux_reset(FLOAT_COL_AUX *col_aux) {
  col_update_status_map_clear(&col_aux->status_map);
  queue_u32_reset(&col_aux->deletions);
  queue_u32_double_reset(&col_aux->insertions);
  queue_u32_double_reset(&col_aux->updates);
  col_aux->undeleted_inserts = 0;
  col_aux->clear = false;
  col_aux->has_conflicts = false;
}

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_clear(FLOAT_COL_AUX *col_aux) {
  col_aux->clear = true;
}

void float_col_aux_delete_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index) {
  if (float_col_contains_1(col, index)) {
    if (!col_update_status_map_check_and_mark_deletion(&col_aux->status_map, index, col_aux->mem_pool)) {
      queue_u32_insert(&col_aux->deletions, index);
      if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, index)) {
        assert(col_aux->undeleted_inserts > 0);
        col_aux->undeleted_inserts--;
      }
    }
  }
}

void float_col_aux_insert(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
    //## TODO: RECORD THE ERROR
    col_aux->has_conflicts = true;
    return;
  }

  if (float_col_contains_1(col, index))
    if (!col_update_status_map_deleted_flag_is_set(&col_aux->status_map, index))
      col_aux->undeleted_inserts++;

  queue_u32_double_insert(&col_aux->insertions, index, value);
}

void float_col_aux_update(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  if (float_col_contains_1(col, index)) {
    if (col_update_status_map_check_and_mark_update_return_previous_inserted_flag(&col_aux->status_map, index, col_aux->mem_pool)) {
      //## TODO: RECORD THE ERROR
      col_aux->has_conflicts = true;
      return;
    }

    queue_u32_double_insert(&col_aux->updates, index, value);
  }
  else {
    if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
      //## TODO: RECORD THE ERROR
      col_aux->has_conflicts = true;
      return;
    }

    queue_u32_double_insert(&col_aux->insertions, index, value);
  }
}

////////////////////////////////////////////////////////////////////////////////

void float_col_aux_apply_deletions(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store, STATE_MEM_POOL *mem_pool) {
  if (col_aux->clear) {
    if (remove != NULL) {
      double *array = col->array;
      uint32 count = col->count;
      uint32 read = 0;
      for (uint32 idx=0 ; read < count ; idx++)
        if (!is_float_col_null(array[idx])) {
          read++;
          if (!col_update_status_map_inserted_flag_is_set(&col_aux->status_map, idx))
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
        if (float_col_delete(col, idx, mem_pool))
          if (remove != NULL && col_update_status_map_inserted_flag_is_set(&col_aux->status_map, idx))
              remove(store, idx, mem_pool);
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

// static void float_col_aux_record_col_1_key_violation(FLOAT_COL_AUX *col_aux, uint32 idx, double value, double other_value, bool between_new) {
// //   //## BUG: Stores may contain only part of the value (id(5) -> 5)
// //   Obj key = store.surrToValue(idx);
// //   Obj[] tuple1 = new Obj[] {key, value};
// //   Obj[] tuple2 = new Obj[] {key, otherValue};
// //   return new KeyViolationException(relvarName, KeyViolationException.key_1, tuple1, tuple2, betweenNew);

//   //## IMPLEMENT IMPLEMENT IMPLEMENT
// }

// static void float_col_aux_record_col_1_key_violation(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 idx, double value, bool between_new) {
//   if (between_new) {
//     uint32 count = col_aux->updates.count;
//     uint32 *idxs = col_aux->updates.u32_array;
//     for (uint32 i=0 ; i < count ; i++)
//       if (idxs[i] == idx) {
//         float_col_aux_record_col_1_key_violation(col_aux, idx, value, col_aux->updates.float_array[i], between_new);
//         return;
//       }

//     count = col_aux->insertions.count;
//     idxs = col_aux->insertions.u32_array;
//     for (uint32 i=0 ; i < count ; i++)
//       if (idxs[i] == idx) {
//         float_col_aux_record_col_1_key_violation(col_aux, idx, value, col_aux->insertions.float_array[i], between_new);
//         return;
//       }

//     internal_fail();
//   }
//   else
//     float_col_aux_record_col_1_key_violation(col_aux, idx, value, float_col_lookup(col, idx), between_new);
// }

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool float_col_aux_check_key_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->has_conflicts)
    return false;

  if (col_aux->undeleted_inserts > 0 && !col_aux->clear) {
    //## TODO: RECORD THE ERROR
    return false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////

void float_col_aux_prepare(FLOAT_COL_AUX *col_aux) {

}

//////////////////////////////////////////////////////////////////////////////

uint32 float_col_aux_size(FLOAT_COL *col, FLOAT_COL_AUX *col_aux) {
  assert(!col_aux->has_conflicts && col_aux->undeleted_inserts == 0);

  return float_col_size(col) - col_aux->deletions.count + col_aux->insertions.count;
}

bool float_col_aux_contains_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 index) {
  if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, index))
    return true;

  if (col_update_status_map_deleted_flag_is_set(&col_aux->status_map, index))
    return false;

  return float_col_contains_1(col, index);
}

bool float_col_aux_is_empty(FLOAT_COL *col, FLOAT_COL_AUX *col_aux) {
  return float_col_aux_size(col, col_aux) == 0;
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
  return true;
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
//## IS THIS EVER USED (I THINK IT IS)? BUT IF SO, WHY ISN'T THERE AN EQUIVALENT VERSION FOR UNARY TABLES?
bool float_col_aux_check_foreign_key_master_bin_table_backward(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  if (col_aux->clear) {
    uint32 ins_count = col_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = master_bin_table_aux_size(src_table, src_table_aux);
      if (src_size > ins_count) {
        //## TODO: RECORD THE ERROR
        return false;
      }

      uint32 *surrs = col_aux->insertions.u32_array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        if (master_bin_table_aux_contains_surr(src_table, src_table_aux, surrs[i]))
          found++;
      }

      if (src_size > ins_count) {
        //## TODO: RECORD THE ERROR
        return false;
      }

      return true;
    }
    else {
      bool src_is_empty = master_bin_table_aux_is_empty(src_table, src_table_aux);
      if (!src_is_empty) {
        //## RECORD THE ERROR
      }
      return src_is_empty;
    }
  }

  uint32 num_dels = col_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *surrs = col_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 surr = surrs[i];
      if (!col_update_status_map_inserted_flag_is_set(&col_aux->status_map, surr))
        if (master_bin_table_aux_contains_surr(src_table, src_table_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  return true;
}
