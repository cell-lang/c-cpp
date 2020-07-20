#include "lib.h"


bool no_sum32_overflow(uint64 x, uint64 y) {
  return x + y <= 0XFFFFFFFF;
}


static uint32 next_capacity(uint32 curr_size, uint32 min_size) {
  uint32 new_size = curr_size != 0 ? 2 * curr_size : 32;
  while (new_size < min_size)
    new_size *= 2;
  return new_size;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ *adjust_obj_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
  assert(no_sum32_overflow(length, extra));

  uint32 new_length = length + extra;
  uint32 used = seq_ptr->used;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = length == used;
  bool has_needed_spare_capacity = used + extra <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended) {
    seq_ptr->used += extra;
    return seq_ptr;
  }

  OBJ *elts = seq_ptr->buffer.obj;

  SEQ_OBJ *new_seq_ptr = new_obj_seq(new_length, next_capacity(capacity, new_length));
  OBJ *new_seq_elts = new_seq_ptr->buffer.obj;

  memcpy(new_seq_elts, elts, length * sizeof(OBJ));

  return new_seq_ptr;
}

SEQ_OBJ *adjust_uint8_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
  assert(no_sum32_overflow(length, extra));

  uint32 new_length = length + extra;
  uint32 used = seq_ptr->used;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = length == used;
  bool has_needed_spare_capacity = used + extra <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended) {
    seq_ptr->used += extra;
    return seq_ptr;
  }

  uint8 *elts = seq_ptr->buffer.uint8_;

  SEQ_OBJ *new_seq_ptr = new_uint8_seq(new_length, next_capacity(capacity, new_length));
  uint8 *new_seq_elts = new_seq_ptr->buffer.uint8_;

  memcpy(new_seq_elts, elts, length * sizeof(uint8));

  return new_seq_ptr;
}

SEQ_OBJ *adjust_int16_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
  assert(no_sum32_overflow(length, extra));

  uint32 new_length = length + extra;
  uint32 used = seq_ptr->used;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = length == used;
  bool has_needed_spare_capacity = used + extra <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended) {
    seq_ptr->used += extra;
    return seq_ptr;
  }

  int16 *elts = seq_ptr->buffer.int16_;

  SEQ_OBJ *new_seq_ptr = new_int16_seq(new_length, next_capacity(capacity, new_length));
  int16 *new_seq_elts = new_seq_ptr->buffer.int16_;

  memcpy(new_seq_elts, elts, length * sizeof(int16));

  return new_seq_ptr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ concat_obj(OBJ *elts1, uint32 len1, OBJ *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_obj_seq(len, next_capacity(16, len));
  memcpy(new_seq_ptr->buffer.obj, elts1, len1 * sizeof(OBJ));
  memcpy(new_seq_ptr->buffer.obj + len1, elts2, len2 * sizeof(OBJ));
  return make_seq(new_seq_ptr, len1 + len2);
}

OBJ concat_uint8(uint8 *elts1, uint32 len1, uint8 *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_uint8_seq(len, next_capacity(16, len));
  memcpy(new_seq_ptr->buffer.uint8_, elts1, len1 * sizeof(uint8));
  memcpy(new_seq_ptr->buffer.uint8_ + len1, elts2, len2 * sizeof(uint8));
  return make_seq_uint8(new_seq_ptr, len1 + len2);
}

