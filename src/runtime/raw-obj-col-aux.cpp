#include "lib.h"


void raw_obj_col_aux_apply(UNARY_TABLE *master_table, UNARY_TABLE_AUX *master_table_aux, RAW_OBJ_COL *column, OBJ_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->clear) {
    assert(master_table->count == 0);
    raw_obj_col_clear(master_table, column, mem_pool);
  }
  else {
    uint32 count = col_aux->deletions.count;
    if (count > 0) {
      uint32 *idxs = col_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 idx = idxs[i];
        raw_obj_col_delete(column, idx, mem_pool);
      }
    }
  }

  uint32 count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    OBJ *values = col_aux->updates.obj_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      OBJ value = values[i];
      assert(!is_blank(value));
      raw_obj_col_update(master_table, column, idx, values[i], mem_pool);
    }
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    OBJ *values = col_aux->insertions.obj_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      raw_obj_col_insert(column, idx, values[i], mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void record_col_1_key_violation(RAW_OBJ_COL *col, OBJ_COL_AUX *col_aux, uint32 idx, OBJ value, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

bool obj_col_aux_build_bitmap_and_check_key(UNARY_TABLE *master_table, RAW_OBJ_COL *column, OBJ_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  assert(col_aux->insertions.count > 0 || col_aux->updates.count > 0);
  assert(col_aux->max_idx_plus_one > 0);

  uint32 max_idx = col_aux->max_idx_plus_one - 1;
  uint32 bitmap_size = col_aux->bitmap_size;
  uint64 *bitmap = col_aux->bitmap;

  if (max_idx / 32 >= bitmap_size) {
    release_state_mem_uint64_array(mem_pool, bitmap, bitmap_size);
    do
      bitmap_size *= 2;
    while (max_idx / 32 >= bitmap_size);
    bitmap = alloc_state_mem_zeroed_uint64_array(mem_pool, bitmap_size);
    col_aux->bitmap_size = bitmap_size;
    col_aux->bitmap = bitmap;
  }

  col_aux->dirty = true;

  // 00 - untouched
  // 01 - deleted
  // 10 - inserted
  // 11 - updated or inserted and deleted

  uint32 count = col_aux->deletions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->deletions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      uint32 slot_idx = idx / 32;
      uint32 shift = 2 * (idx % 32);
      uint64 slot = bitmap[slot_idx];
      slot |= 1ULL << shift;
      bitmap[slot_idx] = slot;
    }
  }

  count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      uint32 slot_idx = idx / 32;
      uint32 shift = 2 * (idx % 32);
      uint64 slot = bitmap[slot_idx];
      if (((slot >> shift) & 2) != 0) {
        //## HERE I WOULD ACTUALLY NEED TO CHECK THAT THE NEW VALUE IS DIFFERENT FROM THE OLD ONE
        record_col_1_key_violation(column, col_aux, idx, col_aux->updates.obj_array[i], true);
        return false;
      }
      slot |= 3ULL << shift;
      bitmap[slot_idx] = slot;
    }
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      uint32 slot_idx = idx / 32;
      uint32 shift = 2 * (idx % 32);
      uint64 slot = bitmap[slot_idx];
      uint64 flags = (slot >> shift) & 3;
      if (flags >= 2) {
        //## HERE I WOULD ACTUALLY NEED TO CHECK THAT THE NEW VALUE IS DIFFERENT FROM THE OLD ONE
        record_col_1_key_violation(column, col_aux, idx, col_aux->insertions.obj_array[i], true);
        return false;
      }
      if (flags == 0 && unary_table_contains(master_table, idx)) {
        //## HERE I WOULD ACTUALLY NEED TO CHECK THAT THE NEW VALUE IS DIFFERENT FROM THE OLD ONE
        record_col_1_key_violation(column, col_aux, idx, col_aux->insertions.obj_array[i], false);
        return false;
      }
      slot |= 2ULL << shift;
      bitmap[slot_idx] = slot;
    }
  }

  return true;
}

bool raw_obj_col_aux_check_key_1(UNARY_TABLE *master_table, RAW_OBJ_COL *column, OBJ_COL_AUX *col_aux, STATE_MEM_POOL *mem_pool) {
  if (col_aux->insertions.count != 0 || col_aux->updates.count != 0) {
    assert(col_aux->max_idx_plus_one > 0 && !col_aux->dirty);
    return obj_col_aux_build_bitmap_and_check_key(master_table, column, col_aux, mem_pool);
  }
  else
    return true;
}
