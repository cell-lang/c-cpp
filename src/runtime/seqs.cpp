#include "lib.h"


OBJ get_obj_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= get_seq_length(seq))
    soft_fail("Invalid sequence index");

  OBJ_TYPE type = get_physical_type(seq);

  if (type == TYPE_NE_SEQ | type == TYPE_NE_SLICE)
    return get_seq_elts_ptr(seq)[idx];

  switch (type) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8:
      return make_int(get_seq_elts_ptr_uint8(seq)[idx]);

    case TYPE_NE_SEQ_UINT8_INLINE:
      return make_int(inline_uint8_at(seq.core_data.int_, idx));

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16:
      return make_int(get_seq_elts_ptr_int16(seq)[idx]);

    case TYPE_NE_SEQ_INT16_INLINE:
      return make_int(inline_int16_at(seq.core_data.int_, idx));

    default:
      internal_fail();
  }
}

double get_float_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= get_seq_length(seq))
    soft_fail("Invalid sequence index");

  return get_float(get_seq_elts_ptr(seq)[idx]);
}

int64 get_int_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= get_seq_length(seq))
    soft_fail("Invalid sequence index");

  OBJ_TYPE type = get_physical_type(seq);

  switch (type) {
    case TYPE_NE_SEQ:
    case TYPE_NE_SLICE:
      return get_int(get_seq_elts_ptr(seq)[idx]);

    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8:
      return get_seq_elts_ptr_uint8(seq)[idx];

    case TYPE_NE_SEQ_UINT8_INLINE:
      return inline_uint8_at(seq.core_data.int_, idx);

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16:
      return get_seq_elts_ptr_int16(seq)[idx];

    case TYPE_NE_SEQ_INT16_INLINE:
      return inline_int16_at(seq.core_data.int_, idx);

    default:
      internal_fail();
  }
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

  switch (type) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts = get_seq_elts_ptr_uint8(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(elts[i]);
      return buffer;
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      uint64 data = seq.core_data.int_;
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(inline_uint8_at(data, i));
      return buffer;
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts = get_seq_elts_ptr_int16(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(elts[i]);
      return buffer;
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      uint64 data = seq.core_data.int_;
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(inline_int16_at(data, i));
      return buffer;
    }

    default:
      internal_fail();
  }
}

