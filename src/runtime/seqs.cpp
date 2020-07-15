#include "lib.h"


OBJ get_obj_at(OBJ seq, int64 idx) {
  assert(is_seq(seq));

  if (((uint64) idx) >= get_seq_length(seq))
    soft_fail("Invalid sequence index");

  OBJ_TYPE type = get_physical_type(seq);

  if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8)
    return make_int(get_seq_elts_ptr_uint8(seq)[idx]);

  if (type == TYPE_NE_SEQ_UINT8_INLINE)
    return make_int(inline_uint8_at(seq.core_data.int_, idx));

  return get_seq_elts_ptr(seq)[idx];
}

double get_float_at(OBJ seq, int64 idx) {
  return get_float(at(seq, idx));
}

int64 get_int_at(OBJ seq, int64 idx) {
  //## TODO: IMPLEMENT THIS FOR REAL!!
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

  if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    uint64 data = seq.core_data.int_;
    for (int i=0 ; i < len ; i++) {
      buffer[i] = make_int(data & 0xFF);
      data >>= 8;
    }
  }
  else {
    uint8 *elts = get_seq_elts_ptr_uint8(seq);

    for (int i=0 ; i < len ; i++)
      buffer[i] = make_int(elts[i]);
  }

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
  else if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    uint64 data = seq.core_data.int_;
    for (int i=0 ; i < len ; i++) {
      buffer[i] = data & 0xFF;
      data >>= 8;
    }
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
  else if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    uint64 data = seq.core_data.int_ >> (8 * idx_first);
    return make_seq_uint8_inline(data, len);
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
        assert(get_int_at(make_seq_uint8_inline(value, 1), 0) == value);
        assert(get_seq_length(make_seq_uint8_inline(value, 1)) == 1);

        return make_seq_uint8_inline(value, 1);
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

    case TYPE_NE_SEQ_UINT8_INLINE: {
      if (is_int(obj)) {
        int64 value = get_int(obj);
        if (value >= 0 & value < 255) {
          if (len < 8) {
            return make_seq_uint8_inline(inline_uint8_array_set_at(seq.core_data.int_, len, value), len + 1);
          }
          else {
            assert(len == 8);
            uint8 value_uint8 = value;
            //## THIS COULD BE IMPROVED, ALTHOUGH IT MAY NOT MATTER IN PRACTICE
            uint8 buffer[8];
            copy_uint8_array(buffer, seq.core_data.int_);
            return concat_uint8(buffer, len, &value_uint8, 1);
          }
        }
      }

      //## THIS ONE TOO COULD BE IMPROVED, ALTHOUGH IN PRACTICE IT MAY MATTER EVEN LESS
      uint8 buffer[8];
      copy_uint8_array(buffer, seq.core_data.int_);
      return concat_uint8_obj(buffer, len, &obj, 1);
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

    if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
      uint8 buffer[8];
      copy_uint8_array(buffer, right.core_data.int_);
      return in_place_concat_uint8(ptrl, lenl, buffer, lenr);
    }

    return concat_uint8_obj(ptrl->buffer.uint8_, lenl, get_seq_elts_ptr(right), lenr);
  }

  if (typel == TYPE_NE_SLICE_UINT8) {
    uint8 *eltsl = get_seq_elts_ptr_uint8(left);

    if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
      return concat_uint8(eltsl, lenl, get_seq_elts_ptr_uint8(right), lenr);

    if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
      uint8 buffer[8];
      copy_uint8_array(buffer, right.core_data.int_);
      return concat_uint8(eltsl, lenl, buffer, lenr);
    }

    return concat_uint8_obj(eltsl, lenl, get_seq_elts_ptr(right), lenr);
  }

  if (typel == TYPE_NE_SEQ_UINT8_INLINE) {
    if (len <= 8) {
      if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8) {
        //## IS THIS CASE EVEN EVER SUPPOSED TO HAPPEN?
        uint8 *eltsr = get_seq_elts_ptr_uint8(right);
        uint64 packed_array = left.core_data.int_;
        for (int i=0 ; i < lenr ; i++)
          packed_array = inline_uint8_array_set_at(packed_array, i + lenl, eltsr[i]);
        //## A CHECK WOULD BE NICE HERE
        return make_seq_uint8_inline(packed_array, len);
      }

      if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
        // uint64 data = left.core_data.int_ | (right.core_data.int_ << (8 * lenl));
        // return make_seq_uint8_inline(data, len);
        //## A CHECK WOULD BE NICE HERE
        //## ALSO, THIS IMPLEMENTATION IS REALLY UGLY
        uint64 packed_array = left.core_data.int_;
        for (int i=0 ; i < lenr ; i++)
          packed_array = inline_uint8_array_set_at(packed_array, i + lenl, inline_uint8_at(right.core_data.int_, i));
        return make_seq_uint8_inline(packed_array, len);
      }
    }
    else {
      uint8 bufferl[8];
      copy_uint8_array(bufferl, left.core_data.int_);

      if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
        return concat_uint8(bufferl, lenl, get_seq_elts_ptr_uint8(right), lenr);

      if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
        uint8 bufferr[8];
        copy_uint8_array(bufferr, right.core_data.int_);
        return concat_uint8(bufferl, lenl, bufferr, lenr);
      }
    }

    uint8 bufferl[8];
    copy_uint8_array(bufferl, left.core_data.int_);
    return concat_uint8_obj(bufferl, lenl, get_seq_elts_ptr(right), lenr);
  }

  if (typel == TYPE_NE_SLICE) {
    OBJ *eltsl = get_seq_elts_ptr(left);

    if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
      return concat_obj_uint8(eltsl, lenl, get_seq_elts_ptr_uint8(right), lenr);

    if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
      uint8 bufferr[8];
      copy_uint8_array(bufferr, right.core_data.int_);
      return concat_obj_uint8(eltsl, lenl, bufferr, lenr);
    }

    return concat_obj(eltsl, lenl, get_seq_elts_ptr(right), lenr);
  }

  assert(typel == TYPE_NE_SEQ);

  SEQ_OBJ *ptrl = get_seq_ptr(left);

  if (typer == TYPE_NE_SEQ_UINT8 | typer == TYPE_NE_SLICE_UINT8)
    return in_place_concat_obj_uint8(ptrl, lenl, get_seq_elts_ptr_uint8(right), lenr);

  if (typer == TYPE_NE_SEQ_UINT8_INLINE) {
    uint8 bufferr[8];
    copy_uint8_array(bufferr, right.core_data.int_);
    return in_place_concat_obj_uint8(ptrl, lenl, bufferr, lenr);
  }

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
  else if (type == TYPE_NE_SEQ_UINT8_INLINE) {
    uint8 buffer[8];
    copy_uint8_array(buffer, seq.core_data.int_);
    for (int i=0 ; i < len / 2 ; i++) {
      uint32 j = len - i - 1;
      uint8 tmp = buffer[i];
      buffer[i] = buffer[j];
      buffer[j] = tmp;
    }
    return make_seq_uint8_inline(pack_uint8_array(buffer, len), len);
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

OBJ build_seq(OBJ *elts, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  for (int i=0 ; i < length ; i++) {
    OBJ elt = elts[i];
    if (!is_int(elt))
      return make_slice(elts, length);
    int64 value = get_int(elt);
    if (value < 0 | value > 255)
      return make_slice(elts, length);
  }

  if (length <= 8) {
    int64 raw_data = 0;
    for (int i=0 ; i < length ; i++)
      raw_data |= get_int(elts[i]) << (8 * i);
    return make_seq_uint8_inline(raw_data, length);
  }
  else {
    uint8 *uint8_elts = (uint8 *) elts;
    for (int i=0 ; i < length ; i++)
      uint8_elts[i] = get_int(elts[i]);
    return make_slice_uint8(uint8_elts, length);
  }
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

  if (min == 0 & max <= 255) {
    if (size <= 8) {
      int64 raw_data = 0;
      for (int i=0 ; i < size ; i++)
        raw_data |= array[i] << (8 * i);
      return make_seq_uint8_inline(raw_data, size);
    }
    else {
      uint8 *uint8_array = (uint8 *) array;
      for (uint32 i=0 ; i < size ; i++)
        uint8_array[i] = array[i];
      return make_slice_uint8(uint8_array, size);
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

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq_int16(int16* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);

  for (uint32 i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_seq_int8(int8* array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  for (int i=0 ; i < size ; i++)
    if (array[i] < 0) {
      SEQ_OBJ *seq = new_obj_seq(size);

      for (uint32 i=0 ; i < size ; i++)
        seq->buffer.obj[i] = make_int(array[i]);

      return make_seq(seq, size);
    }

  //## THE 'BUG' THAT CAUSED THIS SHOULD BE FIXED NOW. CHECK
  return build_seq_uint8((uint8 *) array, size);
}

OBJ build_seq_uint8(uint8 *array, int32 size) {
  if (size == 0)
    return make_empty_seq();

  if (size <= 8) {
    int64 raw_data = 0;
    for (int i=0 ; i < size ; i++)
      raw_data |= ((uint64) array[i]) << (8 * i);
    return make_seq_uint8_inline(raw_data, size);
  }

  return make_slice_uint8(array, size);
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_const_seq_uint8(const uint8 *array, uint32 size) {
  assert(size != 0);
  // assert((((uint64) array) & 7) == 0); //## THIS ACTUALLY HAPPENS, AND IT'S NOT AN EXCEPTION

  if (size <= 8)
    //## ADD A CHECK HERE
    return make_seq_uint8_inline(* (uint64 *) array, size);
  else
    return make_slice_uint8((uint8 *) array, size);
}

OBJ build_const_seq_int8(const int8 *array, uint32 size) {
  assert(size != 0);

  SEQ_OBJ *seq = new_obj_seq(size);

  for (int i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
}

OBJ build_const_seq_int16(const int16 *array, uint32 size) {
  assert(size != 0);

  SEQ_OBJ *seq = new_obj_seq(size);

  for (int i=0 ; i < size ; i++)
    seq->buffer.obj[i] = make_int(array[i]);

  return make_seq(seq, size);
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
