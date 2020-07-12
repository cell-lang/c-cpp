#include "lib.h"


OBJ get_obj_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));
  if (((uint64) idx) >= get_seq_length(seq))
    soft_fail("Invalid sequence index");
  OBJ_TYPE type = get_physical_type(seq);
  if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8)
    return make_int(get_seq_elts_ptr_uint8(seq)[idx]);
  return get_seq_elts_ptr(seq)[idx];
}

double get_float_at(OBJ seq, int64 idx) {
  return get_float(at(seq, idx));
}

int64 get_int_at(OBJ seq, int64 idx) {
  return get_int(at(seq, idx));
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

OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len) {
  assert(is_seq(seq));

  if (idx_first < 0 | len < 0 | idx_first + len > get_seq_length(seq))
    soft_fail("_slice_(): Invalid start index and/or subsequence length");

  if (len == 0)
    return make_empty_seq();

  // if (idx_first == 0) {
  //   ## IMPLEMENT
  // }

  OBJ_TYPE type = get_physical_type(seq);
  if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8) {
    uint8 *elts = get_seq_elts_ptr_uint8(seq);
    return make_slice_uint8(elts + idx_first, len);
  }
  else {
    OBJ *ptr = get_seq_elts_ptr(seq);
    return make_slice(ptr + idx_first, len);
  }
}

OBJ append_to_seq(OBJ seq, OBJ obj) {
  if (is_empty_seq(seq)) {
    if (is_int(obj)) {
      int64 value = get_int(obj);
      if (value >= 0 & value <= 255) {
        SEQ_OBJ *seq_ptr = new_uint8_seq(1, 16);
        seq_ptr->buffer.uint8_[0] = value;
        return make_seq_uint8(seq_ptr, 1);
      }
    }

    OBJ *ptr = new_obj_array(1);
    ptr[0] = obj;
    return make_slice(ptr, 1);

    // SEQ_OBJ *seq_ptr = new_obj_seq(1, 2);
    // seq_ptr->buffer.obj[0] = obj;
    // return make_seq(seq_ptr, 1);
  }

  uint32 len = get_seq_length(seq);

  // Checking that the new sequence doesn't overflow
  if (len == 0xFFFFFFFF)
    impl_fail("Resulting sequence is too large");

  switch (get_physical_type(seq)) {
    case TYPE_NE_SEQ: {
      return in_place_concat_obj(get_seq_ptr(seq), len, &obj, 1);
    }

    case TYPE_NE_SLICE: {
      OBJ *elts = get_seq_elts_ptr(seq);
      if (len == 1) {
        OBJ *new_elts = new_obj_array(2);
        new_elts[0] = elts[0];
        new_elts[1] = obj;
        return make_slice(new_elts, 2);
      }
      if (len == 2) {
        OBJ *new_elts = new_obj_array(3);
        new_elts[0] = elts[0];
        new_elts[1] = elts[1];
        new_elts[2] = obj;
        return make_slice(new_elts, 3);
      }
      if (len == 3) {
        SEQ_OBJ *seq_ptr = new_obj_seq(4, 8);
        OBJ *new_elts = seq_ptr->buffer.obj;
        new_elts[0] = elts[0];
        new_elts[1] = elts[1];
        new_elts[2] = elts[2];
        new_elts[3] = obj;
        return make_seq(seq_ptr, 4);
      }
      return concat_obj(elts, len, &obj, 1);
    }

    case TYPE_NE_SEQ_UINT8: {
      if (is_int(obj)) {
        int64 value = get_int(obj);
        if (value >= 0 & value < 255) {
          uint8 value_uint8 = (uint8) value;
          return in_place_concat_uint8(get_seq_ptr(seq), len, &value_uint8, 1);
        }
      }

      return concat_uint8_obj(get_seq_elts_ptr_uint8(seq), len, &obj, 1);
    }

    case TYPE_NE_SLICE_UINT8: {
      if (is_int(obj)) {
        int64 value = get_int(obj);
        if (value >= 0 & value < 255) {
          uint8 value_uint8 = (uint8) value;
          return concat_uint8(get_seq_elts_ptr_uint8(seq), len, &value_uint8, 1);
        }
      }

      return concat_uint8_obj(get_seq_elts_ptr_uint8(seq), len, &obj, 1);
    }

    default:
      internal_fail();
  }
}




// OBJ update_seq_at(OBJ seq, OBJ idx, OBJ value) {
//   uint32 len = get_seq_length(seq);
//   int64 int_idx = get_int(idx);

//   if (int_idx < 0 | int_idx >= len)
//     soft_fail("Invalid sequence index");

//   OBJ *src_ptr = get_seq_elts_ptr(seq);
//   SEQ_OBJ *new_seq_ptr = new_obj_seq(len);

//   new_seq_ptr->buffer.obj[int_idx] = value;
//   for (uint32 i=0 ; i < len ; i++)
//     if (i != int_idx)
//       new_seq_ptr->buffer.obj[i] = src_ptr[i];

//   return make_seq(new_seq_ptr, len);
// }