int64* get_long_array(OBJ seq, int64 *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;

  int len = get_seq_length(seq);

  if (buffer == NULL | size < len)
    buffer = new_int64_array(len);

  OBJ_TYPE type = get_physical_type(seq);

  switch (type) {
    case TYPE_NE_SEQ:
    case TYPE_NE_SLICE: {
      OBJ *elts = get_seq_elts_ptr(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = get_int(elts[i]);
      return buffer;
    }

    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts = get_seq_elts_ptr_uint8(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
      return buffer;
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      uint64 data = seq.core_data.int_;
      for (int i=0 ; i < len ; i++)
        buffer[i] = inline_uint8_at(data, i);
      return buffer;
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts = get_seq_elts_ptr_int16(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
      return buffer;
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      uint64 data = seq.core_data.int_;
      for (int i=0 ; i < len ; i++)
        buffer[i] = inline_int16_at(data, i);
      return buffer;
    }

    default:
      internal_fail();
  }
}

double* get_double_array(OBJ seq, double *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;

  int len = get_seq_length(seq);

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

  int len = get_seq_length(seq);

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

  switch (type) {
    case TYPE_NE_SEQ:
    case TYPE_NE_SLICE: {
      OBJ *elts = get_seq_elts_ptr(seq) + idx_first;

      int64 min = 0;
      int64 max = 0;

      for (int i=0 ; i < len ; i++) {
        OBJ elt = elts[i];
        if (!is_int(elt))
          return make_slice(elts, len);

        int64 value = get_int(elt);
        if (value < min)
          min = value;
        else if (value > max)
          max = value;
      }

      if (min == 0 & max < 256) {
        if (len <= 8) {
          int64 data = 0;
          for (uint32 i=0 ; i < len ; i++)
            data = inline_uint8_init_at(data, i, (uint8) get_int(elts[i]));
          return make_seq_uint8_inline(data, len);
        }
        else {
          SEQ_OBJ *seq_ptr = new_uint8_seq(len);
          for (uint32 i=0 ; i < len ; i++)
            seq_ptr->buffer.uint8_[i] = (uint8) get_int(elts[i]);
          return make_seq_uint8(seq_ptr, len);
        }
      }

      if (min >= -32768 & max < 32768) {
        if (len <= 4) {
          uint64 data = 0;
          for (uint32 i=0 ; i < len ; i++)
            data = inline_int16_init_at(data, i, (int16) get_int(elts[i]));
          return make_seq_int16_inline(data, len);
        }
        else {
          SEQ_OBJ *seq_ptr = new_int16_seq(len);
          for (uint32 i=0 ; i < len ; i++)
            seq_ptr->buffer.int16_[i] = (int16) get_int(elts[i]);
          return make_seq_int16(seq_ptr, len);
        }
      }

      return make_slice(elts, len);
    }

    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts = get_seq_elts_ptr_uint8(seq) + idx_first;
      if (len <= 8)
        return make_seq_uint8_inline(inline_uint8_pack(elts, len), len);
      else
        return make_slice_uint8(elts, len);
    }

    case TYPE_NE_SEQ_UINT8_INLINE:
      return make_seq_uint8_inline(inline_uint8_slice(seq.core_data.int_, idx_first, len), len);

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts = get_seq_elts_ptr_int16(seq) + idx_first;
      if (len <= 8) {
        uint64 data = 0;
        for (int i=0 ; i < len ; i++) {
          int16 elt = elts[i];
          if (elt < 0 | elt > 255)
            goto no_uint8;
          data = inline_uint8_init_at(data, i, (uint8) elt);
        }
        return make_seq_uint8_inline(data, len);
      }

no_uint8:
      if (len <= 4)
        return make_seq_int16_inline(inline_int16_pack(elts, len), len);
      else
        return make_slice_int16(elts, len);
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      uint64 data = inline_int16_slice(seq.core_data.int_, idx_first, len);
      for (int i=0 ; i < len ; i++) {
        int16 elt = inline_int16_at(data, i);
        if (elt < 0 | elt > 255)
          return make_seq_int16_inline(data, len);
      }
      uint64 data8 = 0;
      for (int i=0 ; i < len ; i++)
        data8 = inline_uint8_init_at(data8, i, inline_int16_at(data, i));
      return make_seq_uint8_inline(data8, len);
    }

    default:
      internal_fail();
  }
}

OBJ append_to_seq(OBJ seq, OBJ obj) {
  OBJ append(OBJ seq, OBJ obj);
  return append(seq, obj);
}

