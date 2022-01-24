#include "lib.h"



void obj_col_init(OBJ_COL *column) {
  const uint32 INIT_SIZE = 256;

  impl_fail(""); //## IMPLEMENT IMPLEMENT IMPLEMENT
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

void obj_col_insert(OBJ_COL *column, uint32 idx, OBJ value) {
  if (idx < column->capacity) {
    // column = Array.extend(column, Array.capacity(column.length, idx+1));
    impl_fail(""); //## IMPLEMENT IMPLEMENT IMPLEMENT
  }
  OBJ *array = column->array;
  OBJ curr_value = array[idx];
  if (is_blank(curr_value)) {
    column->count++;
    array[idx] = value; //## DOES THIS HAVE TO BE COPIED NOW INTO LONG-TERM MEMORY?
  }
  else if (!are_eq(curr_value, value)) {
    soft_fail(NULL); //## ADD A MESSAGE?
  }
}

void obj_col_update(OBJ_COL *column, uint32 idx, OBJ value) {
  if (idx < column->capacity) {
    // column = Array.extend(column, Array.capacity(column.length, idx+1));
    impl_fail(""); //## IMPLEMENT IMPLEMENT IMPLEMENT
  }
  OBJ *array = column->array;
  OBJ curr_value = array[idx];
  if (is_blank(curr_value)) {
    column->count++;
  }
  else {
    //## BUG BUG BUG: RELEASE THE CURRENT VALUE
  }
  array[idx] = value; //## DOES THIS HAVE TO BE COPIED NOW INTO LONG-TERM MEMORY?
}

void obj_col_delete(OBJ_COL *column, uint32 idx) {
  if (idx < column->capacity) {
    OBJ *array = column->array;
    OBJ obj = array[idx];
    if (!is_blank(obj)) {
      //## RELEASE THE OBJECT HERE
      array[idx] = make_blank_obj();
      column->count--;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void obj_col_init_iter(OBJ_COL *column, OBJ_COL_ITER *iter) {
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
