#include "lib.h"


OBJ get_obj_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= read_size_field(seq))
    soft_fail("Invalid sequence index");

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_SEQ)
    return get_seq_elts_ptr(seq)[idx];

  switch (type) {
    case TYPE_NE_SEQ_UINT8_INLINE:
      return make_int(inline_uint8_at(seq.core_data.int_, idx));

    case TYPE_NE_SEQ_INT16_INLINE:
      return make_int(inline_int16_at(seq.core_data.int_, idx));

    case TYPE_NE_SEQ_INT32_INLINE:
      return make_int(inline_int32_at(seq.core_data.int_, idx));

    case TYPE_NE_INT_SEQ:
      return make_int(get_int_at_unchecked(seq, idx));

    case TYPE_NE_FLOAT_SEQ:
      return make_float(get_seq_elts_ptr_float(seq)[idx]);

    case TYPE_NE_BOOL_SEQ:
      internal_fail();

    default:
      internal_fail();
  }
}

double get_float_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= read_size_field(seq))
    soft_fail("Invalid sequence index");

  return get_seq_elts_ptr_float(seq)[idx];
}

int64 get_int_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= read_size_field(seq))
    soft_fail("Invalid sequence index");

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_INT_SEQ)
    return get_int_at_unchecked(seq, idx);

  if (type == TYPE_NE_SEQ_UINT8_INLINE)
    return inline_uint8_at(seq.core_data.int_, idx);

  if (type == TYPE_NE_SEQ_INT16_INLINE)
    return inline_int16_at(seq.core_data.int_, idx);

  assert(type == TYPE_NE_SEQ_INT32_INLINE);
  return inline_int32_at(seq.core_data.int_, idx);
}

int64 get_int_at_unchecked(OBJ seq, uint32 idx) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(idx < read_size_field(seq));

  uint32 width_tag = get_int_bits_tag(seq);

  if (!is_signed(seq)) {
    assert(width_tag == INT_BITS_TAG_8);
    uint8 *elts = (uint8 *) seq.core_data.ptr;
    return elts[idx];
  }

  if (width_tag < INT_BITS_TAG_32) {
    if (width_tag == INT_BITS_TAG_8) {
      int8 *elts = (int8 *) seq.core_data.ptr;
      return elts[idx];
    }
    else {
      assert(width_tag == INT_BITS_TAG_16);
      int16 *elts = (int16 *) seq.core_data.ptr;
      return elts[idx];
    }
  }
  else {
    if (width_tag == INT_BITS_TAG_32) {
      int32 *elts = (int32 *) seq.core_data.ptr;
      return elts[idx];
    }
    else {
      assert(width_tag == INT_BITS_TAG_64);
      int64 *elts = (int64 *) seq.core_data.ptr;
      return elts[idx];
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void copy_int64_range_unchecked(OBJ seq, uint32 start, uint32 len, int64 *buffer) {
  assert(is_seq(seq));
  assert(start + len <= read_size_field(seq));

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_INT_SEQ) {
    INT_BITS_TAG width_tag = get_int_bits_tag(seq);

    if (!is_signed(seq)) {
      assert(width_tag == INT_BITS_TAG_8);
      uint8 *elts = (uint8 *) seq.core_data.ptr + start;
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }

    if (width_tag < INT_BITS_TAG_32) {
      if (width_tag == INT_BITS_TAG_8) {
        int8 *elts = (int8 *) seq.core_data.ptr + start;
        for (int i=0 ; i < len ; i++)
          buffer[i] = elts[i];
      }
      else {
        assert(width_tag == INT_BITS_TAG_16);
        int16 *elts = (int16 *) seq.core_data.ptr + start;
        for (int i=0 ; i < len ; i++)
          buffer[i] = elts[i];
      }
    }
    else {
      if (width_tag == INT_BITS_TAG_32) {
        int32 *elts = (int32 *) seq.core_data.ptr + start;
        for (int i=0 ; i < len ; i++)
          buffer[i] = elts[i];
      }
      else {
        assert(width_tag == INT_BITS_TAG_64);
        memcpy(buffer, (int64 *) seq.core_data.ptr + start, len);
      }
    }
  }
  else if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_uint8_at(seq.core_data.int_, i + start);
  }
  else if (type == TYPE_NE_SEQ_INT16_INLINE) {
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_int16_at(seq.core_data.int_, i + start);
  }
  else {
    assert(type == TYPE_NE_SEQ_INT32_INLINE);
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_int32_at(seq.core_data.int_, i + start);
  }
}

