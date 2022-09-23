#include "lib.h"


void raw_float_col_aux_delete_1(UNARY_TABLE *master, FLOAT_COL_AUX *col_aux, uint32 index) {
  if (unary_table_contains(master, index)) {
    if (!col_update_status_map_check_and_mark_deletion(&col_aux->status_map, index, col_aux->mem_pool)) {
      queue_u32_insert(&col_aux->deletions, index);
      if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, index)) {
        assert(col_aux->undeleted_inserts > 0);
        col_aux->undeleted_inserts--;
      }
    }
  }
}

void raw_float_col_aux_insert(UNARY_TABLE *master, FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
    //## TODO: RECORD THE ERROR
    col_aux->has_conflicts = true;
    return;
  }

  if (unary_table_contains(master, index))
    if (!col_update_status_map_deleted_flag_is_set(&col_aux->status_map, index))
      col_aux->undeleted_inserts++;

  queue_u32_double_insert(&col_aux->insertions, index, value);
}

void raw_float_col_aux_update(UNARY_TABLE *master, FLOAT_COL_AUX *col_aux, uint32 index, double value) {
  if (unary_table_contains(master, index)) {
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

void raw_float_col_aux_apply(UNARY_TABLE *master_table, UNARY_TABLE_AUX *master_table_aux, RAW_FLOAT_COL *column, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    double *values = col_aux->updates.float_array;
    for (uint32 i=0 ; i < count ; i++)
      raw_float_col_update(master_table, column, idxs[i], values[i], mem_pool);
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    double *values = col_aux->insertions.float_array;
    for (uint32 i=0 ; i < count ; i++)
      raw_float_col_insert(column, idxs[i], values[i], mem_pool);
  }
}

////////////////////////////////////////////////////////////////////////////////

static void raw_float_col_aux_record_col_1_key_violation(RAW_FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 idx, double value, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING OBJ AND INT VERSIONS
bool raw_float_col_aux_check_key_1(UNARY_TABLE *master, RAW_FLOAT_COL *, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->has_conflicts)
    return false;

  if (col_aux->undeleted_inserts > 0 && !col_aux->clear) {
    //## TODO: RECORD THE ERROR
    return false;
  }

  return true;
}
