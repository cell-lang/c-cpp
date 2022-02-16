#include "lib.h"


void raw_obj_col_init(UNARY_TABLE *master_table, RAW_OBJ_COL *column, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = master_table->capacity;
  column->array = alloc_state_mem_blanked_obj_array(mem_pool, capacity);
#ifndef NDEBUG
  column->capacity = capacity;
  column->count = 0;
#endif
}

void raw_obj_col_resize(RAW_OBJ_COL *column, uint32 capacity, uint32 new_capacity, STATE_MEM_POOL *mem_pool) {
  assert(capacity == column->capacity);

  if (new_capacity > capacity) {
    column->array = extend_state_mem_blanked_obj_array(mem_pool, column->array, capacity, new_capacity);
  }
  else {
    OBJ *array = column->array;
    for (int i=new_capacity ; i < capacity ; i++) {
      OBJ obj = array[i];
      if (!is_inline_obj(obj))
        remove_from_pool(mem_pool, obj);
    }
    OBJ *new_array = alloc_state_mem_obj_array(mem_pool, new_capacity);
    memcpy(new_array, array, new_capacity * sizeof(OBJ));
    release_state_mem_obj_array(mem_pool, array, capacity);
  }

#ifndef NDEBUG
  column->capacity = new_capacity;
#endif
}

////////////////////////////////////////////////////////////////////////////////

OBJ raw_obj_col_lookup(UNARY_TABLE *master_table, RAW_OBJ_COL *column, uint32 idx) {
  assert(master_table->count == column->count && master_table->capacity == column->capacity);

  if (unary_table_contains(master_table, idx)) {
    assert(!is_blank(column->array[idx]));
    return column->array[idx];
  }

  soft_fail(NULL); //## ADD MESSAGE
}

////////////////////////////////////////////////////////////////////////////////

void raw_obj_col_insert(RAW_OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *mem_pool) {
  assert(idx < column->capacity && is_blank(column->array[idx]));
  column->array[idx] = copy_to_pool(mem_pool, value);

#ifndef NDEBUG
  column->count++;
#endif
}

void raw_obj_col_update(UNARY_TABLE *master_table, RAW_OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *mem_pool) {
  assert(master_table->capacity == column->capacity);
  assert(idx < column->capacity && unary_table_contains(master_table, idx));

  OBJ *ptr = column->array + idx;
  OBJ curr_value = *ptr;
  if (!is_blank(curr_value))
    remove_from_pool(mem_pool, curr_value);
  *ptr = copy_to_pool(mem_pool, value);

#ifndef NDEBUG
  if (is_blank(curr_value))
    column->count++;
#endif
}

void raw_obj_col_delete(RAW_OBJ_COL *column, uint32 idx, STATE_MEM_POOL *mem_pool) {
  assert(idx < column->capacity);

  OBJ *ptr = column->array + idx;
  OBJ value = *ptr;
  if (!is_inline_obj(value))
    remove_from_pool(mem_pool, value);
  *ptr = make_blank_obj(); //## ANY WAY TO MAKE THIS UNNECESSARY?

#ifndef NDEBUG
  if (!is_blank(value))
    column->count--;
#endif
}

////////////////////////////////////////////////////////////////////////////////

void raw_obj_col_copy_to(UNARY_TABLE *master_table, RAW_OBJ_COL *column, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  assert(master_table->count == column->count && master_table->capacity == column->capacity);

  UNARY_TABLE_ITER iter;
  unary_table_iter_init(master_table, &iter);

  while (!unary_table_iter_is_out_of_range(&iter)) {
    uint32 key_surr = unary_table_iter_get(&iter);
    OBJ key = surr_to_obj(store, key_surr);
    OBJ value = column->array[key_surr];
    assert(!is_blank(value));

    append(*strm_1, key);
    append(*strm_2, value);

    unary_table_iter_move_forward(&iter);
  }
}

void raw_obj_col_write(WRITE_FILE_STATE *write_state, UNARY_TABLE *master_table, RAW_OBJ_COL *column, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip) {
  assert(master_table->count == column->count && master_table->capacity == column->capacity);

  uint32 remaining = master_table->count;

  UNARY_TABLE_ITER iter;
  unary_table_iter_init(master_table, &iter);

  while (!unary_table_iter_is_out_of_range(&iter)) {
    uint32 key_surr = unary_table_iter_get(&iter);
    OBJ key = surr_to_obj(store, key_surr);
    OBJ value = column->array[key_surr];
    assert(!is_blank(value));

    write_str(write_state, "\n    ");
    write_obj(write_state, flip ? value : key);
    write_str(write_state, " -> ");
    write_obj(write_state, flip ? key : value);
    if (--remaining > 0)
      write_str(write_state, ",");

    unary_table_iter_move_forward(&iter);
  }
}

////////////////////////////////////////////////////////////////////////////////

void raw_obj_col_iter_init(UNARY_TABLE *master_table, RAW_OBJ_COL *column, RAW_OBJ_COL_ITER *iter) {
  unary_table_iter_init(master_table, &iter->iter);
  iter->array = column->array;
}

bool raw_obj_col_iter_is_out_of_range(RAW_OBJ_COL_ITER *iter) {
  unary_table_iter_is_out_of_range(&iter->iter);
}

uint32 raw_obj_col_iter_get_idx(RAW_OBJ_COL_ITER *iter) {
  return unary_table_iter_get(&iter->iter);
}

OBJ raw_obj_col_iter_get_value(RAW_OBJ_COL_ITER *iter) {
  uint32 idx = unary_table_iter_get(&iter->iter);
  assert(!is_blank(iter->array[idx]));
  return iter->array[idx];
}

void raw_obj_col_iter_move_forward(RAW_OBJ_COL_ITER *iter) {
  unary_table_iter_move_forward(&iter->iter);
}
