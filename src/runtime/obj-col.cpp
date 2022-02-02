#include "lib.h"


const uint32 INIT_SIZE = 256;

void obj_col_init(OBJ_COL *column, STATE_MEM_POOL *mem_pool) {
  column->array = alloc_state_mem_blanked_obj_array(mem_pool, INIT_SIZE);
  column->capacity = INIT_SIZE;
  column->count = 0;
}

////////////////////////////////////////////////////////////////////////////////

static void obj_col_resize(OBJ_COL *column, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = column->capacity;
  uint32 new_capacity = 2 * capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;
  column->capacity = new_capacity;
  column->array = extend_state_mem_blanked_obj_array(mem_pool, column->array, capacity, new_capacity);
}

////////////////////////////////////////////////////////////////////////////////

bool obj_col_contains_1(OBJ_COL *column, uint32 idx) {
  return idx < column->capacity && !is_blank(column->array[idx]);
}

OBJ obj_col_lookup(OBJ_COL *column, uint32 idx) {
  OBJ obj = column->array[idx];
  if (is_blank(obj))
    soft_fail(NULL); //## ADD A MESSAGE?
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

void obj_col_insert(OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    obj_col_resize(column, idx + 1, mem_pool);
  OBJ *array = column->array;
  OBJ curr_value = array[idx];
  if (is_blank(curr_value)) {
    column->count++;
    array[idx] = copy_to_pool(mem_pool, value);
  }
  else if (!are_eq(curr_value, value)) {
    soft_fail(NULL); //## ADD A MESSAGE?
  }
}

void obj_col_update(OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    obj_col_resize(column, idx + 1, mem_pool);
  OBJ *array = column->array;
  OBJ curr_value = array[idx];
  //## SHOULD WE BE CHECKING THAT curr_value AND value ARE NOT EQUAL BEFORE PROCEEDING?
  array[idx] = copy_to_pool(mem_pool, value);
  if (is_blank(curr_value))
    column->count++;
  else
    remove_from_pool(mem_pool, curr_value);
}

void obj_col_delete(OBJ_COL *column, uint32 idx, STATE_MEM_POOL *mem_pool) {
  if (idx < column->capacity) {
    OBJ *array = column->array;
    OBJ obj = array[idx];
    if (!is_blank(obj)) {
      array[idx] = make_blank_obj();
      column->count--;
      remove_from_pool(mem_pool, obj);
    }
  }
}

void obj_col_clear(OBJ_COL *column, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = column->capacity;
  //## MAYBE WE SHOULDN'T GO ALL THE WAY BACK DOWN TO THE INITIAL CAPACITY
  if (capacity != INIT_SIZE) {
    release_state_mem_obj_array(mem_pool, column->array, capacity);
    obj_col_init(column, mem_pool);
  }
  else if (column->count != 0) {
    //## IF THE NUMBER OF SET ENTRIES IS MUCH LOWER THAN THE CAPACITY,
    //## IT WOULD MAKE SENSE TO COUNT THE NUMBER OF ENTRIES THAT WE RESET
    column->count = 0;
    memset(column->array, 0, capacity);
    for (int i=0 ; i < capacity ; i++)
      assert(is_blank(column->array[i]));
  }
}

////////////////////////////////////////////////////////////////////////////////

void obj_col_copy_to(OBJ_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  uint32 capacity = col->capacity;
  uint32 remaining = col->count;
  OBJ *array = col->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    OBJ value = array[i];
    if (!is_blank(value)) {
      OBJ key = surr_to_obj(store, i);
      append(*strm_1, key);
      append(*strm_2, value);
      remaining--;
    }
  }
}

void obj_col_write(WRITE_FILE_STATE *write_state, OBJ_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip) {
  uint32 capacity = col->capacity;
  uint32 remaining = col->count;
  OBJ *array = col->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    OBJ value = array[i];
    if (!is_blank(value)) {
      OBJ key = surr_to_obj(store, i);
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

void obj_col_iter_init(OBJ_COL *column, OBJ_COL_ITER *iter) {
  OBJ *array = column->array;
  uint32 count = column->count;

  uint32 idx = 0;
  if (count > 0)
    while (is_blank(array[idx]))
      idx++;

  iter->array = column->array;
  iter->left = count;
  iter->idx = idx;
}

bool obj_col_iter_is_out_of_range(OBJ_COL_ITER *iter) {
  return iter->left <= 0;
}

uint32 obj_col_iter_get_idx(OBJ_COL_ITER *iter) {
  assert(!obj_col_iter_is_out_of_range(iter));
  return iter->idx;
}

OBJ obj_col_iter_get_value(OBJ_COL_ITER *iter) {
  assert(!obj_col_iter_is_out_of_range(iter));
  return iter->array[iter->idx];
}

void obj_col_iter_move_forward(OBJ_COL_ITER *iter) {
  assert(!obj_col_iter_is_out_of_range(iter));
  if (--iter->left) {
    OBJ *array = iter->array;
    uint32 idx = iter->idx;
    do
      idx++;
    while (is_blank(array[idx]));
    iter->idx = idx;
  }
}
