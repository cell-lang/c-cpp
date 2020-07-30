#include "lib.h"


static uint32 next_capacity(uint32 start_size, uint32 min_size) {
  assert(start_size > 0);
  uint32 new_size = start_size;
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

SEQ_OBJ *adjust_float_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
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

  double *elts = seq_ptr->buffer.float_;

  SEQ_OBJ *new_seq_ptr = new_float_seq(new_length, next_capacity(capacity, new_length));
  double *new_seq_elts = new_seq_ptr->buffer.float_;

  memcpy(new_seq_elts, elts, length * sizeof(double));

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

SEQ_OBJ *adjust_int8_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
  return adjust_uint8_seq(seq_ptr, length, extra);
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

SEQ_OBJ *adjust_int32_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
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

  int32 *elts = seq_ptr->buffer.int32_;

  SEQ_OBJ *new_seq_ptr = new_int32_seq(new_length, next_capacity(capacity, new_length));
  int32 *new_seq_elts = new_seq_ptr->buffer.int32_;

  memcpy(new_seq_elts, elts, length * sizeof(int32));

  return new_seq_ptr;
}

SEQ_OBJ *adjust_int64_seq(SEQ_OBJ *seq_ptr, uint32 length, uint32 extra) {
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

  int64 *elts = seq_ptr->buffer.int64_;

  SEQ_OBJ *new_seq_ptr = new_int64_seq(new_length, next_capacity(capacity, new_length));
  int64 *new_seq_elts = new_seq_ptr->buffer.int64_;

  memcpy(new_seq_elts, elts, length * sizeof(int64));

  return new_seq_ptr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ concat_slow(OBJ left, OBJ right) {
  uint32 lenl = read_size_field(left);
  uint32 lenr = read_size_field(right);
  uint32 len = lenl + lenr;

  OBJ *elts = new_obj_array(len);
  for (int i=0 ; i < lenl ; i++)
    elts[i] = get_obj_at(left, i);
  for (int i=0 ; i < lenr ; i++)
    elts[i + lenl] = get_obj_at(right, i);
  return build_seq(elts, lenl + lenr);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct {
  INT_BITS_TAG bits_tag;
  bool is_signed;
} INT_INFO;

INT_INFO least_common_supertype(INT_INFO info1, INT_INFO info2) {
  INT_INFO supertype;

  if (info1.bits_tag != info2.bits_tag) {
    if (info1.bits_tag >= info2.bits_tag) {
      supertype.bits_tag =  info1.bits_tag;
      supertype.is_signed = info1.is_signed;
    }
    else {
      supertype.bits_tag = info2.bits_tag;
      supertype.is_signed = info2.is_signed;
    }
  }
  else if (info1.is_signed == info2.is_signed) {
    supertype.bits_tag = info1.bits_tag;
    supertype.is_signed = info1.is_signed;
  }
  else {
    assert(info1.bits_tag == INT_BITS_TAG_8 & info2.bits_tag == INT_BITS_TAG_8);
    supertype.bits_tag = INT_BITS_TAG_16;
    supertype.is_signed = true;
  }

  return supertype;
}

INT_INFO get_int_info(OBJ seq) {
  INT_INFO info;

  OBJ_TYPE type = get_obj_type(seq);

  if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    info.bits_tag = INT_BITS_TAG_8;
    info.is_signed = false;
  }
  else if (type == TYPE_NE_SEQ_INT16_INLINE) {
    info.bits_tag = INT_BITS_TAG_16;
    info.is_signed = true;
  }
  else if (type == TYPE_NE_SEQ_INT32_INLINE) {
    info.bits_tag = INT_BITS_TAG_32;
    info.is_signed = true;
  }
  else {
    assert(type == TYPE_NE_INT_SEQ);
    info.bits_tag = get_int_bits_tag(seq);
    info.is_signed = is_signed(seq);
  }

  return info;
}

typedef struct {
  int64 min;
  int64 max;
} INT_RANGE;

INT_RANGE get_actual_int_range(OBJ seq) {
  uint32 len = read_size_field(seq);

  INT_RANGE range;
  range.min = 0;
  range.max = 0;

  int64 buffer[8];

  for (uint32 i=0 ; i < len ; i += 8) {
    int count = len - i;
    if (count > 8)
    count = 8;
    copy_int64_range_unchecked(seq, i, count, buffer);
    for (int j=0 ; j < count ; j++) {
      int64 value = buffer[j];
      if (value < range.min)
        range.min = value;
      else if (value > range.max)
        range.max = value;
    }
  }

  return range;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ concat_ne_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  assert(get_obj_type(left) == TYPE_NE_SEQ);

  OBJ_TYPE typer = get_obj_type(right);

  if (typer == TYPE_NE_SEQ) {
    if (is_array_obj(left)) {
      SEQ_OBJ *ptrl = adjust_obj_seq(get_seq_ptr(left), lenl, lenr);
      memcpy(ptrl->buffer.obj + lenl, get_seq_elts_ptr(right), lenr * sizeof(OBJ));
      return make_seq(ptrl, lenl + lenr);
    }
    else {
      uint32 len = lenl + lenr;
      OBJ *eltsl = get_seq_elts_ptr(left);
      OBJ *eltsr = get_seq_elts_ptr(right);

      SEQ_OBJ *new_seq_ptr = new_obj_seq(len, next_capacity(16, len));
      memcpy(new_seq_ptr->buffer.obj, eltsl, lenl * sizeof(OBJ));
      memcpy(new_seq_ptr->buffer.obj + lenl, eltsr, lenr * sizeof(OBJ));
      return make_seq(new_seq_ptr, lenl + lenr);
    }
  }

  return concat_slow(left, right);
}

////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ concat_ne_float_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  assert(get_obj_type(left) == TYPE_NE_FLOAT_SEQ);

  if (get_obj_type(right) == TYPE_NE_FLOAT_SEQ) {
    if (is_array_obj(left)) {
      SEQ_OBJ *ptrl = adjust_float_seq(get_seq_ptr(left), lenl, lenr);
      memcpy(ptrl->buffer.float_ + lenl, get_seq_elts_ptr(right), lenr * sizeof(double));
      return make_seq(ptrl, lenl + lenr);
    }
    else {
      uint32 len = lenl + lenr;
      double *eltsl = get_seq_elts_ptr_float(left);
      double *eltsr = get_seq_elts_ptr_float(right);

      SEQ_OBJ *new_seq_ptr = new_float_seq(len, next_capacity(16, len));
      memcpy(new_seq_ptr->buffer.float_, eltsl, lenl * sizeof(double));
      memcpy(new_seq_ptr->buffer.float_ + lenl, eltsr, lenr * sizeof(double));
      return make_seq_float(new_seq_ptr, lenl + lenr);
    }
  }

  return concat_slow(left, right);
}