OBJ concat_int16(int16 *elts1, uint32 len1, int16 *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_int16_seq(len, next_capacity(16, len));
  memcpy(new_seq_ptr->buffer.int16_, elts1, len1 * sizeof(int16));
  memcpy(new_seq_ptr->buffer.int16_ + len1, elts2, len2 * sizeof(int16));
  return make_seq_int16(new_seq_ptr, len1 + len2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ concat_slow(OBJ left, OBJ right) {
  SEQ_OBJ *seq_ptr;

  uint32 lenl = get_seq_length(left);
  uint32 lenr = get_seq_length(right);
  uint32 len = lenl + lenr;

  if (get_physical_type(left) == TYPE_NE_SEQ) {
    seq_ptr = adjust_obj_seq(get_seq_ptr(left), lenl, lenr);
  }
  else {
    seq_ptr = new_obj_seq(len, next_capacity(2, len));
    for (int i=0 ; i < lenl ; i++)
      seq_ptr->buffer.obj[i] = get_obj_at(left, i);
  }

  for (int i=0 ; i < lenr ; i++)
    seq_ptr->buffer.obj[i + lenl] = get_obj_at(right, i);

  return make_seq(seq_ptr, lenl + lenr);
}

__attribute__ ((noinline)) OBJ concat_slow_int16(OBJ left, OBJ right) {
  SEQ_OBJ *seq_ptr;

  uint32 lenl = get_seq_length(left);
  uint32 lenr = get_seq_length(right);
  uint32 len = lenl + lenr;

  if (get_physical_type(left) == TYPE_NE_SEQ_INT16) {
    seq_ptr = adjust_int16_seq(get_seq_ptr(left), lenl, lenr);
  }
  else {
    seq_ptr = new_int16_seq(len, next_capacity(4, len));
    for (int i=0 ; i < lenl ; i++)
      seq_ptr->buffer.int16_[i] = (int16) get_int_at(left, i);
  }

  for (int i=0 ; i < lenr ; i++)
    seq_ptr->buffer.int16_[i + lenl] = (int16) get_int_at(right, i);

  return make_seq_int16(seq_ptr, lenl + lenr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline OBJ concat_ne_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ | typer == TYPE_NE_SLICE) {
    SEQ_OBJ *ptrl = adjust_obj_seq(get_seq_ptr(left), lenl, lenr);
    memcpy(ptrl->buffer.obj + lenl, get_seq_elts_ptr(right), lenr * sizeof(OBJ));
    return make_seq(ptrl, lenl + lenr);
  }

  return concat_slow(left, right);
}

inline OBJ concat_ne_slice(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  OBJ *eltsl = get_seq_elts_ptr(left);
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ | typer == TYPE_NE_SLICE)
    return concat_obj(eltsl, lenl, get_seq_elts_ptr(right), lenr);

  return concat_slow(make_slice(eltsl, lenl), right);
}

inline OBJ concat_ne_seq_uint8(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  SEQ_OBJ *ptrl = get_seq_ptr(left);
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8) {
    ptrl = adjust_uint8_seq(ptrl, lenl, lenr);
    memcpy(ptrl->buffer.uint8_ + lenl, get_seq_elts_ptr_uint8(right), lenr * sizeof(uint8));
    return make_seq_uint8(ptrl, lenl + lenr);
  }

  if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
    ptrl = adjust_uint8_seq(ptrl, lenl, lenr);
    for (int i=0 ; i < lenr ; i++)
      ptrl->buffer.uint8_[i + lenl] = inline_uint8_at(right.core_data.int_, i);
    return make_seq_uint8(ptrl, lenl + lenr);
  }

  if (typer == TYPE_NE_SEQ_INT16 | typer == TYPE_NE_SLICE_INT16 | typer == TYPE_NE_SEQ_INT16_INLINE)
    return concat_slow_int16(left, right);

  return concat_slow(left, right);
}

inline OBJ concat_ne_slice_uint8(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  uint8 *eltsl = get_seq_elts_ptr_uint8(left);
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
    return concat_uint8(eltsl, lenl, get_seq_elts_ptr_uint8(right), lenr);

  if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
    uint32 len = lenl + lenr;
    SEQ_OBJ *seq_ptr = new_uint8_seq(len, next_capacity(16, len));
    memcpy(seq_ptr->buffer.uint8_, get_seq_elts_ptr_uint8(left), lenl * sizeof(uint8));
    for (int i=0 ; i < lenr ; i++)
      seq_ptr->buffer.uint8_[i + lenl] = inline_uint8_at(right.core_data.int_, i);
    return make_seq_uint8(seq_ptr, len);
  }

  if (typer == TYPE_NE_SEQ_INT16 | typer == TYPE_NE_SLICE_INT16 | typer == TYPE_NE_SEQ_INT16_INLINE)
    return concat_slow_int16(left, right);

  return concat_slow(left, right);
}

