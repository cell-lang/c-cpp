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
  return column->collisions.count(idx) != 0;
}

inline bool is_set_at(INT_COL *column, uint32 idx, int64 value) {
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
      column->collisions.insert(idx);
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
      column->collisions.insert(idx);
  }
  else if (column->collisions.count(idx) != 0) {
    // The existing value happens to be INT_NULL
    if (value != INT_NULL) {
      array[idx] = value;
      column->collisions.erase(idx);
    }
  }
  else {
    // There's no existing value
    column->count++;
    array[idx] = value;
    if (value == INT_NULL)
      column->collisions.insert(idx);
  }
}

void int_col_delete(INT_COL *column, uint32 idx, STATE_MEM_POOL *mem_pool) {
  if (idx < column->capacity) {
    int64 *array = column->array;
    int64 value = array[idx];
    if (value != INT_NULL) {
      array[idx] = INT_NULL;
      column->count--;
    }
    else if (is_collision(column, idx)) {
      column->collisions.erase(idx);
      column->count--;
    }
  }
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
    column->collisions.clear();
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

void int_col_iter_init(INT_COL *column, INT_COL_ITER *iter) {
  int64 *array = column->array;
  uint32 count = column->count;

  uint32 idx = 0;
  if (count > 0)
    while (!is_set_at(column, idx, array[idx]))
      idx++;

  iter->array = column->array;
  iter->left = count;
  iter->idx = idx;
  iter->collisions = &column->collisions;
}

bool int_col_iter_is_out_of_range(INT_COL_ITER *iter) {
  return iter->left <= 0;
}

uint32 int_col_iter_get_idx(INT_COL_ITER *iter) {
  assert(!int_col_iter_is_out_of_range(iter));
  return iter->idx;
}

int64 int_col_iter_get_value(INT_COL_ITER *iter) {
  assert(!int_col_iter_is_out_of_range(iter));
  return iter->array[iter->idx];
}

void int_col_iter_move_forward(INT_COL_ITER *iter) {
  assert(!int_col_iter_is_out_of_range(iter));
  if (--iter->left) {
    int64 *array = iter->array;
    uint32 idx = iter->idx;
    do
      idx++;
    while (array[idx] == INT_NULL && iter->collisions->count(idx) == 0);
    iter->idx = idx;
  }
}
