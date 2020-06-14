#include "lib.h"


int32 new_size(int32 curr_size, int32 min_size) {
  int32 new_size = 2 * curr_size;
  while (new_size < min_size)
    new_size *= 2;
  return new_size;
}

////////////////////////////////////////////////////////////////////////////////

int64 get_inner_long(OBJ obj) {
  return get_int(get_inner_obj(obj));
}

////////////////////////////////////////////////////////////////////////////////

OBJ* get_obj_array(OBJ seq, OBJ* buffer, int32 size) {
  return is_empty_seq(seq) ? NULL : get_seq_buffer_ptr(seq);
}

int64* get_long_array(OBJ seq, int64 *buffer, int32 size) {
  int len = get_size(seq);
  if (buffer == NULL | size < len)
    buffer = new_int64_array(len);
  OBJ *seq_buffer = get_seq_buffer_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_int(seq_buffer[i]);
  return buffer;
}

double* get_double_array(OBJ seq, double *buffer, int32 size) {
  int len = get_size(seq);
  if (buffer == NULL | size < len)
    buffer = new_double_array(len);
  OBJ *seq_buffer = get_seq_buffer_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_float(seq_buffer[i]);
  return buffer;
}

bool* get_bool_array(OBJ seq, bool *buffer, int32 size) {
  int len = get_size(seq);
  if (buffer == NULL | size < len)
    buffer = new_bool_array(len);
  OBJ *seq_buffer = get_seq_buffer_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_bool(seq_buffer[i]);
  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_seq(int64* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int8* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint8* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(bool* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_bool(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(double* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_float(array[i]);

  return make_seq(seq, size);
}

OBJ build_record(uint16 *labels, OBJ *values, int32 count) {
  OBJ buffer[1024];
  if (count > 1024)
    impl_fail("Record with more than 1024 fields");
  for (int i=0 ; i < count ; i++)
    buffer[i] = make_symb(labels[i]);
  OBJ record = build_map(buffer, values, count);
}

double float_pow(double base, double exp) {
  return pow(base, exp);
}

double float_sqrt(double x) {
  return sqrt(x);
}

int64 float_round(double x) {
  return (int64) x;
}

int32 cast_int32(int64 val64) {
  int32 val32 = (int32) val64;
  if (val32 != val64)
    soft_fail("Invalid 64 to 32 bit integer conversion");
  return val32;
}

OBJ set_insert(OBJ set, OBJ elt) {
  impl_fail("Not implemented yet");
}

OBJ set_key_value(OBJ map, OBJ key, OBJ value) {
  impl_fail("Not implemented yet");
}

OBJ make_tag_int(uint16 tag, int64 value) {
  return make_tag_obj(tag, make_int(value));
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

double get_float_at(OBJ seq, int64 idx) {
  return get_float(at(seq, idx));
}

int64 get_int_at(OBJ seq, int64 idx) {
  return get_int(at(seq, idx));
}

////////////////////////////////////////////////////////////////////////////////

OBJ *array_append(OBJ *array, int32 size, int32 &capacity, OBJ elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    OBJ *new_array = new_obj_array(capacity);
    memcpy(new_array, array, size * sizeof(OBJ));
    array = new_array;
  }
  array[size] = elt;
}

bool *array_append(bool *array, int32 size, int32 &capacity, bool elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    bool *new_array = new_bool_array(capacity);
    memcpy(new_array, array, size * sizeof(bool));
    array = new_array;
  }
  array[size] = elt;
}

double *array_append(double *array, int32 size, int32 &capacity, double elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    double *new_array = new_double_array(capacity);
    memcpy(new_array, array, size * sizeof(double));
    array = new_array;
  }
  array[size] = elt;
}

int64 *array_append(int64 *array, int32 size, int32 &capacity, int64 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    int64 *new_array = new_int64_array(capacity);
    memcpy(new_array, array, size * sizeof(int64));
    array = new_array;
  }
  array[size] = elt;
}

int32 *array_append(int32 *array, int32 size, int32 &capacity, int32 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    int32 *new_array = new_int32_array(capacity);
    memcpy(new_array, array, size * sizeof(int32));
    array = new_array;
  }
  array[size] = elt;
}

uint32 *array_append(uint32 *array, int32 size, int32 &capacity, uint32 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    uint32 *new_array = new_uint32_array(capacity);
    memcpy(new_array, array, size * sizeof(uint32));
    array = new_array;
  }
  array[size] = elt;
}

int16 *array_append(int16 *array, int32 size, int32 &capacity, int16 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    int16 *new_array = new_int16_array(capacity);
    memcpy(new_array, array, size * sizeof(int16));
    array = new_array;
  }
  array[size] = elt;
}

uint16 *array_append(uint16 *array, int32 size, int32 &capacity, uint16 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    uint16 *new_array = new_uint16_array(capacity);
    memcpy(new_array, array, size * sizeof(uint16));
    array = new_array;
  }
  array[size] = elt;
}

int8 *array_append(int8 *array, int32 size, int32 &capacity, int8 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    int8 *new_array = new_int8_array(capacity);
    memcpy(new_array, array, size * sizeof(int8));
    array = new_array;
  }
  array[size] = elt;
}

uint8 *array_append(uint8 *array, int32 size, int32 &capacity, uint8 elt) {
  if (size == capacity) {
    capacity = new_size(capacity, size);
    uint8 *new_array = new_uint8_array(capacity);
    memcpy(new_array, array, size * sizeof(uint8));
    array = new_array;
  }
  array[size] = elt;
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
