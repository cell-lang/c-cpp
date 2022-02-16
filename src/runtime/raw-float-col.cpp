#include "lib.h"


void raw_float_col_init(UNARY_TABLE *master_table, RAW_FLOAT_COL *column, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = master_table->capacity;
  column->array = alloc_state_mem_float_array(mem_pool, capacity);
#ifndef NDEBUG
  for (uint32 i=0 ; i < capacity ; i++)
    column->array[i] = 0;
  column->capacity = capacity;
#endif
}

void raw_float_col_resize(RAW_FLOAT_COL *column, uint32 capacity, uint32 new_capacity, STATE_MEM_POOL *mem_pool) {
  assert(capacity == column->capacity);

  if (new_capacity > capacity) {
    column->array = extend_state_mem_float_array(mem_pool, column->array, capacity, new_capacity);
#ifndef NDEBUG
    for (uint32 i=capacity ; i < new_capacity ; i++)
      column->array[i] = 0;
#endif
  }
  else {
    double *array = column->array;
    double *new_array = alloc_state_mem_float_array(mem_pool, new_capacity);
    memcpy(new_array, array, new_capacity * sizeof(double));
    column->array = new_array;
    release_state_mem_float_array(mem_pool, array, capacity);
  }

#ifndef NDEBUG
  column->capacity = new_capacity;
#endif
}

////////////////////////////////////////////////////////////////////////////////

double raw_float_col_lookup(UNARY_TABLE *master_table, RAW_FLOAT_COL *column, uint32 idx) {
  assert(master_table->capacity == column->capacity);

  if (unary_table_contains(master_table, idx)) {
    return column->array[idx];
  }

  soft_fail(NULL); //## ADD MESSAGE
}

////////////////////////////////////////////////////////////////////////////////

void raw_float_col_insert(RAW_FLOAT_COL *column, uint32 idx, double value, STATE_MEM_POOL *mem_pool) {
  assert(idx < column->capacity);

  column->array[idx] = value;
}

void raw_float_col_update(UNARY_TABLE *master_table, RAW_FLOAT_COL *column, uint32 idx, double value, STATE_MEM_POOL *mem_pool) {
  assert(master_table->capacity == column->capacity);
  assert(idx < column->capacity && unary_table_contains(master_table, idx));

  column->array[idx] = value;
}

////////////////////////////////////////////////////////////////////////////////

void raw_float_col_copy_to(UNARY_TABLE *master_table, RAW_FLOAT_COL *column, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  assert(master_table->capacity == column->capacity);

  double *array = column->array;

  UNARY_TABLE_ITER iter;
  unary_table_iter_init(master_table, &iter);

  while (!unary_table_iter_is_out_of_range(&iter)) {
    uint32 key_surr = unary_table_iter_get(&iter);
    OBJ key = surr_to_obj(store, key_surr);
    OBJ value = make_int(array[key_surr]);

    append(*strm_1, key);
    append(*strm_2, value);

    unary_table_iter_move_forward(&iter);
  }
}

void raw_float_col_write(WRITE_FILE_STATE *write_state, UNARY_TABLE *master_table, RAW_FLOAT_COL *column, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip) {
  assert(master_table->capacity == column->capacity);

  uint32 remaining = master_table->count;
  double *array = column->array;

  UNARY_TABLE_ITER iter;
  unary_table_iter_init(master_table, &iter);

  while (!unary_table_iter_is_out_of_range(&iter)) {
    uint32 key_surr = unary_table_iter_get(&iter);
    OBJ key = surr_to_obj(store, key_surr);
    OBJ value = make_int(column->array[key_surr]); //## NO NEED TO CONVERT THIS TO OBJ

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

void raw_float_col_iter_init(UNARY_TABLE *master_table, RAW_FLOAT_COL *column, RAW_FLOAT_COL_ITER *iter) {
  unary_table_iter_init(master_table, &iter->iter);
  iter->array = column->array;
}

bool raw_float_col_iter_is_out_of_range(RAW_FLOAT_COL_ITER *iter) {
  unary_table_iter_is_out_of_range(&iter->iter);
}

uint32 raw_float_col_iter_get_idx(RAW_FLOAT_COL_ITER *iter) {
  return unary_table_iter_get(&iter->iter);
}

double raw_float_col_iter_get_value(RAW_FLOAT_COL_ITER *iter) {
  uint32 idx = unary_table_iter_get(&iter->iter);
  return iter->array[idx];
}

void raw_float_col_iter_move_forward(RAW_FLOAT_COL_ITER *iter) {
  unary_table_iter_move_forward(&iter->iter);
}