void copy_int32_range_unchecked(OBJ seq, uint32 start, uint32 len, int32 *buffer) {
  assert(is_seq(seq));
  assert(start + len <= read_size_field(seq));

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_INT_SEQ) {
    INT_BITS_TAG width_tag = get_int_bits_tag(seq);

    if (!is_signed(seq)) {
      assert(width_tag == INT_BITS_TAG_8);
      uint8 *elts = (uint8 *) seq.core_data.ptr + start;
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }

    if (width_tag == INT_BITS_TAG_8) {
      int8 *elts = (int8 *) seq.core_data.ptr + start;
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }
    else if (width_tag == INT_BITS_TAG_16) {
      int16 *elts = (int16 *) seq.core_data.ptr + start;
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }
    else {
      assert(width_tag == INT_BITS_TAG_32);
      memcpy(buffer, (int32 *) seq.core_data.ptr + start, len);
    }
  }
  else if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_uint8_at(seq.core_data.int_, i + start);
  }
  else if (type == TYPE_NE_SEQ_INT16_INLINE) {
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_int16_at(seq.core_data.int_, i + start);
  }
  else {
    assert(type == TYPE_NE_SEQ_INT32_INLINE);
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_int32_at(seq.core_data.int_, i + start);
  }
}

void copy_int16_range_unchecked(OBJ seq, uint32 start, uint32 len, int16 *buffer) {
  assert(is_seq(seq));
  assert(no_sum32_overflow(start, len));
  assert(start + len <= read_size_field(seq));

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_INT_SEQ) {
    INT_BITS_TAG width_tag = get_int_bits_tag(seq);

    if (!is_signed(seq)) {
      assert(width_tag == INT_BITS_TAG_8);
      uint8 *elts = (uint8 *) seq.core_data.ptr + start;
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }

    if (width_tag == INT_BITS_TAG_8) {
      int8 *elts = (int8 *) seq.core_data.ptr + start;
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }
    else {
      assert(width_tag == INT_BITS_TAG_16);
      memcpy(buffer, (int16 *) seq.core_data.ptr + start, len);
    }
  }
  else if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_uint8_at(seq.core_data.int_, i + start);
  }
  else {
    assert(type == TYPE_NE_SEQ_INT16_INLINE);
    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_int16_at(seq.core_data.int_, i + start);
  }
}

void copy_int8_range_unchecked(OBJ seq, uint32 start, uint32 len, int8 *buffer) {
  assert(is_seq(seq));
  assert(no_sum32_overflow(start, len));
  assert(start + len <= read_size_field(seq));

  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(get_int_bits_tag(seq) == INT_BITS_TAG_8);
  assert(!is_signed(seq));

  memcpy(buffer, (int8 *) seq.core_data.ptr + start, len * sizeof(int8));
}