inline OBJ concat_ne_seq_uint8_inline(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
    uint32 len = lenl + lenr;
    if (len <= 8) {
      uint64 elts = inline_uint8_concat(left.core_data.int_, lenl, right.core_data.int_, lenr);
      return make_seq_uint8_inline(elts, len);
    }
    else {
      SEQ_OBJ *seq_ptr = new_uint8_seq(len, next_capacity(16, len));
      for (int i=0 ; i < lenl ; i++)
        seq_ptr->buffer.uint8_[i] = inline_uint8_at(left.core_data.int_, i);
      for (int i=0 ; i < lenr ; i++)
        seq_ptr->buffer.uint8_[i + lenl] = inline_uint8_at(right.core_data.int_, i);
      return make_seq_uint8(seq_ptr, len);
    }
  }

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8) {
    uint32 len = lenl + lenr;
    SEQ_OBJ *seq_ptr = new_uint8_seq(len, next_capacity(16, len));
    for (int i=0 ; i < lenl ; i++)
      seq_ptr->buffer.uint8_[i] = inline_uint8_at(left.core_data.int_, i);
    memcpy(seq_ptr->buffer.uint8_ + lenl, get_seq_elts_ptr_uint8(right), lenr * sizeof(uint8));
    return make_seq_uint8(seq_ptr, len);
  }

  if (typer == TYPE_NE_SEQ_INT16 | typer == TYPE_NE_SLICE_INT16 | typer == TYPE_NE_SEQ_INT16_INLINE)
    return concat_slow_int16(left, right);

  return concat_slow(left, right);
}

inline OBJ concat_ne_seq_int16(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  SEQ_OBJ *ptrl = get_seq_ptr(left);
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ_INT16 | typer == TYPE_NE_SLICE_INT16) {
    ptrl = adjust_int16_seq(ptrl, lenl, lenr);
    memcpy(ptrl->buffer.int16_ + lenl, get_seq_elts_ptr_int16(right), lenr * sizeof(int16));
    return make_seq_int16(ptrl, lenl + lenr);
  }

  if (typer == TYPE_NE_SEQ_INT16_INLINE) {
    ptrl = adjust_int16_seq(ptrl, lenl, lenr);
    for (int i=0 ; i < lenr ; i++)
      ptrl->buffer.int16_[i + lenl] = inline_int16_at(right.core_data.int_, i);
    return make_seq_int16(ptrl, lenl + lenr);
  }

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8 | typer == TYPE_NE_SEQ_UINT8_INLINE)
    return concat_slow_int16(left, right);

  return concat_slow(left, right);
}

inline OBJ concat_ne_slice_int16(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  int16 *eltsl = get_seq_elts_ptr_int16(left);
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ_INT16 | typer == TYPE_NE_SLICE_INT16)
    return concat_int16(eltsl, lenl, get_seq_elts_ptr_int16(right), lenr);

  if (typer == TYPE_NE_SEQ_INT16_INLINE)
    return concat_slow_int16(left, right);

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8 | typer == TYPE_NE_SEQ_UINT8_INLINE)
    return concat_slow_int16(left, right);

  return concat_slow(left, right);
}

