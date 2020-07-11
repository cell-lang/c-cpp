#include "lib.h"


int64 get_inner_long(OBJ obj) {
  return get_int(get_inner_obj(obj));
}

////////////////////////////////////////////////////////////////////////////////

OBJ* get_obj_array(OBJ seq, OBJ* buffer, int32 size) {
  assert(get_seq_length(seq) == size);

  if (is_empty_seq(seq))
    return NULL;

  OBJ_TYPE type = get_physical_type(seq);

  if (type == TYPE_NE_SEQ | type == TYPE_NE_SLICE)
    return get_seq_elts_ptr(seq);

  uint32 len = get_seq_length(seq);

  if (buffer == NULL)
    buffer = new_obj_array(len);

  uint8 *elts = get_seq_elts_ptr_uint8(seq);

  for (int i=0 ; i < len ; i++)
    buffer[i] = make_int(elts[i]);

  return buffer;
}

int64* get_long_array(OBJ seq, int64 *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;
  int len = get_size(seq);
  if (buffer == NULL | size < len)
    buffer = new_int64_array(len);
  OBJ_TYPE type = get_physical_type(seq);
  if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8) {
    uint8 *elts = get_seq_elts_ptr_uint8(seq);
    for (int i=0 ; i < len ; i++)
      buffer[i] = elts[i];
  }
  else {
    OBJ *seq_buffer = get_seq_elts_ptr(seq);
    for (int i=0 ; i < len ; i++)
      buffer[i] = get_int(seq_buffer[i]);
  }
  return buffer;
}

double* get_double_array(OBJ seq, double *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;
  int len = get_size(seq);
  if (buffer == NULL | size < len)
    buffer = new_double_array(len);
  OBJ *seq_buffer = get_seq_elts_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_float(seq_buffer[i]);
  return buffer;
}

bool* get_bool_array(OBJ seq, bool *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;
  int len = get_size(seq);
  if (buffer == NULL | size < len)
    buffer = new_bool_array(len);
  OBJ *seq_buffer = get_seq_elts_ptr(seq);
  for (int i=0 ; i < len ; i++)
    buffer[i] = get_bool(seq_buffer[i]);
  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_seq(int64* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  int64 min = 0;
  int64 max = 127;

  for (uint32 i=0 ; i < size ; i++) {
    int64 elt = array[i];
    if (elt < min)
      min = elt;
    if (elt > max)
      max = elt;
  }

  if (min == 0 & max <= 255) {
    uint8 *uint8_array = (uint8 *) array;
    for (uint32 i=0 ; i < size ; i++)
      uint8_array[i] = array[i];
    return make_slice_uint8(uint8_array, size);
  }

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int8* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  for (int i=0 ; i < size ; i++)
    if (array[i] < 0) {
      SEQ_OBJ *seq = new_obj_seq(size);

      for (uint32 i=0 ; i < size ; i++)
        seq->buffer.obj[i] = make_int(array[i]);

      return make_seq(seq, size);
    }
  return make_slice_uint8((uint8 *) array, size);
}

OBJ build_seq(uint8 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  return make_slice_uint8(array, size);

  // SEQ_OBJ *seq = new_obj_seq(size);

  // for (uint32 i=0 ; i < size ; i++)
  //   seq->buffer.obj[i] = make_int(array[i]);

  // return make_seq(seq, size);
}

OBJ build_seq(bool* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_bool(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(double* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_float(array[i]);

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

double get_float_at(OBJ seq, int64 idx) {
  return get_float(at(seq, idx));
}

int64 get_int_at(OBJ seq, int64 idx) {
  return get_int(at(seq, idx));
}

bool is_ne_int_seq(OBJ obj) {
  if (!is_ne_seq(obj))
    return false;

  int len = get_size(obj);
  OBJ *elts = get_seq_elts_ptr(obj);
  for (int i=0 ; i < len ; i++)
    if (!is_int(elts[i]))
      return false;

  return true;
}

bool is_ne_float_seq(OBJ obj) {
  if (!is_ne_seq(obj))
    return false;

  int len = get_size(obj);
  OBJ *elts = get_seq_elts_ptr(obj);
  for (int i=0 ; i < len ; i++)
    if (!is_float(elts[i]))
      return false;

  return true;
}
