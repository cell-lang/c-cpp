#include "lib.h"


void raw_obj_col_aux_delete_1(UNARY_TABLE *master, OBJ_COL_AUX *col_aux, uint32 index) {
  if (unary_table_contains(master, index))
    if (!col_update_status_map_check_and_mark_deletion(&col_aux->status_map, index, col_aux->mem_pool)) {
      queue_u32_insert(&col_aux->deletions, index);
      if (col_update_status_map_inserted_flag_is_set(&col_aux->status_map, index)) {
        assert(col_aux->undeleted_inserts > 0);
        col_aux->undeleted_inserts--;
      }
    }
}

void raw_obj_col_aux_insert(UNARY_TABLE *master, OBJ_COL_AUX *col_aux, uint32 index, OBJ value) {
  if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
    col_aux->has_conflicts = true;
    return;
  }

  if (unary_table_contains(master, index))
    if (!col_update_status_map_deleted_flag_is_set(&col_aux->status_map, index))
      col_aux->undeleted_inserts++;

  queue_u32_obj_insert(&col_aux->insertions, index, value);
}

void raw_obj_col_aux_update(UNARY_TABLE *master, OBJ_COL_AUX *col_aux, uint32 index, OBJ value) {
  if (unary_table_contains(master, index)) {
    if (col_update_status_map_check_and_mark_update_return_previous_inserted_flag(&col_aux->status_map, index, col_aux->mem_pool)) {
      col_aux->has_conflicts = true;
      return;
    }

    queue_u32_obj_insert(&col_aux->updates, index, value);
  }
  else {
    if (col_update_status_map_check_and_mark_insertion(&col_aux->status_map, index, col_aux->mem_pool)) {
      col_aux->has_conflicts = true;
      return;
    }

    queue_u32_obj_insert(&col_aux->insertions, index, value);
  }
}

void raw_obj_col_aux_update_unchecked(UNARY_TABLE *master, OBJ_COL_AUX *col_aux, uint32 index, OBJ value) {
  //## THERE OUGHT TO BE SOME EXTRA CHECKS HERE IN DEBUG MODE
  if (unary_table_contains(master, index))
    queue_u32_obj_insert(&col_aux->updates, index, value);
  else
    queue_u32_obj_insert(&col_aux->insertions, index, value);
}

void raw_obj_col_aux_update_existing_unchecked(UNARY_TABLE *master, OBJ_COL_AUX *col_aux, uint32 index, OBJ value) {
  //## THERE OUGHT TO BE SOME EXTRA CHECKS HERE IN DEBUG MODE
  assert(unary_table_contains(master, index));
  queue_u32_obj_insert(&col_aux->updates, index, value);
}

////////////////////////////////////////////////////////////////////////////////

void raw_obj_col_aux_apply(UNARY_TABLE *master_table, UNARY_TABLE_AUX *master_table_aux, RAW_OBJ_COL *column, OBJ_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->clear) {
    assert(master_table->count == 0);
    raw_obj_col_clear(master_table, column, mem_pool);
  }
  else {
    uint32 count = col_aux->deletions.count;
    if (count > 0) {
      uint32 *idxs = col_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++)
        raw_obj_col_delete(column, idxs[i], mem_pool);
    }
  }

  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    OBJ *values = col_aux->updates.obj_array;
    for (uint32 i=0 ; i < count ; i++)
      raw_obj_col_update(master_table, column, idxs[i], values[i], mem_pool);
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    OBJ *values = col_aux->insertions.obj_array;
    for (uint32 i=0 ; i < count ; i++)
      raw_obj_col_insert(column, idxs[i], values[i], mem_pool);
  }
}

////////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING INT AND FLOAT VERSIONS
bool raw_obj_col_aux_check_key_1(UNARY_TABLE *master, RAW_OBJ_COL *, OBJ_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->has_conflicts)
    return false;

  if (col_aux->undeleted_inserts > 0 && !col_aux->clear)
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING INT AND FLOAT VERSIONS
bool raw_obj_col_aux_check_foreign_key_unary_table_1_forward(RAW_OBJ_COL *col, OBJ_COL_AUX *col_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = col_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *arg1s = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1s[i]))
        return false;
    }
  }
  return true;
}

//## NEARLY IDENTICAL TO THE CORRESPONDING INT AND FLOAT VERSIONS
bool raw_obj_col_aux_check_foreign_key_master_bin_table_forward(RAW_OBJ_COL *col, OBJ_COL_AUX *col_aux, MASTER_BIN_TABLE *target_table, MASTER_BIN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = col_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *surrs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      if (!master_bin_table_aux_contains_surr(target_table, target_table_aux, surrs[i]))
        return false;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

//## NEARLY IDENTICAL TO THE CORRESPONDING INT AND FLOAT VERSIONS
bool raw_obj_col_aux_check_foreign_key_unary_table_1_backward(RAW_OBJ_COL *col, OBJ_COL_AUX *col_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  if (col_aux->clear) {
    uint32 ins_count = col_aux->insertions.count;
    if (ins_count > 0) {
      uint32 src_size = unary_table_aux_size(src_table, src_table_aux);
      if (src_size > ins_count)
        return false;

      uint32 *surrs = col_aux->insertions.u32_array;
      uint32 found = 0;
      for (uint32 i=0 ; i < ins_count ; i++) {
        if (unary_table_aux_contains(src_table, src_table_aux, surrs[i]))
          found++;
      }

      if (src_size > found)
        return false;

      return true;
    }
    else {
      bool src_is_empty = unary_table_aux_is_empty(src_table, src_table_aux);
      return src_is_empty;
    }
  }

  uint32 num_dels = col_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *surrs = col_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      uint32 surr = surrs[i];
      if (!col_update_status_map_inserted_flag_is_set(&col_aux->status_map, surr))
        if (unary_table_aux_contains(src_table, src_table_aux, surr))
          return false;
    }
  }

  return true;
}