inline OBJ concat_ne_seq_int16_inline(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  OBJ_TYPE typer = get_physical_type(right);

  if (typer == TYPE_NE_SEQ_INT16_INLINE & lenl + lenr <= 4) {
    uint64 elts = inline_int16_concat(left.core_data.int_, lenl, right.core_data.int_, lenr);
    return make_seq_int16_inline(elts, lenl + lenr);
  }

  if (typer == TYPE_NE_SEQ_INT16 | typer == TYPE_NE_SLICE_INT16 | typer == TYPE_NE_SEQ_INT16_INLINE)
    return concat_slow_int16(left, right);

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8 | typer == TYPE_NE_SEQ_UINT8_INLINE)
    return concat_slow_int16(left, right);

  return concat_slow(left, right);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ concat(OBJ left, OBJ right) {
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

  switch (typel) {
    case TYPE_NE_SEQ:
      return concat_ne_seq(left, lenl, right, lenr);

    case TYPE_NE_SLICE:
      return concat_ne_slice(left, lenl, right, lenr);

    case TYPE_NE_SEQ_UINT8:
      return concat_ne_seq_uint8(left, lenl, right, lenr);

    case TYPE_NE_SLICE_UINT8:
      return concat_ne_slice_uint8(left, lenl, right, lenr);

    case TYPE_NE_SEQ_INT16:
      return concat_ne_seq_int16(left, lenl, right, lenr);

    case TYPE_NE_SLICE_INT16:
      return concat_ne_slice_int16(left, lenl, right, lenr);

    case TYPE_NE_SEQ_UINT8_INLINE:
      return concat_ne_seq_uint8_inline(left, lenl, right, lenr);

    case TYPE_NE_SEQ_INT16_INLINE:
      return concat_ne_seq_int16_inline(left, lenl, right, lenr);

    default:
      internal_fail();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ append_slow(OBJ seq, OBJ obj) {
  uint32 len = get_seq_length(seq);

  SEQ_OBJ *seq_ptr = new_obj_seq(len + 1, next_capacity(4, len + 1));

  for (int i=0 ; i < len ; i++)
    seq_ptr->buffer.obj[i] = get_obj_at(seq, i);
  seq_ptr->buffer.obj[len] = obj;

  return make_seq(seq_ptr, len + 1);
}

//## THIS FUNCTION SHOULD NOT EXIST. IT'S ONLY USED WITH A FULL UINT8 INLINE SEQUENCE
__attribute__ ((noinline)) OBJ append_slow_uint8(OBJ seq, uint8 value) {
  uint32 len = get_seq_length(seq);

  SEQ_OBJ *seq_ptr = new_uint8_seq(len + 1, next_capacity(16, len + 1));

  for (int i=0 ; i < len ; i++)
    seq_ptr->buffer.uint8_[i] = (uint8) get_int_at(seq, i);
  seq_ptr->buffer.uint8_[len] = value;

  return make_seq_uint8(seq_ptr, len + 1);
}

__attribute__ ((noinline)) OBJ append_slow_int16(OBJ seq, int16 value) {
  uint32 len = get_seq_length(seq);

  SEQ_OBJ *seq_ptr = new_int16_seq(len + 1, next_capacity(8, len + 1));

  for (int i=0 ; i < len ; i++)
    seq_ptr->buffer.int16_[i] = (int16) get_int_at(seq, i);
  seq_ptr->buffer.int16_[len] = value;

  return make_seq_int16(seq_ptr, len + 1);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline OBJ append_ne_seq(OBJ seq, uint32 len, OBJ obj) {
  SEQ_OBJ *seq_ptr = adjust_obj_seq(get_seq_ptr(seq), len, 1);
  seq_ptr->buffer.obj[len] = obj;
  return make_seq(seq_ptr, len + 1);
}

inline OBJ append_ne_slice(OBJ seq, uint32 len, OBJ obj) {
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

inline OBJ append_ne_seq_uint8(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (value >= 0 & value < 256) {
      SEQ_OBJ *seq_ptr = adjust_uint8_seq(get_seq_ptr(seq), len, 1);
      seq_ptr->buffer.uint8_[len] = (uint8) value;
      return make_seq_uint8(seq_ptr, len + 1);
    }

    if (value >= -32768 & value < 32768)
      return append_slow_int16(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_slice_uint8(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (value >= 0 & value < 256) {
      uint8 value_uint8 = (uint8) value;
      return concat_uint8(get_seq_elts_ptr_uint8(seq), len, &value_uint8, 1);
    }

    if (value >= -32768 & value < 32768)
      append_slow_int16(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_uint8_inline(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (value >= 0 & value < 256) {
      if (len < 8)
        return make_seq_uint8_inline(inline_uint8_init_at(seq.core_data.int_, len, value), len + 1);
      else
        return append_slow_uint8(seq, value);
    }

    if (value >= -32768 & value < 32768) {
      assert(((int16) value) == value);

      if (len < 4) {
        uint64 packed_array = 0;
        for (int i=0 ; i < len ; i++)
          packed_array = inline_int16_init_at(packed_array, i, inline_uint8_at(obj.core_data.int_, i));
        packed_array = inline_int16_init_at(packed_array, len, (int16) value);
        return make_seq_int16_inline(packed_array, len + 1);
      }
      else
        return append_slow_int16(seq, value);
    }
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int16(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (value >= -32768 & value < 32768) {
      SEQ_OBJ *seq_ptr = adjust_int16_seq(get_seq_ptr(seq), len, 1);
      seq_ptr->buffer.int16_[len] = (int16) value;
      return make_seq_int16(seq_ptr, len + 1);
    }
  }

  return append_slow(seq, obj);

}

inline OBJ append_ne_slice_int16(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (value >= -32768 & value < 32768) {
      int16 int16_value = (int16) value;
      return concat_int16(get_seq_elts_ptr_int16(seq), len, &int16_value, 1);
    }
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int16_inline(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (value >= -32768 & value < 32768) {
      if (len < 4)
        return make_seq_int16_inline(inline_int16_init_at(seq.core_data.int_, len, value), len + 1);
      else
        return append_slow_int16(seq, value);
    }
  }

  return append_slow(seq, obj);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ append(OBJ seq, OBJ obj) {
  if (is_empty_seq(seq)) {
    if (is_int(obj)) {
      int64 value = get_int(obj);
      if (value >= 0 & value < 256) {
        assert(get_int_at(make_seq_uint8_inline(value, 1), 0) == value);
        assert(get_seq_length(make_seq_uint8_inline(value, 1)) == 1);

        return make_seq_uint8_inline(value, 1);
      }

      if (value >= -32768 & value < 32768) {
        assert(get_int_at(make_seq_int16_inline(value, 1), 0) == value);
        assert(get_seq_length(make_seq_int16_inline(value, 1)) == 1);

        return make_seq_int16_inline(value, 1);
      }
    }

    OBJ *ptr = new_obj_array(1);
    ptr[0] = obj;
    return make_slice(ptr, 1);
  }

  uint32 len = get_seq_length(seq);

  // Checking that the new sequence doesn't overflow
  if (len == 0xFFFFFFFF)
    impl_fail("Resulting sequence is too large");

  switch (get_physical_type(seq)) {
    case TYPE_NE_SEQ:
      return append_ne_seq(seq, len, obj);

    case TYPE_NE_SLICE:
      return append_ne_slice(seq, len, obj);

    case TYPE_NE_SEQ_UINT8:
      return append_ne_seq_uint8(seq, len, obj);

    case TYPE_NE_SLICE_UINT8:
      return append_ne_slice_uint8(seq, len, obj);

    case TYPE_NE_SEQ_UINT8_INLINE:
      return append_ne_seq_uint8_inline(seq, len, obj);

    case TYPE_NE_SEQ_INT16:
      return append_ne_seq_int16(seq, len, obj);

    case TYPE_NE_SLICE_INT16:
      return append_ne_slice_int16(seq, len, obj);

    case TYPE_NE_SEQ_INT16_INLINE:
      return append_ne_seq_int16_inline(seq, len, obj);

    default:
      internal_fail();
  }
}