////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ concat_ne_int_seq_new(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  uint32 len = lenl + lenr;

  INT_INFO info = least_common_supertype(get_int_info(left), get_int_info(right));

  switch (info.bits_tag) {
    case INT_BITS_TAG_8: {
      if (info.is_signed) {
        SEQ_OBJ *seq_ptr = new_int8_seq(len, next_capacity(32, len));
        copy_int8_range_unchecked(left, 0, lenl, seq_ptr->buffer.int8_);
        copy_int8_range_unchecked(right, 0, lenr, seq_ptr->buffer.int8_ + lenl);
        return make_seq_int8(seq_ptr, len);
      }
      else {
        SEQ_OBJ *seq_ptr = new_uint8_seq(len, next_capacity(32, len));
        copy_uint8_range_unchecked(left, 0, lenl, seq_ptr->buffer.uint8_);
        copy_uint8_range_unchecked(right, 0, lenr, seq_ptr->buffer.uint8_ + lenl);
        return make_seq_uint8(seq_ptr, len);
      }
    }

    case INT_BITS_TAG_16: {
      SEQ_OBJ *seq_ptr = new_int16_seq(len, next_capacity(16, len));
      copy_int16_range_unchecked(left, 0, lenl, seq_ptr->buffer.int16_);
      copy_int16_range_unchecked(right, 0, lenr, seq_ptr->buffer.int16_ + lenl);
      return make_seq_int16(seq_ptr, len);
    }

    case INT_BITS_TAG_32: {
      SEQ_OBJ *seq_ptr = new_int32_seq(len, next_capacity(8, len));
      copy_int32_range_unchecked(left, 0, lenl, seq_ptr->buffer.int32_);
      copy_int32_range_unchecked(right, 0, lenr, seq_ptr->buffer.int32_ + lenl);
      return make_seq_int32(seq_ptr, len);
    }

    case INT_BITS_TAG_64: {
      SEQ_OBJ *seq_ptr = new_int32_seq(len, next_capacity(4, len));
      copy_int32_range_unchecked(left, 0, lenl, seq_ptr->buffer.int32_);
      copy_int32_range_unchecked(right, 0, lenr, seq_ptr->buffer.int32_ + lenl);
      return make_seq_int32(seq_ptr, len);
    }

    default:
      internal_fail();
  }
}