OBJ join_seqs(OBJ left, OBJ right) {
  OBJ concat(OBJ left, OBJ right);
  return concat(left, right);
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

OBJ rev_seq(OBJ seq) {
  // No need to check the parameters here

  uint32 len = get_seq_length(seq);
  if (len <= 1)
    return seq;

  OBJ_TYPE type = get_physical_type(seq);

  switch (type) {
    case TYPE_NE_SEQ:
    case TYPE_NE_SLICE: {
      SEQ_OBJ *seq_ptr = new_obj_seq(len);
      OBJ *elts = get_seq_elts_ptr(seq);
      OBJ *rev_elts = seq_ptr->buffer.obj;
      for (uint32 i=0 ; i < len ; i++)
        rev_elts[len-i-1] = elts[i];
      return make_seq(seq_ptr, len);
    }

    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts = get_seq_elts_ptr_uint8(seq);
      uint8 *rev_elts = new_uint8_array(len);
      for (int i=0 ; i < len ; i++)
        rev_elts[i] = elts[len - 1 - i];
      return make_slice_uint8(rev_elts, len);
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      uint64 elts = seq.core_data.int_;
      uint64 rev_elts = 0;
      for (int i=0 ; i < len ; i++)
        rev_elts = inline_uint8_init_at(rev_elts, i, inline_uint8_at(elts, len - i - 1));
      return make_seq_uint8_inline(rev_elts, len);
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts = get_seq_elts_ptr_int16(seq);
      int16 *rev_elts = new_int16_array(len);
      for (int i=0 ; i < len ; i++)
        rev_elts[i] = elts[len - 1 - i];
      return make_slice_int16(rev_elts, len);
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      uint64 elts = seq.core_data.int_;
      uint64 rev_elts = 0;
      for (int i=0 ; i < len ; i++)
        rev_elts = inline_int16_init_at(rev_elts, i, inline_int16_at(elts, len - i - 1));
      return make_seq_int16_inline(rev_elts, len);
    }

    default:
      internal_fail();
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_seq(OBJ *elts, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  int64 min = 0;
  int64 max = 127;

  for (uint32 i=0 ; i < length ; i++) {
    OBJ elt = elts[i];
    if (!is_int(elt))
      return make_slice(elts, length);

    int64 value = get_int(elt);
    if (value < min)
      min = value;
    else if (value > max)
      max = value;
  }

  if (min == 0 & max < 256) {
    if (length <= 8) {
      uint64 packed_elts = 0;
      for (uint32 i=0 ; i < length ; i++)
        packed_elts = inline_uint8_init_at(packed_elts, i, get_int(elts[i]));
      return make_seq_uint8_inline(packed_elts, length);
    }
    else {
      uint8 *uint8_elts = (uint8 *) elts;
      for (uint32 i=0 ; i < length ; i++)
        uint8_elts[i] = get_int(elts[i]);
      return make_slice_uint8(uint8_elts, length);
    }
  }

  if (min >= -32768 & max < 32768) {
    if (length <= 4) {
      uint64 packed_elts = 0;
      for (uint32 i=0 ; i < length ; i++)
        packed_elts = inline_int16_init_at(packed_elts, i, get_int(elts[i]));
      return make_seq_int16_inline(packed_elts, length);
    }
    else {
      int16 *int16_elts = (int16 *) elts;
      for (uint32 i=0 ; i < length ; i++)
        int16_elts[i] = get_int(elts[i]);
      return make_slice_int16(int16_elts, length);
    }
  }

  return make_slice(elts, length);
}

OBJ build_seq_bool(bool* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_bool(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq_double(double* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);
  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_float(array[i]);
  return make_seq(seq, size);
}

OBJ build_seq_int64(int64* array, int32 size) {
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

  if (min == 0 & max < 256) {
    if (size <= 8) {
      int64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_uint8_init_at(elts, i, (uint8) array[i]);
      return make_seq_uint8_inline(elts, size);
    }
    else {
      uint8 *uint8_array = (uint8 *) array;
      for (uint32 i=0 ; i < size ; i++)
        uint8_array[i] = array[i];
      return make_slice_uint8(uint8_array, size);
    }
  }

  if (min >= -32768 & max < 32768) {
    if (size <= 4) {
      uint64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_int16_init_at(elts, i, array[i]);
      return make_seq_int16_inline(elts, size);
    }
    else {
      int16 *int16_array = (int16 *) array;
      for (uint32 i=0 ; i < size ; i++)
        int16_array[i] = array[i];
      return make_slice_int16(int16_array, size);
    }
  }

  SEQ_OBJ *seq = new_obj_seq(size);
  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);
  return make_seq(seq, size);
}

OBJ build_seq_int32(int32* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  int32 min = 0;
  int32 max = 127;

  for (uint32 i=0 ; i < size ; i++) {
    int32 elt = array[i];
    if (elt < min)
      min = elt;
    if (elt > max)
      max = elt;
  }

  if (min == 0 & max < 256) {
    if (size <= 8) {
      uint64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_uint8_init_at(elts, i, (uint8) array[i]);
      return make_seq_uint8_inline(elts, size);
    }
    else {
      uint8 *uint8_array = (uint8 *) array;
      for (uint32 i=0 ; i < size ; i++)
        uint8_array[i] = array[i];
      return make_slice_uint8(uint8_array, size);
    }
  }

  if (min >= -32768 & max < 32768) {
    if (size <= 4) {
      uint64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_int16_init_at(elts, i, array[i]);
      return make_seq_int16_inline(elts, size);
    }
    else {
      int16 *int16_array = (int16 *) array;
      for (uint32 i=0 ; i < size ; i++)
        int16_array[i] = array[i];
      return make_slice_int16(int16_array, size);
    }
  }

  SEQ_OBJ *seq = new_obj_seq(size);
  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);
  return make_seq(seq, size);
}

OBJ build_seq_int16(int16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 8) {
    for (int i=0 ; i < size ; i++) {
      int16 elt = array[i];
      if (elt < 0 | elt > 255)
        goto no_uint8;
    }

    uint64 data = 0;
    for (int i=0 ; i < size ; i++)
      data = inline_uint8_init_at(data, i, (uint8) array[i]);
    return make_seq_uint8_inline(data, size);
  }

no_uint8:

  if (size <= 4)
    return make_seq_int16_inline(inline_int16_pack(array, size), size);
  else
    return make_slice_int16(array, size);
}

