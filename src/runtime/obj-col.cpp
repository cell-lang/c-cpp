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

uint32 obj_col_size(OBJ_COL *column) {
  return column->count;
}

////////////////////////////////////////////////////////////////////////////////

bool obj_col_contains_1(OBJ_COL *column, uint32 idx) {
  return idx < column->capacity && !is_blank(column->array[idx]);
}

OBJ obj_col_lookup(OBJ_COL *column, uint32 idx) {
  //## BUG BUG BUG: CHECK THE CAPACITY HERE. THE SAME BUG IS ALSO PRESENT IN THE OTHER TYPES OF COLUMNS
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

void obj_col_insert_no_copy(OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    obj_col_resize(column, idx + 1, mem_pool);
  OBJ *array = column->array;
  OBJ curr_value = array[idx];
  if (is_blank(curr_value)) {
    column->count++;
    array[idx] = value;
  }
  else if (!are_eq(curr_value, value)) {
    soft_fail(NULL); //## ADD A MESSAGE?
  }
}

void obj_col_update_no_copy(OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *mem_pool) {
  if (idx >= column->capacity)
    obj_col_resize(column, idx + 1, mem_pool);
  OBJ *array = column->array;
  OBJ curr_value = array[idx];
  //## SHOULD WE BE CHECKING THAT curr_value AND value ARE NOT EQUAL BEFORE PROCEEDING?
  array[idx] = value;
  if (is_blank(curr_value))
    column->count++;
  else
    remove_from_pool(mem_pool, curr_value);
}

bool obj_col_delete(OBJ_COL *column, uint32 idx, STATE_MEM_POOL *mem_pool) {
  if (idx < column->capacity) {
    OBJ *array = column->array;
    OBJ obj = array[idx];
    if (!is_blank(obj)) {
      array[idx] = make_blank_obj();
      column->count--;
      remove_from_pool(mem_pool, obj);
      return true;
    }
  }
  return false;
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

void slave_obj_col_copy_to(MASTER_BIN_TABLE *master, OBJ_COL *column, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  uint32 capacity = column->capacity;
  uint32 remaining = column->count;
  OBJ *array = column->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    OBJ obj3 = array[i];
    if (!is_blank(obj3)) {
      uint32 arg1 = master_bin_table_get_arg_1(master, i);
      uint32 arg2 = master_bin_table_get_arg_2(master, i);
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      append(*strm_1, obj1);
      append(*strm_2, obj2);
      append(*strm_3, obj3);
      remaining--;
    }
  }
}

void slave_obj_col_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master, OBJ_COL *column, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, uint32 idx1, uint32 idx2, uint32 idx3) {
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
  OBJ *array = column->array;
  for (uint32 i=0 ; remaining > 0 ; i++) {
    assert(i < capacity);
    OBJ obj3 = array[i];
    if (!is_blank(obj3)) {
      uint32 arg1 = master_bin_table_get_arg_1(master, i);
      uint32 arg2 = master_bin_table_get_arg_2(master, i);
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);

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
