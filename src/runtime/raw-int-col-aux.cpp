#include "lib.h"


void raw_int_col_aux_delete_1(UNARY_TABLE *master, INT_COL_AUX *col_aux, uint32 index) {
  if (unary_table_contains(master, index))
    if (!col_update_status_map_check_and_mark_deletion(&col_aux->status_map, index, col_aux->mem_pool)) {
      queue_u32_insert(&col_aux->deletions, index);
      if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, index)) {
        assert(col_aux->undeleted_inserts > 0);
        col_aux->undeleted_inserts--;
      }
    }
}

void raw_int_col_aux_insert(UNARY_TABLE *master, INT_COL_AUX *col_aux, uint32 index, int64 value) {
  if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
    //## TODO: RECORD THE ERROR
    col_aux->has_conflicts = true;
    return;
  }

  if (unary_table_contains(master, index))
    if (!col_update_status_map_deleted_flag_is_set(&col_aux->status_map, index))
      col_aux->undeleted_inserts++;

  queue_u32_i64_insert(&col_aux->insertions, index, value);
}

void raw_int_col_aux_update(UNARY_TABLE *master, INT_COL_AUX *col_aux, uint32 index, int64 value) {
  if (unary_table_contains(master, index)) {
    if (col_update_status_map_check_and_mark_update_return_previous_inserted_flag(&col_aux->status_map, index, col_aux->mem_pool)) {
      //## TODO: RECORD THE ERROR
      col_aux->has_conflicts = true;
      return;
    }

    queue_u32_i64_insert(&col_aux->updates, index, value);
  }
  else {
    if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
      //## TODO: RECORD THE ERROR
      col_aux->has_conflicts = true;
      return;
    }

    queue_u32_i64_insert(&col_aux->insertions, index, value);
  }
}

////////////////////////////////////////////////////////////////////////////////

void raw_int_col_aux_apply(UNARY_TABLE *master_table, UNARY_TABLE_AUX *master_table_aux, RAW_INT_COL *column, INT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    int64 *values = col_aux->updates.i64_array;
    for (uint32 i=0 ; i < count ; i++)
      raw_int_col_update(master_table, column, idxs[i], values[i], mem_pool);
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    int64 *values = col_aux->insertions.i64_array;
    for (uint32 i=0 ; i < count ; i++)
      raw_int_col_insert(column, idxs[i], values[i], mem_pool);
  }
}

////////////////////////////////////////////////////////////////////////////////

static void raw_int_col_aux_record_col_1_key_violation(RAW_INT_COL *col, INT_COL_AUX *col_aux, uint32 idx, int64 value, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool raw_int_col_aux_check_key_1(UNARY_TABLE *master, RAW_INT_COL *, INT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->has_conflicts)
    return false;

  if (col_aux->undeleted_inserts > 0 && !col_aux->clear) {
    //## TODO: RECORD THE ERROR
    return false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool raw_int_col_aux_check_foreign_key_unary_table_1_forward(RAW_INT_COL *col, INT_COL_AUX *col_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
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

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool raw_int_col_aux_check_foreign_key_master_bin_table_forward(RAW_INT_COL *col, INT_COL_AUX *col_aux, MASTER_BIN_TABLE *target_table, MASTER_BIN_TABLE_AUX *target_table_aux) {
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

////////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND FLOAT VERSIONS
bool raw_int_col_aux_check_foreign_key_unary_table_1_backward(RAW_INT_COL *col, INT_COL_AUX *col_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  if (col_aux->clear) {
    uint32 ins_count = col_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = unary_table_aux_size(src_table, src_table_aux);
      if (src_size > ins_count) {
        //## TODO: RECORD THE ERROR
        return false;
      }

      uint32 *surrs = col_aux->insertions.u32_array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        if (unary_table_aux_contains(src_table, src_table_aux, surrs[i]))
          found++;
      }

      if (src_size > found) {
        //## TODO: RECORD THE ERROR
        return false;
      }

      return true;
    }
    else {
      bool src_is_empty = unary_table_aux_is_empty(src_table, src_table_aux);
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
        if (unary_table_aux_contains(src_table, src_table_aux, surr)) {
          //## RECORD THE ERROR
          return false;
        }
    }
  }

  return true;
}