__attribute__ ((noinline)) OBJ concat_ne_int_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  assert(get_obj_type(left) == TYPE_NE_INT_SEQ);

  uint32 len = lenl + lenr;

  INT_BITS_TAG left_bits_tag = get_int_bits_tag(left);
  OBJ_TYPE typer = get_obj_type(right);

  INT_INFO right_info = get_int_info(right);

  if (is_array_obj(left)) {
    switch (get_int_bits_tag(left)) {
      case INT_BITS_TAG_8: {
        if (right_info.bits_tag == INT_BITS_TAG_8) {
          if (is_signed(left)) {
            if (right_info.is_signed) {
              SEQ_OBJ *ptrl = adjust_int8_seq(get_seq_ptr(left), lenl, lenr);
              copy_int8_range_unchecked(right, 0, lenr, ptrl->buffer.int8_ + lenl);
              return make_seq_int8(ptrl, len);
            }
          }
          else {
            if (!right_info.is_signed) {
              SEQ_OBJ *ptrl = adjust_uint8_seq(get_seq_ptr(left), lenl, lenr);
              copy_uint8_range_unchecked(right, 0, lenr, ptrl->buffer.uint8_ + lenl);
              return make_seq_uint8(ptrl, len);
            }
          }
        }

        break;
      }

      case INT_BITS_TAG_16: {
        if (right_info.bits_tag <= INT_BITS_TAG_16) {
          SEQ_OBJ *ptrl = adjust_int16_seq(get_seq_ptr(left), lenl, lenr);
          copy_int16_range_unchecked(right, 0, lenr, ptrl->buffer.int16_ + lenl);
          return make_seq_int16(ptrl, len);
        }

        break;
      }

      case INT_BITS_TAG_32: {
        if (right_info.bits_tag <= INT_BITS_TAG_32) {
          SEQ_OBJ *ptrl = adjust_int32_seq(get_seq_ptr(left), lenl, lenr);
          copy_int32_range_unchecked(right, 0, lenr, ptrl->buffer.int32_ + lenl);
          return make_seq_int32(ptrl, len);
        }

        break;
      }

      case INT_BITS_TAG_64: {
        if (right_info.bits_tag <= INT_BITS_TAG_64) {
          SEQ_OBJ *ptrl = adjust_int64_seq(get_seq_ptr(left), lenl, lenr);
          copy_int64_range_unchecked(right, 0, lenr, ptrl->buffer.int64_ + lenl);
          return make_seq_int64(ptrl, len);
        }

        break;
      }

      default:
        internal_fail();
    }
  }

  return concat_ne_int_seq_new(left, lenl, right, lenr);
}

__attribute__ ((noinline)) OBJ concat_ne_uint8_inline_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  uint32 len = lenl + lenr;

  if (len <= 8) {
    INT_RANGE range = get_actual_int_range(right);
    if (is_uint8_range(range.min, range.max)) {
      uint8 buffer[8];
      copy_uint8_range_unchecked(right, 0, lenr, buffer);
      uint64 data = left.core_data.int_;
      for (int i=0 ; i < lenr ; i++)
        data = inline_uint8_init_at(data, i + lenl, buffer[i]);
      return make_seq_uint8_inline(data, len);
    }
  }

  return concat_ne_int_seq_new(left, lenl, right, lenr);
}

__attribute__ ((noinline)) OBJ concat_ne_int16_inline_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  uint32 len = lenl + lenr;

  if (len <= 4) {
    INT_RANGE range = get_actual_int_range(right);
    if (is_int16_range(range.min, range.max)) {
      int16 buffer[4];
      copy_int16_range_unchecked(right, 0, lenr, buffer);
      uint64 data = left.core_data.int_;
      for (int i=0 ; i < lenr ; i++)
        data = inline_int16_init_at(data, i + lenl, buffer[i]);
      return make_seq_int16_inline(data, len);
    }
  }

  return concat_ne_int_seq_new(left, lenl, right, lenr);
}