void copy_uint8_range_unchecked(OBJ seq, uint32 start, uint32 len, uint8 *buffer) {
  assert(is_seq(seq));
  assert(no_sum32_overflow(start, len));
  assert(start + len <= read_size_field(seq));

  if (get_obj_type(seq) == TYPE_NE_INT_SEQ) {
    assert(get_int_bits_tag(seq) == INT_BITS_TAG_8);
    assert(is_signed(seq));

    memcpy(buffer, (uint8 *) seq.core_data.ptr + start, len * sizeof(uint8));
  }
  else {
    assert(get_obj_type(seq) == TYPE_NE_SEQ_UINT8_INLINE);

    for (int i=0 ; i < len ; i++)
      buffer[i] = inline_uint8_at(seq.core_data.int_, i + start);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ* get_obj_array(OBJ seq, OBJ* buffer, int32 size) {
  assert(read_size_field(seq) == size);

  if (is_empty_seq(seq))
    return NULL;

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_SEQ)
    return get_seq_elts_ptr(seq);

  uint32 len = read_size_field(seq);

  if (buffer == NULL)
    buffer = new_obj_array(len);

  switch (type) {
    case TYPE_NE_SEQ_UINT8_INLINE:
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(inline_uint8_at(seq.core_data.int_, i));
      return buffer;

    case TYPE_NE_SEQ_INT16_INLINE:
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(inline_int16_at(seq.core_data.int_, i));
      return buffer;

    case TYPE_NE_SEQ_INT32_INLINE:
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(inline_int32_at(seq.core_data.int_, i));
      return buffer;

    case TYPE_NE_INT_SEQ:
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_int(get_int_at_unchecked(seq, i));
      return buffer;

    case TYPE_NE_FLOAT_SEQ:
      for (int i=0 ; i < len ; i++)
        buffer[i] = make_float(get_seq_elts_ptr_float(seq)[i]);
      return buffer;

    case TYPE_NE_BOOL_SEQ:
      internal_fail();

    default:
      internal_fail();
  }
}

int64* get_long_array(OBJ seq, int64 *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;

  OBJ_TYPE type = get_obj_type(seq);
  uint32 len = read_size_field(seq);

  if (type == TYPE_NE_INT_SEQ) {
    INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

    if (bits_tag == INT_BITS_TAG_64)
      return get_seq_elts_ptr_int64(seq);

    if (buffer == NULL | size < len)
      buffer = new_int64_array(len);

    if (bits_tag == INT_BITS_TAG_8) {
      if (is_signed(seq)) {
        int8 *elts = get_seq_elts_ptr_int8(seq);
        for (int i=0 ; i < len ; i++)
          buffer[i] = elts[i];
      }
      else {
        uint8 *elts = get_seq_elts_ptr_uint8(seq);
        for (int i=0 ; i < len ; i++)
          buffer[i] = elts[i];
      }
    }
    else if (bits_tag == INT_BITS_TAG_16) {
      int16 *elts = get_seq_elts_ptr_int16(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }
    else {
      assert(bits_tag == INT_BITS_TAG_32);
      int32 *elts = get_seq_elts_ptr_int32(seq);
      for (int i=0 ; i < len ; i++)
        buffer[i] = elts[i];
    }
  }
  else {
    if (buffer == NULL | size < len)
      buffer = new_int64_array(len);

    if (type == TYPE_NE_SEQ_UINT8_INLINE) {
      for (int i=0 ; i < len ; i++)
        buffer[i] = inline_uint8_at(seq.core_data.int_, i);
    }
    else if (type == TYPE_NE_SEQ_INT16_INLINE) {
      for (int i=0 ; i < len ; i++)
        buffer[i] = inline_int16_at(seq.core_data.int_, i);
    }
    else {
      assert(type == TYPE_NE_SEQ_INT32_INLINE);
      for (int i=0 ; i < len ; i++)
        buffer[i] = inline_int32_at(seq.core_data.int_, i);
    }
  }

  return buffer;
}

double* get_double_array(OBJ seq, double *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;

  return get_seq_elts_ptr_float(seq);
}

bool* get_bool_array(OBJ seq, bool *buffer, int32 size) {
  if (is_empty_seq(seq))
    return NULL;

  int len = read_size_field(seq);

  if (buffer == NULL | size < len)
    buffer = new_bool_array(len);

  OBJ *seq_buffer = get_seq_elts_ptr(seq);

  for (int i=0 ; i < len ; i++)
    buffer[i] = get_bool(seq_buffer[i]);

  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ get_inline_int16_seq_slice(OBJ seq, uint32 idx_first, uint32 len) {
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

inline OBJ get_inline_int32_seq_slice(OBJ seq, uint32 idx_first, uint32 len) {
  assert(len == 1 | len == 2);

  if (len == 2) {
    assert(idx_first == 0);
    assert(read_size_field(seq) == 2);
    return seq;
  }

  int32 only_elt = inline_int32_at(seq.core_data.int_, idx_first);

  if (only_elt >= 0 & only_elt < 256)
    return make_seq_uint8_inline(inline_uint8_init_at(0, 0, only_elt), 1);

  if (only_elt >= -32768 & only_elt < 32768)
    return make_seq_int16_inline(inline_int16_init_at(0, 0, only_elt), 1);

  return make_seq_int32_inline(inline_int32_init_at(0, 0, only_elt), 1);
}

inline OBJ get_int_seq_slice(OBJ seq, uint32 idx_first, uint32 len) {
  if (len <= 8) {
    int64 elts[8];

    int64 min = 0;
    int64 max = 0;

    for (int i=0 ; i < len ; i++) {
      int64 elt = get_int_at_unchecked(seq, i + idx_first);
      elts[i] = elt;
      if (elt < min)
        min = elt;
      if (elt > max)
        max = elt;
    }

    if (min == 0 & max < 256) {
      int64 data = 0;
      for (uint32 i=0 ; i < len ; i++)
        data = inline_uint8_init_at(data, i, (uint8) elts[i]);
      return make_seq_uint8_inline(data, len);
    }

    if (len <= 4 & min >= -32768 & max < 32768) {
      uint64 data = 0;
      for (uint32 i=0 ; i < len ; i++)
        data = inline_int16_init_at(data, i, (int16) elts[i]);
      return make_seq_int16_inline(data, len);
    }

    if (len <= 2 & min >= -2147483648 & max < 2147483648) {
      uint64 data = inline_int32_init_at(0, 0, (int32) elts[0]);
      if (len == 2)
        data = inline_int32_init_at(data, 1, (int32) elts[1]);
      return make_seq_int32_inline(data, len);
    }
  }

  INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

  if (bits_tag < INT_BITS_TAG_32) {
    if (bits_tag == INT_BITS_TAG_8) {
      if (is_signed(seq))
        return make_slice_int8(get_seq_elts_ptr_int8(seq) + idx_first, len);
      else
        return make_slice_uint8(get_seq_elts_ptr_uint8(seq) + idx_first, len);
    }
    else {
      assert(bits_tag == INT_BITS_TAG_16);
      return make_slice_int16(get_seq_elts_ptr_int16(seq) + idx_first, len);
    }
  }
  else {
    if (bits_tag == INT_BITS_TAG_32) {
      return make_slice_int32(get_seq_elts_ptr_int32(seq) + idx_first, len);
    }
    else {
      assert(bits_tag == INT_BITS_TAG_64);
      return make_slice_int64(get_seq_elts_ptr_int64(seq) + idx_first, len);
    }
  }
}

OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len) {
  assert(is_seq(seq));

  if (idx_first < 0 | len < 0 | idx_first + len > read_size_field(seq))
    soft_fail("_slice_(): Invalid start index and/or subsequence length");

  if (len == 0)
    return make_empty_seq();

  switch (get_obj_type(seq)) {
    case TYPE_NE_SEQ_UINT8_INLINE:
      return make_seq_uint8_inline(inline_uint8_slice(seq.core_data.int_, idx_first, len), len);

    case TYPE_NE_SEQ_INT16_INLINE:
      return get_inline_int16_seq_slice(seq, idx_first, len);

    case TYPE_NE_SEQ_INT32_INLINE:
      return get_inline_int32_seq_slice(seq, idx_first, len);

    case TYPE_NE_INT_SEQ:
      return get_int_seq_slice(seq, idx_first, len);

    case TYPE_NE_FLOAT_SEQ:
      return make_slice_float(get_seq_elts_ptr_float(seq) + idx_first, len);

    case TYPE_NE_BOOL_SEQ:
      internal_fail();

    case TYPE_NE_SEQ:
      return build_seq(get_seq_elts_ptr(seq) + idx_first, len);

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
//   uint32 len = read_size_field(seq);
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

  uint32 len = read_size_field(seq);
  if (len <= 1)
    return seq;

  OBJ *rev_elts = new_obj_array(len);
  for (int i=0 ; i < len ; i++)
    rev_elts[i] = get_obj_at(seq, len - i - 1);
  return build_seq(rev_elts, len);
}

void get_seq_iter(SEQ_ITER &it, OBJ seq) {
  it.idx = 0;

  if (!is_empty_seq(seq)) {
    it.seq = seq;
    it.len = read_size_field(seq);
  }
  else {
    it.len = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_seq(OBJ *elts, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  OBJ first_elt = elts[0];

  if (is_int(first_elt)) {
    int64 min = get_int(first_elt);
    int64 max = min;

    for (uint32 i=1 ; i < length ; i++) {
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

    if (min >= -2147483648 & max < 2147483648) {
      if (length <= 2) {
        uint64 data = inline_int32_init_at(0, 0, (int32) get_int(first_elt));
        if (length == 2)
          data = inline_int32_init_at(data, 1, (int32) get_int(elts[1]));
        return make_seq_int32_inline(data, length);
      }
      else {
        int32 *int32_elts = new_int32_array(length);
        for (uint32 i=0 ; i < length ; i++)
          int32_elts[i] = (int32) get_int(elts[i]);
        return make_slice_int32(int32_elts, length);
      }
    }

    int64 *int64_elts = new_int64_array(length);
    for (uint32 i=0 ; i < length ; i++)
      int64_elts[i] = get_int(elts[i]);
    return make_slice_int64(int64_elts, length);
  }

  if (is_float(first_elt)) {
    for (int i=1 ; i < length ; i++)
      if (!is_float(elts[i]))
        return make_slice(elts, length);

    double *float_elts = new_float_array(length);
    for (int i=0 ; i < length ; i++)
      float_elts[i] = get_float(elts[i]);
    return make_slice_float(float_elts, length);
  }

  return make_slice(elts, length);
}

OBJ build_seq_bool(bool *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_bool(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq_double(double *array, int32 size) {
  if (size == 0)
    return make_empty_seq();
  else
    return make_slice_float(array, size);
}

OBJ build_seq_int64(int64 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 8) {
    int64 min = 0;
    int64 max = 0;

    for (uint32 i=0 ; i < size ; i++) {
      int64 elt = array[i];
      if (elt < min)
        min = elt;
      if (elt > max)
        max = elt;
    }

    if (min == 0 & max < 256) {
      int64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_uint8_init_at(elts, i, (uint8) array[i]);
      return make_seq_uint8_inline(elts, size);
    }

    if (size <= 4 & min >= -32768 & max < 32768) {
      uint64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_int16_init_at(elts, i, array[i]);
      return make_seq_int16_inline(elts, size);
    }

    if (size <= 2 & min >= -2147483648 & max < 2147483648) {
      uint64 data = inline_int32_init_at(0, 0, (int32) array[0]);
      if (size == 2)
        data = inline_int32_init_at(data, 1, (int32) array[1]);
      return make_seq_int32_inline(data, size);
    }
  }

  return make_slice_int64(array, size);
}

OBJ build_seq_int32(int32 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 0) {
    int32 min = 0;
    int32 max = 0;

    for (uint32 i=0 ; i < size ; i++) {
      int64 elt = array[i];
      if (elt < min)
        min = elt;
      if (elt > max)
        max = elt;
    }

    if (min == 0 & max < 256) {
      int64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_uint8_init_at(elts, i, (uint8) array[i]);
      return make_seq_uint8_inline(elts, size);
    }

    if (size <= 4 & min >= -32768 & max < 32768) {
      uint64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_int16_init_at(elts, i, array[i]);
      return make_seq_int16_inline(elts, size);
    }

    if (size <= 2) {
      uint64 data = inline_int32_init_at(0, 0, (int32) array[0]);
      if (size == 2)
        data = inline_int32_init_at(data, 1, (int32) array[1]);
      return make_seq_int32_inline(data, size);
    }
  }

  return make_slice_int32(array, size);
}

OBJ build_seq_int16(int16 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 8) {
    uint8 elts[8];
    for (int i=0 ; i < size ; i++) {
      int16 elt = array[i];
      if (elt >= 0 & elt < 256)
        elts[i] = (uint8) elt;
      else
        goto no_uint8;
    }
    return make_seq_uint8_inline(inline_uint8_pack(elts, size), size);
  }

no_uint8:
  if (size <= 4)
    return make_seq_int16_inline(inline_int16_pack(array, size), size);
  else
    return make_slice_int16(array, size);
}

OBJ build_seq_int8(int8 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 8) {
    bool non_neg = true;
    for (int i=0 ; i < size ; i++)
      if (array[i] < 0) {
        non_neg = false;
        break;
      }

    if (non_neg)
      return make_seq_uint8_inline(inline_uint8_pack((uint8 *) array, size), size);

    if (size <= 4) {
      uint64 elts = 0;
      for (int i=0 ; i < size ; i++)
        elts = inline_int16_init_at(elts, i, array[i]);
      return make_seq_int16_inline(elts, size);
    }
  }

  return make_slice_int8(array, size);
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
