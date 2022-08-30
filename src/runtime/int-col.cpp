#include "lib.h"


const uint32 INIT_SIZE = 256;

// const int64 INT_NULL = -9223372036854775808L;
const int64 INT_NULL = -5091454680840284659L; // Random null value


void int_col_init(INT_COL *column, STATE_MEM_POOL *mem_pool) {
  int64 *array = alloc_state_mem_int64_array(mem_pool, INIT_SIZE);
  for (int i=0 ; i < INIT_SIZE ; i++)
    array[i] = INT_NULL;
  column->array = array;
  column->capacity = INIT_SIZE;
  column->count = 0;
  bit_map_init(&column->collisions);
}

////////////////////////////////////////////////////////////////////////////////

static void int_col_resize(INT_COL *column, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = column->capacity;
  uint32 new_capacity = 2 * capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;
  column->capacity = new_capacity;
  int64 *array = extend_state_mem_int64_array(mem_pool, column->array, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    array[i] = INT_NULL;
  column->array = array;
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_collision(INT_COL *column, uint32 idx) {
  assert(idx < column->capacity && column->array[idx] == INT_NULL);
  return bit_map_is_set(&column->collisions, idx);
}

bool is_set_at(INT_COL *column, uint32 idx, int64 value) {
  return value != INT_NULL || is_collision(column, idx);
}

inline bool is_set_at(INT_COL *column, uint32 idx) {
  assert(idx < column->capacity);
  return is_set_at(column, idx, column->array[idx]);
}

////////////////////////////////////////////////////////////////////////////////

uint32 int_col_size(INT_COL *column) {
  return column->count;
}

////////////////////////////////////////////////////////////////////////////////

bool int_col_contains_1(INT_COL *column, uint32 idx) {
  return idx < column->capacity && is_set_at(column, idx);
}

int64 int_col_lookup(INT_COL *column, uint32 idx) {
  int64 value = column->array[idx];
  if (!is_set_at(column, idx, value))
    soft_fail(NULL); //## ADD A MESSAGE?
  return value;
}

////////////////////////////////////////////////////////////////////////////////

void int_col_insert(INT_COL *column, uint32 idx, int64 value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    int_col_resize(column, idx + 1, mem_pool);

  int64 *array = column->array;
  int64 curr_value = array[idx];
  if (!is_set_at(column, idx, curr_value)) {
    column->count++;
    array[idx] = value;
    if (value == INT_NULL)
      bit_map_set(&column->collisions, idx, mem_pool);
  }
  else if (curr_value != value) {
    soft_fail(NULL); //## ADD A MESSAGE?
  }
}

void int_col_update(INT_COL *column, uint32 idx, int64 value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    int_col_resize(column, idx + 1, mem_pool);
  int64 *array = column->array;
  int64 curr_value = array[idx];

  if (curr_value != INT_NULL) {
    // There is an existing value, and it's not INT_NULL
    array[idx] = value;
    if (value == INT_NULL)
      bit_map_set(&column->collisions, idx, mem_pool);
  }
  else if (bit_map_is_set(&column->collisions, idx)) {
    // The existing value happens to be INT_NULL
    if (value != INT_NULL) {
      array[idx] = value;
      bit_map_clear(&column->collisions, idx);
    }
  }
  else {
    // There's no existing value
    column->count++;
    array[idx] = value;
    if (value == INT_NULL)
      bit_map_set(&column->collisions, idx, mem_pool);
  }
}

bool int_col_delete(INT_COL *column, uint32 idx, STATE_MEM_POOL *mem_pool) {
  if (idx < column->capacity) {
    int64 *array = column->array;
    int64 value = array[idx];
    if (value != INT_NULL) {
      array[idx] = INT_NULL;
      column->count--;
      return true;
    }
    else if (is_collision(column, idx)) {
      bit_map_clear(&column->collisions, idx);
      column->count--;
      return true;
    }
  }
  return false;
}

void int_col_clear(INT_COL *column, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = column->capacity;
  //## MAYBE WE SHOULDN'T GO ALL THE WAY BACK DOWN TO THE INITIAL CAPACITY
  if (capacity != INIT_SIZE) {
    release_state_mem_int64_array(mem_pool, column->array, capacity);
    int_col_init(column, mem_pool);
  }
  else if (column->count != 0) {
    //## IF THE NUMBER OF SET ENTRIES IS MUCH LOWER THAN THE CAPACITY,
    //## IT WOULD MAKE SENSE TO COUNT THE NUMBER OF ENTRIES THAT WE RESET
    column->count = 0;
    int64 *array = column->array;
    for (uint32 i=0 ; i < capacity ; i++)
      array[i] = INT_NULL;
    bit_map_clear(&column->collisions, mem_pool);
  }
}

////////////////////////////////////////////////////////////////////////////////

void int_col_copy_to(INT_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  uint32 capacity = col->capacity;
  uint32 remaining = col->count;
  int64 *array = col->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    int64 int_value = array[i];
    if (is_set_at(col, i, int_value)) {
      OBJ key = surr_to_obj(store, i);
      OBJ value = make_int(int_value);
      append(*strm_1, key);
      append(*strm_2, value);
      remaining--;
    }
  }
}

void int_col_write(WRITE_FILE_STATE *write_state, INT_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip) {
  uint32 capacity = col->capacity;
  uint32 remaining = col->count;
  int64 *array = col->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    int64 int_value = array[i];
    if (is_set_at(col, i, int_value)) {
      OBJ key = surr_to_obj(store, i);
      OBJ value = make_int(int_value);
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

void slave_int_col_copy_to(MASTER_BIN_TABLE *master, INT_COL *column, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  uint32 capacity = column->capacity;
  uint32 remaining = column->count;
  int64 *array = column->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    int64 value = array[i];
    if (is_set_at(column, i, value)) {
      uint32 arg1 = master_bin_table_get_arg_1(master, i);
      uint32 arg2 = master_bin_table_get_arg_2(master, i);
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      OBJ obj3 = make_int(value);
      append(*strm_1, obj1);
      append(*strm_2, obj2);
      append(*strm_3, obj3);
      remaining--;
    }
  }
}

void slave_int_col_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master, INT_COL *column, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, uint32 idx1, uint32 idx2, uint32 idx3) {
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
  int64 *array = column->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    int64 value = array[i];
    if (is_set_at(column, i, value)) {
      uint32 arg1 = master_bin_table_get_arg_1(master, i);
      uint32 arg2 = master_bin_table_get_arg_2(master, i);
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      OBJ obj3 = make_int(value);

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