__attribute__ ((noinline)) OBJ concat_ne_int32_inline_seq(OBJ left, uint32 lenl, OBJ right, uint32 lenr) {
  uint32 len = lenl + lenr;

  if (len <= 2) {
    assert(lenl == 1 & lenr == 1);
    int64 value = get_int_at(right, 0); //## WE COULD USE AN UNCHECKED VERSION HERE MAYBE
    if (is_int32(value)) {
      int64 data = inline_int32_init_at(left.core_data.int_, 1, (int32) value);
      return make_seq_int32_inline(data, 2);
    }
  }

  return concat_ne_int_seq_new(left, lenl, right, lenr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ concat(OBJ left, OBJ right) {
  uint64 lenr = read_size_field(right);
  if (lenr == 0)
    return left;

  uint64 lenl = read_size_field(left);
  if (lenl == 0)
    return right;

  if (lenl + lenr > 0xFFFFFFFF)
    impl_fail("_cat_(): Resulting sequence is too large");

  switch (get_obj_type(left)) {
    case TYPE_NE_SEQ_UINT8_INLINE:
      return concat_ne_uint8_inline_seq(left, lenl, right, lenr);

    case TYPE_NE_SEQ_INT16_INLINE:
      return concat_ne_int16_inline_seq(left, lenl, right, lenr);

    case TYPE_NE_SEQ_INT32_INLINE:
      return concat_ne_int32_inline_seq(left, lenl, right, lenr);

    case TYPE_NE_INT_SEQ:
      return concat_ne_int_seq(left, lenl, right, lenr);

    case TYPE_NE_FLOAT_SEQ:
      return concat_ne_float_seq(left, lenl, right, lenr);

    case TYPE_NE_BOOL_SEQ:
      internal_fail();

    case TYPE_NE_SEQ:
      return concat_ne_seq(left, lenl, right, lenr);

    default:
      internal_fail();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) OBJ append_slow(OBJ seq, OBJ obj) {
  uint32 len = read_size_field(seq);

  SEQ_OBJ *seq_ptr = new_obj_seq(len + 1, next_capacity(4, len + 1));

  for (uint32 i=0 ; i < len ; i++)
    seq_ptr->buffer.obj[i] = get_obj_at(seq, i);
  seq_ptr->buffer.obj[len] = obj;

  return make_seq(seq_ptr, len + 1);
}

__attribute__ ((noinline)) OBJ append_slow_int16(OBJ seq, int16 value) {
  uint32 len = read_size_field(seq);

  SEQ_OBJ *seq_ptr = new_int16_seq(len + 1, next_capacity(8, len + 1));

  for (uint32 i=0 ; i < len ; i++)
    seq_ptr->buffer.int16_[i] = (int16) get_int_at(seq, i);
  seq_ptr->buffer.int16_[len] = value;

  return make_seq_int16(seq_ptr, len + 1);
}

__attribute__ ((noinline)) OBJ append_slow_int32(OBJ seq, int32 value) {
  uint32 len = read_size_field(seq);

  SEQ_OBJ *seq_ptr = new_int32_seq(len + 1, next_capacity(8, len + 1));

  for (uint32 i=0 ; i < len ; i++)
    seq_ptr->buffer.int32_[i] = (int32) get_int_at(seq, i);
  seq_ptr->buffer.int32_[len] = value;

  return make_seq_int32(seq_ptr, len + 1);
}

__attribute__ ((noinline)) OBJ append_slow_int64(OBJ seq, int64 value) {
  uint32 len = read_size_field(seq);

  SEQ_OBJ *seq_ptr = new_int64_seq(len + 1, next_capacity(4, len + 1));
  for (uint32 i=0 ; i < len ; i++)
    seq_ptr->buffer.int64_[i] = get_int_at(seq, i);
  seq_ptr->buffer.int64_[len] = value;

  return make_seq_int64(seq_ptr, len + 1);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline OBJ append_ne_seq_obj(OBJ seq, uint32 len, OBJ obj) {
  if (is_array_obj(seq)) {
    SEQ_OBJ *seq_ptr = adjust_obj_seq(get_seq_ptr(seq), len, 1);
    seq_ptr->buffer.obj[len] = obj;
    return make_seq(seq_ptr, len + 1);
  }
  else {
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

    return append_slow(seq, obj);
  }
}

inline OBJ append_ne_seq_float(OBJ seq, uint32 len, OBJ obj) {
  if (is_float(obj)) {
    double value = get_float(obj);

    if (is_array_obj(seq)) {
      SEQ_OBJ *seq_ptr = adjust_float_seq(get_seq_ptr(seq), len, 1);
      seq_ptr->buffer.float_[len] = value;
      return make_seq_float(seq_ptr, len + 1);
    }
    else {
      double *elts = get_seq_elts_ptr_float(seq);

      if (len == 1) {
        double *new_elts = new_float_array(2);
        new_elts[0] = elts[0];
        new_elts[1] = value;
        return make_slice_float(new_elts, 2);
      }

      if (len == 2) {
        double *new_elts = new_float_array(3);
        new_elts[0] = elts[0];
        new_elts[1] = elts[1];
        new_elts[2] = value;
        return make_slice_float(new_elts, 3);
      }

      SEQ_OBJ *seq_ptr = new_float_seq(len + 1, next_capacity(8, len + 1));
      memcpy(seq_ptr->buffer.float_, elts, len * sizeof(double));
      seq_ptr->buffer.float_[len] = value;
      return make_seq_float(seq_ptr, len + 1);
    }
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_uint8(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_uint8(value)) {
      if (is_array_obj(seq)) {
        SEQ_OBJ *seq_ptr = adjust_uint8_seq(get_seq_ptr(seq), len, 1);
        seq_ptr->buffer.uint8_[len] = (uint8) value;
        return make_seq_uint8(seq_ptr, len + 1);
      }

      SEQ_OBJ *seq_ptr = new_uint8_seq(len + 1, next_capacity(16, len + 1));
      memcpy(seq_ptr->buffer.uint8_, get_seq_elts_ptr_uint8(seq), len * sizeof(uint8));
      seq_ptr->buffer.uint8_[len] = (uint8) value;
      return make_seq_uint8(seq_ptr, len + 1);
    }

    if (is_int16(value))
      return append_slow_int16(seq, (int16) value);

    if (is_int32(value))
      return append_slow_int32(seq, (int32) value);

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int8(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_int8(value)) {
      if (is_array_obj(seq)) {
        SEQ_OBJ *seq_ptr = adjust_int8_seq(get_seq_ptr(seq), len, 1);
        seq_ptr->buffer.int8_[len] = (int8) value;
        return make_seq_int8(seq_ptr, len + 1);
      }

      SEQ_OBJ *seq_ptr = new_int8_seq(len + 1, next_capacity(16, len + 1));
      memcpy(seq_ptr->buffer.int8_, get_seq_elts_ptr_int8(seq), len * sizeof(int8));
      seq_ptr->buffer.int8_[len] = (int8) value;
      return make_seq_int8(seq_ptr, len + 1);
    }

    if (is_int16(value))
      return append_slow_int16(seq, (int16) value);

    if (is_int32(value))
      return append_slow_int32(seq, (int32) value);

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int16(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_int16(value)) {
      if (is_array_obj(seq)) {
        SEQ_OBJ *seq_ptr = adjust_int16_seq(get_seq_ptr(seq), len, 1);
        seq_ptr->buffer.int16_[len] = (int16) value;
        return make_seq_int16(seq_ptr, len + 1);
      }

      return append_slow_int16(seq, (int16) value);
    }

    if (is_int32(value))
      return append_slow_int32(seq, (int32) value);

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int32(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_int32(value)) {
      if (is_array_obj(seq)) {
        SEQ_OBJ *seq_ptr = adjust_int32_seq(get_seq_ptr(seq), len, 1);
        seq_ptr->buffer.int32_[len] = (uint32) value;
        return make_seq_int32(seq_ptr, len + 1);
      }

      return append_slow_int32(seq, (int32) value);
    }

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int64(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_array_obj(seq)) {
      SEQ_OBJ *seq_ptr = adjust_int64_seq(get_seq_ptr(seq), len, 1);
      seq_ptr->buffer.int64_[len] = value;
      return make_seq_int64(seq_ptr, len + 1);
    }

    return append_slow_int64(seq, get_int(obj));
  }

  return append_slow(seq, obj);
}

OBJ append_ne_seq_int(OBJ seq, uint32 len, OBJ obj) {
  INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

  if (bits_tag == INT_BITS_TAG_8)
    if (is_signed(seq))
      return append_ne_seq_int8(seq, len, obj);
    else
      return append_ne_seq_uint8(seq, len, obj);

  if (bits_tag == INT_BITS_TAG_16)
    return append_ne_seq_int16(seq, len, obj);

  if (bits_tag == INT_BITS_TAG_32)
    return append_ne_seq_int32(seq, len, obj);

  assert(bits_tag == INT_BITS_TAG_64);
  return append_ne_seq_int64(seq, len, obj);

}

////////////////////////////////////////////////////////////////////////////////

inline OBJ append_ne_seq_uint8_inline(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_uint8(value)) {
      if (len < 8)
        return make_seq_uint8_inline(inline_uint8_init_at(seq.core_data.int_, len, value), len + 1);

      assert(len == 8);

      SEQ_OBJ *seq_ptr = new_uint8_seq(9, 16);
      for (int i=0 ; i < 8 ; i++)
        seq_ptr->buffer.uint8_[i] = inline_uint8_at(seq.core_data.int_, i);
      seq_ptr->buffer.uint8_[8] = (uint8) value;
      return make_seq_uint8(seq_ptr, 9);
    }

    if (is_int16(value)) {
      assert(((int16) value) == value);

      if (len < 4) {
        uint64 packed_array = 0;
        for (int i=0 ; i < len ; i++)
          packed_array = inline_int16_init_at(packed_array, i, inline_uint8_at(obj.core_data.int_, i));
        packed_array = inline_int16_init_at(packed_array, len, (int16) value);
        return make_seq_int16_inline(packed_array, len + 1);
      }

      return append_slow_int16(seq, value);
    }

    if (is_int32(value)) {
      if (len == 1)
        return make_seq_int32_inline(inline_int32_init_at(seq.core_data.int_, 1, (uint32) value), 2);
      else
        return append_slow_int32(seq, value);
    }

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int16_inline(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_int16(value)) {
      if (len < 4)
        return make_seq_int16_inline(inline_int16_init_at(seq.core_data.int_, len, value), len + 1);
      else
        return append_slow_int16(seq, value);
    }

    if (is_int32(value)) {
      if (len == 1)
        return make_seq_int32_inline(inline_int32_init_at(seq.core_data.int_, 1, (uint32) value), 2);
      else
        return append_slow_int32(seq, value);
    }

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

inline OBJ append_ne_seq_int32_inline(OBJ seq, uint32 len, OBJ obj) {
  if (is_int(obj)) {
    int64 value = get_int(obj);

    if (is_int32(value)) {
      if (len == 1)
        return make_seq_int32_inline(inline_int32_init_at(seq.core_data.int_, 1, (uint32) value), 2);
      else
        return append_slow_int32(seq, value);
    }

    return append_slow_int64(seq, value);
  }

  return append_slow(seq, obj);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ append(OBJ seq, OBJ obj) {
  if (is_empty_seq(seq)) {
    if (is_int(obj)) {
      int64 value = get_int(obj);

      if (is_uint8(value)) {
        assert(get_int_at(make_seq_uint8_inline(value, 1), 0) == value);
        assert(read_size_field(make_seq_uint8_inline(value, 1)) == 1);

        return make_seq_uint8_inline(value, 1);
      }

      if (is_int16(value)) {
        assert(get_int_at(make_seq_int16_inline(value, 1), 0) == value);
        assert(read_size_field(make_seq_int16_inline(value, 1)) == 1);

        return make_seq_int16_inline(value, 1);
      }

      if (is_int32(value)) {
        assert(get_int_at(make_seq_int32_inline(value, 1), 0) == value);
        assert(read_size_field(make_seq_int16_inline(value, 1)) == 1);

        return make_seq_int32_inline(value, 1);
      }

      int64 *ptr = new_int64_array(1);
      ptr[0] = value;
      return make_slice_int64(ptr, 1);
    }

    if (is_float(obj)) {
      double *ptr = new_float_array(1);
      ptr[0] = get_float(obj);
      return make_slice_float(ptr, 1);
    }

    OBJ *ptr = new_obj_array(1);
    ptr[0] = obj;
    return make_slice(ptr, 1);
  }

  uint32 len = read_size_field(seq);

  // Checking that the new sequence doesn't overflow
  if (len == 0xFFFFFFFF)
    impl_fail("Resulting sequence is too large");

  switch (get_obj_type(seq)) {
    case TYPE_NE_SEQ_UINT8_INLINE:
      return append_ne_seq_uint8_inline(seq, len, obj);

    case TYPE_NE_SEQ_INT16_INLINE:
      return append_ne_seq_int16_inline(seq, len, obj);

    case TYPE_NE_SEQ_INT32_INLINE:
      return append_ne_seq_int32_inline(seq, len, obj);

    case TYPE_NE_INT_SEQ:
      return append_ne_seq_int(seq, len, obj);

    case TYPE_NE_FLOAT_SEQ:
      return append_ne_seq_float(seq, len, obj);

    case TYPE_NE_BOOL_SEQ:
      internal_fail();

    case TYPE_NE_SEQ:
      return append_ne_seq_obj(seq, len, obj);

    default:
      internal_fail();
  }
}
