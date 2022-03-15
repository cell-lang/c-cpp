#include "lib.h"


const uint32 INIT_SIZE = 256;

// const uint64 NULL_BIT_MASK = 0x7FF8000000000000L;
// const uint64 NULL_BIT_MASK = 0x7FFFFFFFFFFFFFFFL;
const uint64 NULL_BIT_MASK = 0x7FFA3E90779F7D08L; // Random NaN

const double FLOAT_NULL = bits_cast_uint64_double(NULL_BIT_MASK);
const double NORM_NAN = 0.0 / 0.0;

////////////////////////////////////////////////////////////////////////////////

inline bool is_float_col_null(double value) {
  return bits_cast_double_uint64(value) == NULL_BIT_MASK;
}

////////////////////////////////////////////////////////////////////////////////

static void float_col_resize(FLOAT_COL *column, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = column->capacity;
  uint32 new_capacity = 2 * capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;
  column->capacity = new_capacity;
  double *array = extend_state_mem_float_array(mem_pool, column->array, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    array[i] = FLOAT_NULL;
  column->array = array;
}

////////////////////////////////////////////////////////////////////////////////

void float_col_init(FLOAT_COL *column, STATE_MEM_POOL *mem_pool) {
  assert(is_nan(FLOAT_NULL));
  assert(is_float_col_null(FLOAT_NULL));
  assert(!is_float_col_null(NORM_NAN));

  double *array = alloc_state_mem_float_array(mem_pool, INIT_SIZE);
  for (int i=0 ; i < INIT_SIZE ; i++)
    array[i] = FLOAT_NULL;
  column->array = array;
  column->capacity = INIT_SIZE;
  column->count = 0;
}

////////////////////////////////////////////////////////////////////////////////

uint32 float_col_size(FLOAT_COL *column) {
  return column->count;
}

////////////////////////////////////////////////////////////////////////////////

bool float_col_contains_1(FLOAT_COL *column, uint32 idx) {
  return idx < column->capacity && !is_float_col_null(column->array[idx]);
}

double float_col_lookup(FLOAT_COL *column, uint32 idx) {
  double value = column->array[idx];
  if (is_float_col_null(value))
    soft_fail(NULL); //## ADD A MESSAGE?
  return value;
}

////////////////////////////////////////////////////////////////////////////////

void float_col_insert(FLOAT_COL *column, uint32 idx, double value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    float_col_resize(column, idx + 1, mem_pool);

  double *array = column->array;
  double curr_value = array[idx];
  if (is_float_col_null(curr_value)) {
    column->count++;
    array[idx] = is_nan(value) ? NORM_NAN : value;
  }
  else if (curr_value != value) {
    soft_fail(NULL); //## ADD A MESSAGE?
  }
}

void float_col_update(FLOAT_COL *column, uint32 idx, double value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    float_col_resize(column, idx + 1, mem_pool);

  double *array = column->array;
  double curr_value = array[idx];

  if (is_float_col_null(curr_value))
    column->count++;

  array[idx] = is_nan(value) ? NORM_NAN : value;
}

bool float_col_delete(FLOAT_COL *column, uint32 idx, STATE_MEM_POOL *mem_pool) {
  if (idx < column->capacity) {
    double *array = column->array;
    double value = array[idx];
    if (!is_float_col_null(value)) {
      array[idx] = FLOAT_NULL;
      column->count--;
      return true;
    }
  }
  return false;
}

void float_col_clear(FLOAT_COL *column, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = column->capacity;
  //## MAYBE WE SHOULDN'T GO ALL THE WAY BACK DOWN TO THE INITIAL CAPACITY
  if (capacity != INIT_SIZE) {
    release_state_mem_float_array(mem_pool, column->array, capacity);
    float_col_init(column, mem_pool);
  }
  else if (column->count != 0) {
    //## IF THE NUMBER OF SET ENTRIES IS MUCH LOWER THAN THE CAPACITY,
    //## IT WOULD MAKE SENSE TO COUNT THE NUMBER OF ENTRIES THAT WE RESET
    column->count = 0;
    double *array = column->array;
    for (uint32 i=0 ; i < capacity ; i++)
      array[i] = FLOAT_NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////

void float_col_copy_to(FLOAT_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  uint32 capacity = col->capacity;
  uint32 remaining = col->count;
  double *array = col->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    double float_value = array[i];
    if (!is_float_col_null(float_value)) {
      OBJ key = surr_to_obj(store, i);
      OBJ value = make_float(float_value);
      append(*strm_1, key);
      append(*strm_2, value);
      remaining--;
    }
  }
}

void float_col_write(WRITE_FILE_STATE *write_state, FLOAT_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip) {
  uint32 capacity = col->capacity;
  uint32 remaining = col->count;
  double *array = col->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    double float_value = array[i];
    if (!is_float_col_null(float_value)) {
      OBJ key = surr_to_obj(store, i);
      OBJ value = make_float(float_value);
      write_str(write_state, "\n    ");
      write_obj(write_state, flip ? value : key);
      write_str(write_state, " -> ");
      write_obj(write_state, flip ? key : value);
      if (--remaining > 0)
        write_str(write_state, ",");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void slave_float_col_copy_to(MASTER_BIN_TABLE *master, FLOAT_COL *column, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  uint32 capacity = column->capacity;
  uint32 remaining = column->count;
  double *array = column->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    double float_value = array[i];
    if (!is_float_col_null(float_value)) {
      uint32 arg1 = master_bin_table_get_arg_1(master, i);
      uint32 arg2 = master_bin_table_get_arg_2(master, i);
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      OBJ obj3 = make_float(float_value);
      append(*strm_1, obj1);
      append(*strm_2, obj2);
      append(*strm_3, obj3);
      remaining--;
    }
  }
}

void slave_float_col_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master, FLOAT_COL *column, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, uint32 idx1, uint32 idx2, uint32 idx3) {
    assert(
      (idx1 == 0 && idx2 == 1 && idx3 == 2) ||
      (idx1 == 0 && idx2 == 2 && idx3 == 1) ||
      (idx1 == 1 && idx2 == 0 && idx3 == 2) ||
      (idx1 == 1 && idx2 == 2 && idx3 == 0) ||
      (idx1 == 2 && idx2 == 0 && idx3 == 1) ||
      (idx1 == 2 && idx2 == 1 && idx3 == 0)
    );

  uint32 capacity = column->capacity;
  uint32 count = column->count;
  uint32 remaining = count;
  double *array = column->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    double float_value = array[i];
    if (!is_float_col_null(float_value)) {
      uint32 arg1 = master_bin_table_get_arg_1(master, i);
      uint32 arg2 = master_bin_table_get_arg_2(master, i);
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      OBJ obj3 = make_float(float_value);

      remaining--;

      write_str(write_state, "\n    ");
      write_obj(write_state, idx1 == 0 ? obj1 : (idx2 == 0 ? obj2 : obj3));
      write_str(write_state, ", ");
      write_obj(write_state, idx1 == 1 ? obj1 : (idx2 == 1 ? obj2 : obj3));
      write_str(write_state, ", ");
      write_obj(write_state, idx1 == 2 ? obj1 : (idx2 == 2 ? obj2 : obj3));
      if (remaining > 0 | count == 1)
        write_str(write_state, ";");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void float_col_iter_init(FLOAT_COL *column, FLOAT_COL_ITER *iter) {
  double *array = column->array;
  uint32 count = column->count;

  uint32 idx = 0;
  if (count > 0)
    while (is_float_col_null(array[idx]))
      idx++;

  iter->array = column->array;
  iter->left = count;
  iter->idx = idx;
}

bool float_col_iter_is_out_of_range(FLOAT_COL_ITER *iter) {
  return iter->left == 0;
}

uint32 float_col_iter_get_idx(FLOAT_COL_ITER *iter) {
  assert(!float_col_iter_is_out_of_range(iter));
  return iter->idx;
}

double float_col_iter_get_value(FLOAT_COL_ITER *iter) {
  assert(!float_col_iter_is_out_of_range(iter));
  return iter->array[iter->idx];
}

void float_col_iter_move_forward(FLOAT_COL_ITER *iter) {
  assert(!float_col_iter_is_out_of_range(iter));
  if (--iter->left) {
    double *array = iter->array;
    uint32 idx = iter->idx;
    do
      idx++;
    while (is_float_col_null(array[idx]));
    iter->idx = idx;
  }
}
