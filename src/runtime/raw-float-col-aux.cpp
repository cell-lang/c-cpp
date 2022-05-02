#include "lib.h"


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
          if (unary_table_contains(master, idx) && !col_update_status_map_deleted_flag_is_set(&col_aux->status_map, idx)) {
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
