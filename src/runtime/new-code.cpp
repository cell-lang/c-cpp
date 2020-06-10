#include "lib.h"


int64 get_inner_long(OBJ obj) {
  return get_int(get_inner_obj(obj));
}

////////////////////////////////////////////////////////////////////////////////

OBJ* get_obj_array(OBJ seq, OBJ* buffer, int32 size) {
  return get_seq_buffer_ptr(seq);
}

int64* get_long_array(OBJ seq, int64 *buffer, int32 size) {
  int len = get_size(seq);
  if (buffer == NULL | size < len) {
    //## IMPLEMENT
    internal_fail();
  }
  OBJ *seq_buffer = get_seq_buffer_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_int(seq_buffer[i]);
  return buffer;
}

double* get_double_array(OBJ seq, double *buffer, int32 size) {
  int len = get_size(seq);
  if (buffer == NULL | size < len) {
    //## IMPLEMENT
    internal_fail();
  }
  OBJ *seq_buffer = get_seq_buffer_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_float(seq_buffer[i]);
  return buffer;
}

bool* get_bool_array(OBJ seq, bool *buffer, int32 size) {
  int len = get_size(seq);
  if (buffer == NULL | size < len) {
    //## IMPLEMENT
    internal_fail();
  }
  OBJ *seq_buffer = get_seq_buffer_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_bool(seq_buffer[i]);
  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

void* get_void_ptr(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_OPT_TAG_REC);
  return obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

bool bin_rel_contains_1(OBJ rel, OBJ arg1) {
  BIN_REL_ITER it;
  get_bin_rel_iter_0(it, rel, arg1);
  return !is_out_of_range(it);
}

bool bin_rel_contains_2(OBJ rel, OBJ arg2) {
  BIN_REL_ITER it;
  get_bin_rel_iter_1(it, rel, arg2);
  return !is_out_of_range(it);
}

bool tern_rel_contains_1(OBJ rel, OBJ arg1) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 0, arg1);
  return !is_out_of_range(it);
}

bool tern_rel_contains_2(OBJ rel, OBJ arg2) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 1, arg2);
  return !is_out_of_range(it);
}

bool tern_rel_contains_3(OBJ rel, OBJ arg3) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 2, arg3);
  return !is_out_of_range(it);
}

bool tern_rel_contains_12(OBJ rel, OBJ arg1, OBJ arg2) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 0, arg1, arg2);
  return !is_out_of_range(it);
}

bool tern_rel_contains_13(OBJ rel, OBJ arg1, OBJ arg3) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 2, arg3, arg1);
  return !is_out_of_range(it);
}

bool tern_rel_contains_23(OBJ rel, OBJ arg2, OBJ arg3) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 1, arg2, arg3);
  return !is_out_of_range(it);
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_seq(int64* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int32* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint32* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int16* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint16* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int8* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint8* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(bool* array, int32 size) {
  SEQ_OBJ *seq = new_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer[i] = make_bool(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(double* array, int32 size) {
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

OBJ set_insert(OBJ, OBJ) {
  impl_fail("Not implemented yet");
}

OBJ set_key_value(OBJ, OBJ, OBJ) {
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
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

bool *array_append(bool *array, int32 size, int32 &capacity, bool elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

double *array_append(double *array, int32 size, int32 &capacity, double elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

int64 *array_append(int64 *array, int32 size, int32 &capacity, int64 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

int32 *array_append(int32 *array, int32 size, int32 &capacity, int32 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

uint32 *array_append(uint32 *array, int32 size, int32 &capacity, uint32 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

int16 *array_append(int16 *array, int32 size, int32 &capacity, int16 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

uint16 *array_append(uint16 *array, int32 size, int32 &capacity, uint16 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

int8 *array_append(int8 *array, int32 size, int32 &capacity, int8 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
  }
  array[size] = elt;
}

uint8 *array_append(uint8 *array, int32 size, int32 &capacity, uint8 elt) {
  if (size == capacity) {
    impl_fail("Not implemented yet");
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