OBJ join_seqs(OBJ left, OBJ right) {
  // No need to check the parameters here

  uint64 lenr = get_seq_length(right);
  if (lenr == 0)
    return left;

  uint64 lenl = get_seq_length(left);
  if (lenl == 0)
    return right;

  if (lenl + lenr > 0xFFFFFFFF)
    impl_fail("_cat_(): Resulting sequence is too large");

  uint32 len = lenl + lenr;

  OBJ_TYPE typel = get_physical_type(left);
  OBJ_TYPE typer = get_physical_type(right);

  if (typel == TYPE_NE_SEQ_UINT8) {
    SEQ_OBJ *ptrl = get_seq_ptr(left);

    if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
      return in_place_concat_uint8(ptrl, lenl, get_seq_elts_ptr_uint8(right), lenr);

    return concat_uint8_obj(ptrl->buffer.uint8_, lenl, get_seq_elts_ptr(right), lenr);
  }

  if (typel == TYPE_NE_SLICE_UINT8) {
    uint8 *eltsl = get_seq_elts_ptr_uint8(left);

    if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
      return concat_uint8(eltsl, lenl, get_seq_elts_ptr_uint8(right), lenr);

    return concat_uint8_obj(eltsl, lenl, get_seq_elts_ptr(right), lenr);
  }

  if (typel == TYPE_NE_SLICE) {
    OBJ *eltsl = get_seq_elts_ptr(left);

    if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
      return concat_obj_uint8(eltsl, lenl, get_seq_elts_ptr_uint8(right), lenr);

    return concat_obj(eltsl, lenl, get_seq_elts_ptr(right), lenr);
  }

  assert(typel == TYPE_NE_SEQ);

  SEQ_OBJ *ptrl = get_seq_ptr(left);

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
    return in_place_concat_obj_uint8(ptrl, lenl, get_seq_elts_ptr_uint8(right), lenr);

  return in_place_concat_obj(ptrl, lenl, get_seq_elts_ptr(right), lenr);
}

OBJ rev_seq(OBJ seq) {
  // No need to check the parameters here

  uint32 len = get_seq_length(seq);
  if (len <= 1)
    return seq;

  OBJ_TYPE type = get_physical_type(seq);
  if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8) {
    uint8 *elems = get_seq_elts_ptr_uint8(seq);

    uint8 *rev_elems = new_uint8_array(len);
    for (int i=0 ; i < len ; i++)
      rev_elems[i] = elems[len - 1 - i];

    return make_slice_uint8(rev_elems, len);
  }
  else {
    OBJ *elems = get_seq_elts_ptr(seq);

    SEQ_OBJ *rs = new_obj_seq(len);
    OBJ *rev_elems = rs->buffer.obj;
    for (uint32 i=0 ; i < len ; i++)
      rev_elems[len-i-1] = elems[i];

    return make_seq(rs, len);
  }
}

////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ build_seq_uint8(OBJ *elts, uint32 length) {
  uint8 *uint8_elts = (uint8 *) elts;
  for (int i=0 ; i < length ; i++)
    uint8_elts[i] = get_int(elts[i]);
  return make_slice_uint8(uint8_elts, length);
}

OBJ build_seq(OBJ *elts, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  // return make_slice(elts, length);

// void gather_build_seq_obj_stats(OBJ *elts, uint32 length);
// gather_build_seq_obj_stats(elts, length);

  for (int i=0 ; i < length ; i++) {
    OBJ elt = elts[i];
    if (!is_int(elt))
      return make_slice(elts, length);
    int64 value = get_int(elt);
    if (value < 0 | value > 255)
      return make_slice(elts, length);
  }

  return build_seq_uint8(elts, length);
}

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

    // counter_build_seq_int64_uint8++;
    // if (size <= 8)
    //   counter_build_seq_small_8++;

    uint8 *uint8_array = (uint8 *) array;
    for (uint32 i=0 ; i < size ; i++)
      uint8_array[i] = array[i];


    return make_slice_uint8(uint8_array, size);
  }

  // if (min >= -128 & max < 128) {
  //   counter_build_seq_int64_int8++;
  //   if (size <= 8)
  //     counter_build_seq_small_8++;
  // }
  // else if (min >= -32768 & max < 32768) {
  //   counter_build_seq_int64_int16++;
  //   if (size <= 4)
  //     counter_build_seq_small_16++;
  // }
  // else if (min >= 0 & max < 65536) {
  //   counter_build_seq_int64_uint16++;
  //   if (size <= 4)
  //     counter_build_seq_small_16++;
  // }
  // else if (min >= -2147483648LL & max < 2147483648LL) {
  //   counter_build_seq_int64_int32;
  // }
  // else if (min >= 0 & max < 4294967296LL) {
  //   counter_build_seq_int64_uint32++;
  // }
  // else {
  //   counter_build_seq_int64++;
  // }

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

// counter_build_seq_int32++;

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

// counter_build_seq_uint32++;

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

// counter_build_seq_int16++;

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(uint16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

// counter_build_seq_uint16++;

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq(int8* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  // if (size <= 8)
  //   counter_build_seq_small_8++;

  for (int i=0 ; i < size ; i++)
    if (array[i] < 0) {
      SEQ_OBJ *seq = new_obj_seq(size);

      for (uint32 i=0 ; i < size ; i++)
        seq->buffer.obj[i] = make_int(array[i]);

// counter_build_seq_int8++;

      return make_seq(seq, size);
    }

// counter_build_seq_int8_uint7++;

  return make_slice_uint8((uint8 *) array, size);
}

OBJ build_seq(uint8 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  // if (size <= 8)
    // counter_build_seq_small_8++;

// counter_build_seq_uint8++;

  return make_slice_uint8(array, size);

  // SEQ_OBJ *seq = new_obj_seq(size);

  // for (uint32 i=0 ; i < size ; i++)
  //   seq->buffer.obj[i] = make_int(array[i]);

  // return make_seq(seq, size);
}




OBJ build_const_seq(int16* array, uint32 size) {
  assert(size != 0);

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}


OBJ build_const_seq(uint8 *array, uint32 size) {
  assert(size != 0);

  return make_slice_uint8(array, size);
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




////////////////////////////////////////////////////////////////////////////////

OBJ build_const_uint8_seq(const uint8* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint16_seq(const uint16* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint32_seq(const uint32* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int8_seq(const int8* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int16_seq(const int16* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int32_seq(const int32* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int64_seq(const int64* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.obj[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}