OBJ build_seq_int8(int8* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 4) {
    uint64 elts = 0;
    for (int i=0 ; i < size ; i++)
      elts = inline_int16_init_at(elts, i, array[i]);
    return make_seq_int16_inline(elts, size);
  }

  SEQ_OBJ *seq_ptr = new_int16_seq(size, ((size + 3) / 4) * 4);
  for (int i=0 ; i < size ; i++)
    seq_ptr->buffer.int16_[i] = array[i];
  return make_seq_int16(seq_ptr, size);
}

OBJ build_seq_uint8(uint8 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 8)
    return make_seq_uint8_inline(inline_uint8_pack(array, size), size);

  return make_slice_uint8(array, size);
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_const_seq_uint8(const uint8 *array, uint32 size) {
  assert(size != 0);
  // assert((((uint64) array) & 7) == 0); //## THIS ACTUALLY HAPPENS, AND IT'S NOT AN EXCEPTION

  if (size <= 8)
    return make_seq_uint8_inline(inline_uint8_pack((uint8 *) array, size), size);
  else
    return make_slice_uint8((uint8 *) array, size);
}

OBJ build_const_seq_int8(const int8 *array, uint32 size) {
  assert(size != 0);

  if (size <= 4) {
    uint64 data = 0;
    for (int i=0 ; i < size ; i++)
      data = inline_int16_init_at(data, i, array[i]);
    return make_seq_int16_inline(data, size);
  }

  SEQ_OBJ *seq = new_int16_seq(size, (size + 3) & ~3);
  for (int i=0 ; i < size ; i++)
    seq->buffer.int16_[i] = array[i];
  return make_seq(seq, size);
}

OBJ build_const_seq_int16(const int16 *array, uint32 size) {
  assert(size != 0);

  if (size <= 4)
    return make_seq_int16_inline(inline_int16_pack((int16 *) array, size), size);
  else
    return make_slice_int16((int16 *) array, size);
}

OBJ build_const_seq_int32(const int32 *array, uint32 size) {
  assert(size != 0);

  SEQ_OBJ *seq = new_obj_seq(size);
  for (int i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);
  return make_seq(seq, size);
}

OBJ build_const_seq_int64(const int64 *array, uint32 size) {
  assert(size != 0);

  SEQ_OBJ *seq = new_obj_seq(size);
  for (int i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);
  return make_seq(seq, size);
}
