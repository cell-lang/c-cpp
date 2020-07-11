#include "lib.h"



static uint32 next_array_size(uint32 curr_size, uint32 min_size) {
  uint32 new_size = curr_size != 0 ? 2 * curr_size : 32;
  while (new_size < min_size)
    new_size *= 2;
  return new_size;
}

////////////////////////////////////////////////////////////////////////////////

uint8 as_byte(int64 value) {
  return (uint8) value;
}

int16 as_short(int64 value) {
  return (int16) value;
}

int32 as_int(int64 value) {
  return (int32) value;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *array_append(OBJ *array, int32 size, int32 &capacity, OBJ elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    OBJ *new_array = new_obj_array(capacity);
    memcpy(new_array, array, size * sizeof(OBJ));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

bool *array_append(bool *array, int32 size, int32 &capacity, bool elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    bool *new_array = new_bool_array(capacity);
    memcpy(new_array, array, size * sizeof(bool));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

double *array_append(double *array, int32 size, int32 &capacity, double elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    double *new_array = new_double_array(capacity);
    memcpy(new_array, array, size * sizeof(double));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

int64 *array_append(int64 *array, int32 size, int32 &capacity, int64 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    int64 *new_array = new_int64_array(capacity);
    memcpy(new_array, array, size * sizeof(int64));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

int32 *array_append(int32 *array, int32 size, int32 &capacity, int32 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    int32 *new_array = new_int32_array(capacity);
    memcpy(new_array, array, size * sizeof(int32));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

uint32 *array_append(uint32 *array, int32 size, int32 &capacity, uint32 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    uint32 *new_array = new_uint32_array(capacity);
    memcpy(new_array, array, size * sizeof(uint32));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

int16 *array_append(int16 *array, int32 size, int32 &capacity, int16 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    int16 *new_array = new_int16_array(capacity);
    memcpy(new_array, array, size * sizeof(int16));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

uint16 *array_append(uint16 *array, int32 size, int32 &capacity, uint16 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    uint16 *new_array = new_uint16_array(capacity);
    memcpy(new_array, array, size * sizeof(uint16));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

int8 *array_append(int8 *array, int32 size, int32 &capacity, int8 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    int8 *new_array = new_int8_array(capacity);
    memcpy(new_array, array, size * sizeof(int8));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

uint8 *array_append(uint8 *array, int32 size, int32 &capacity, uint8 elt) {
  if (size == capacity) {
    capacity = next_array_size(capacity, size);
    uint8 *new_array = new_uint8_array(capacity);
    memcpy(new_array, array, size * sizeof(uint8));
    array = new_array;
  }
  array[size] = elt;
  return array;
}

////////////////////////////////////////////////////////////////////////////////

OBJ array_at(OBJ *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

OBJ array_at(bool *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_bool(array[idx]);
}

OBJ array_at(double *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_float(array[idx]);
}

OBJ array_at(int64 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}

OBJ array_at(int32 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}

OBJ array_at(uint32 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}

OBJ array_at(int16 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}

OBJ array_at(uint16 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}

OBJ array_at(int8 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}

OBJ array_at(uint8 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return make_int(array[idx]);
}


bool bool_array_at(bool *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

double float_array_at(double *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}


int64 int_array_at(int64 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

int64 int_array_at(int32 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

int64 int_array_at(uint32 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

int64 int_array_at(int16 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

int64 int_array_at(uint16 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

int64 int_array_at(int8 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}

int64 int_array_at(uint8 *array, int32 len, int32 idx) {
  if (idx < 0 | idx >= len)
    soft_fail("Out of bound array access");
  return array[idx];
}
