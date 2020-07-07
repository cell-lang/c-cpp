#include "lib.h"
#include "extern.h"


void init(STREAM &s) {
  s.buffer = s.internal_buffer;
  s.capacity = 32;
  s.count = 0;
}

void append(STREAM &s, OBJ obj) { // obj must be already reference-counted
  assert(s.count <= s.capacity);

  uint32 count = s.count;
  uint32 capacity = s.capacity;
  OBJ *buffer = s.buffer;

  if (count == capacity) {
    uint32 new_capacity = capacity == 0 ? 32 : 2 * capacity;
    OBJ *new_buffer = new_obj_array(new_capacity);
    for (uint32 i=0 ; i < count ; i++)
      new_buffer[i] = buffer[i];
    s.buffer = new_buffer;
    s.capacity = new_capacity;
  }

  s.buffer[count] = obj;
  s.count++;
}

__attribute__ ((noinline)) OBJ build_seq_elts8(OBJ *elts, uint32 length) {
  // // uint8 *uint8_elts = (uint8 *) elts;
  // uint8 buffer[16 * 1024];
  // uint8 *uint8_elts = length <= 16 * 1024 ? buffer : new_uint8_array(length);
  // for (int i=0 ; i < length ; i++)
  //   uint8_elts[i] = get_int(elts[i]);
  // // return make_slice_uint8(uint8_elts, length);
  return make_slice(elts, length);
}

OBJ build_seq(OBJ *elts, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  return make_slice(elts, length);

  // for (int i=0 ; i < length ; i++) {
  //   OBJ elt = elts[i];
  //   if (!is_int(elt))
  //     return make_slice(elts, length);
  //   int64 value = get_int(elt);
  //   if (value < 0 | value > 255)
  //     return make_slice(elts, length);
  // }

  // return build_seq_elts8(elts, length);
}

OBJ build_seq_copy(OBJ *elems, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(length);
  memcpy(seq->buffer.objs, elems, length * sizeof(OBJ));
  return make_seq(seq, length);
}

OBJ build_seq(STREAM &s) {
  if (s.count == 0)
    return make_empty_seq();

  //## COULD IT BE OPTIMIZED?

  return build_seq_copy(s.buffer, s.count);
}

inline OBJ make_raw_set(OBJ *elts, uint32 size) {
  return make_set((SET_OBJ *) elts, size);
}

OBJ build_set(OBJ *elts, uint32 size) {
  if (size == 0)
    return make_empty_rel();

  if (size == 2) {
    OBJ elt0 = elts[0];
    OBJ elt1 = elts[1];

    int cr = comp_objs(elt0, elt1);

    if (cr == 0)
      return make_raw_set(elts, 1);

    if (cr < 0) {
      // elts[0] > elts[1], swapping
      elts[0] = elt1;
      elts[1] = elt0;
    }

    return make_raw_set(elts, 2);
  }

  if (size > 1)
    size = sort_unique(elts, size);

  return make_raw_set(elts, size);
}

OBJ build_set(STREAM &s) {
  // assert((s.count == 0 && s.capacity == 0 && s.buffer == NULL) || (s.count > 0 && s.capacity > 0 && s.buffer != NULL));

  uint32 size = s.count;
  if (size == 0)
    return make_empty_rel();

  OBJ *buffer = s.buffer;

  if (buffer != s.internal_buffer)
    return build_set(buffer, size);

  if (size == 2) {
    OBJ elt0 = buffer[0];
    OBJ elt1 = buffer[1];

    int cr = comp_objs(elt0, elt1);

    if (cr == 0) {
      SET_OBJ *set = new_set(1);
      set->buffer[0] = elt0;
      return make_set(set, 1);
    }

    SET_OBJ *set = new_set(2);
    if (cr > 0) { // elt0 < elt1
      set->buffer[0] = elt0;
      set->buffer[1] = elt1;
    }
    else { // elt0 > elt1
      set->buffer[0] = elt1;
      set->buffer[1] = elt0;
    }
    return make_set(set, 2);
  }

  // ## Showed no improvement

  // if (size == 3) {
  //   OBJ elt0 = buffer[0];
  //   OBJ elt1 = buffer[1];
  //   OBJ elt2 = buffer[2];

  //   int cr01 = comp_objs(elt0, elt1);

  //   if (cr01 == 0) {
  //     int cr12 = comp_objs(elt1, elt2);

  //     if (cr12 == 0) {
  //       // elt0 == elt1 == elt2
  //       SET_OBJ *set = new_set(1);
  //       set->buffer[0] = elt0;
  //       return make_set(set, 1);
  //     }

  //     SET_OBJ *set = new_set(2);
  //     if (cr12 > 0) {
  //       // elt0 == elt1, elt1 < elt2
  //       set->buffer[0] = elt1;
  //       set->buffer[1] = elt2;
  //     }
  //     else {
  //       // elt0 == elt1, elt1 > elt2
  //       set->buffer[0] = elt2;
  //       set->buffer[1] = elt1;
  //     }
  //     return make_set(set, 2);
  //   }

  //   if (cr01 < 0) {
  //     OBJ tmp = elt0;
  //     elt0 = elt1;
  //     elt1 = tmp;
  //   }

  //   // elt0 < elt1
  //   int cr12 = comp_objs(elt1, elt2);

  //   if (cr12 > 0) {
  //     // elt0 < elt1 < elt2
  //     SET_OBJ *set = new_set(3);
  //     set->buffer[0] = elt0;
  //     set->buffer[1] = elt1;
  //     set->buffer[2] = elt2;
  //     return make_set(set, 3);
  //   }
  //   else if (cr12 < 0) {
  //     int cr02 = comp_objs(elt0, elt2);

  //     if (cr02 > 0) {
  //       // elt0 < elt2 < elt1
  //       SET_OBJ *set = new_set(3);
  //       set->buffer[0] = elt0;
  //       set->buffer[1] = elt2;
  //       set->buffer[2] = elt1;
  //       return make_set(set, 3);
  //     }
  //     else if (cr02 < 0) {
  //       // elt2 < elt0 < elt1
  //       SET_OBJ *set = new_set(3);
  //       set->buffer[0] = elt2;
  //       set->buffer[1] = elt0;
  //       set->buffer[2] = elt1;
  //       return make_set(set, 3);
  //     }
  //     else {
  //       // elt0 < elt1, elt0 == elt2
  //       SET_OBJ *set = new_set(2);
  //       set->buffer[0] = elt0;
  //       set->buffer[1] = elt1;
  //       return make_set(set, 2);
  //     }
  //   }
  //   else {
  //     // elt0 < elt1, elts[1] == elt2
  //     SET_OBJ *set = new_set(2);
  //     set->buffer[0] = elt0;
  //     set->buffer[1] = elt1;
  //     return make_set(set, 2);
  //   }
  // }

  if (size > 1)
    size = sort_unique(buffer, size);

  SET_OBJ *ptr = new_set(size);
  memcpy(ptr->buffer, buffer, size * sizeof(OBJ));

  return make_set(ptr, size);
}

OBJ int_to_float(OBJ obj) {
  return make_float(get_int(obj));
}

OBJ blank_array(int64 size) {
  if (size > 0xFFFFFFFF)
    impl_fail("Maximum permitted array size exceeded");

  if (size <= 0) //## I DON'T LIKE THIS
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(size);
  OBJ *buffer = seq->buffer.objs;
  OBJ blank_obj = make_blank_obj();

  for (uint32 i=0 ; i < size ; i++)
    buffer[i] = blank_obj;

  return make_seq(seq, size);
}

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
  if (is_empty_seq(seq))
    return build_seq_copy(&obj, 1);

  uint32 len = get_seq_length(seq);

  // Checking that the new sequence doesn't overflow
  if (len == 0xFFFFFFFF)
    impl_fail("Resulting sequence is too large");

  // OBJ_TYPE type = get_physical_type(seq);
  // if (type == TYPE_NE_SEQ)
  //   return in_place_concat_obj(get_seq_ptr(seq), len, &obj, 1);
  // else
  //   return concat_obj(get_seq_elts_ptr(seq), len, &obj, 1);

  switch (get_physical_type(seq)) {
    case TYPE_NE_SEQ: {
      return in_place_concat_obj(get_seq_ptr(seq), len, &obj, 1);
    }

    case TYPE_NE_SLICE: {
      return concat_obj(get_seq_elts_ptr(seq), len, &obj, 1);
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

OBJ update_seq_at(OBJ seq, OBJ idx, OBJ value) {
  uint32 len = get_seq_length(seq);
  int64 int_idx = get_int(idx);

  if (int_idx < 0 | int_idx >= len)
    soft_fail("Invalid sequence index");

  OBJ *src_ptr = get_seq_elts_ptr(seq);
  SEQ_OBJ *new_seq_ptr = new_obj_seq(len);

  new_seq_ptr->buffer.objs[int_idx] = value;
  for (uint32 i=0 ; i < len ; i++)
    if (i != int_idx)
      new_seq_ptr->buffer.objs[i] = src_ptr[i];

  return make_seq(new_seq_ptr, len);
}

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

    return concat_uint8_obj(ptrl->buffer.uint8s, lenl, get_seq_elts_ptr(right), lenr);
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
    OBJ *rev_elems = rs->buffer.objs;
    for (uint32 i=0 ; i < len ; i++)
      rev_elems[len-i-1] = elems[i];

    return make_seq(rs, len);
  }
}

void set_at(OBJ seq, uint32 idx, OBJ value) { // Value must be already reference counted
  // This is not called directly by the user, so asserts should be sufficient
  assert(idx < get_seq_length(seq));

  OBJ *target = get_seq_elts_ptr(seq) + idx;
  *target = value;
}

OBJ internal_sort(OBJ set) {
  if (is_empty_rel(set))
    return make_empty_seq();

  uint32 size = get_set_size(set);
  OBJ *src = get_set_elts_ptr(set);

  if (is_int(src[0]) & is_int(src[size-1])) {
    int64 min = 0;
    int64 max = 0;
    for (int i=0 ; i < size ; i++) {
      int64 elt = get_int(src[i]);
      if (elt < min)
        min = elt;
      if (elt > max)
        max = elt;
    }

    if (min == 0 & max <= 255) {
      uint32 capacity = ((((uint64) size) + 7) / 8) * 8;
      assert(capacity >= size & capacity < size + 8);
      SEQ_OBJ *seq_ptr = new_uint8_seq(size, capacity);
      uint8 *elts = seq_ptr->buffer.uint8s;
      for (int i=0 ; i < size ; i++)
        elts[i] = get_int(src[i]);
      return make_seq_uint8(seq_ptr, size);
    }
  }

  SEQ_OBJ *seq = new_obj_seq(size);
  OBJ *dest = seq->buffer.objs;
  for (uint32 i=0 ; i < size ; i++)
    dest[i] = src[i];

  return make_seq(seq, size);
}

OBJ parse_value(OBJ str_obj) {
  char *raw_str = obj_to_str(str_obj);
  uint32 len = strlen(raw_str);
  OBJ obj;
  uint32 error_offset;
  bool ok = parse(raw_str, len, &obj, &error_offset);
  if (ok)
    return make_tag_obj(symb_id_success, obj);
  else
    return make_tag_obj(symb_id_failure, make_int(error_offset));
}

char *print_value_alloc(void *ptr, uint32 size) {
  uint32 *size_ptr = (uint32 *) ptr;
  assert(*size_ptr == 0);
  *size_ptr = size;
  return new_byte_array(size);
}

OBJ print_value(OBJ obj) {
  uint32 size = 0;
  char *raw_str = printed_obj(obj, print_value_alloc, &size);
  uint32 len = strlen(raw_str);
  //## NOTE: len IS NOT NECESSARILY EQUAL TO size - 1
  for (int i=0 ; i < len ; i++)
    assert(raw_str[i] > 0 & raw_str[i] <= 127);
  return make_tag_obj(symb_id_string, make_slice_uint8((uint8 *) raw_str, len));
  // OBJ str_obj = str_to_obj(raw_str);
  // return str_obj;
}

void get_set_iter(SET_ITER &it, OBJ set) {
  it.idx = 0;
  if (!is_empty_rel(set)) {
    it.buffer = get_set_elts_ptr(set);
    it.size = get_set_size(set);
  }
  else {
    it.buffer = 0;  //## NOT STRICTLY NECESSARY
    it.size = 0;
  }
}

void get_seq_iter(SEQ_ITER &it, OBJ seq) {
  it.idx = 0;
  if (!is_empty_seq(seq)) {
    it.len = get_seq_length(seq);

    OBJ_TYPE type = get_physical_type(seq);
    if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8) {
      it.buffer.uint8s = get_seq_elts_ptr_uint8(seq);
      it.type = ELT_TYPE_UINT8;
    }
    else {
      assert(type == TYPE_NE_SEQ | type == TYPE_NE_SLICE);
      it.buffer.objs = get_seq_elts_ptr(seq);
      it.type = ELT_TYPE_OBJ;
    }
  }
  else {
    // it.buffer = 0; //## NOT STRICTLY NECESSARY
    // it.type = -1; // Invalid value
    it.len = 0;
  }
}

void move_forward(SET_ITER &it) {
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(SEQ_ITER &it) {
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(BIN_REL_ITER &it) {
  assert(!is_out_of_range(it));

  if (it.type == BIN_REL_ITER::BRIT_OPT_REC) {
    for ( ; ; ) {
      if (++it.idx >= it.end)
        break;
      uint16 field = it.iter.opt_rec.fields[it.idx];
      if (opt_repr_has_field(it.iter.opt_rec.ptr, it.iter.opt_rec.repr_id, field))
        break;
    }
  }
  else {
    assert(it.type == BIN_REL_ITER::BRIT_BIN_REL);
    it.idx++;
  }
}

void move_forward(TERN_REL_ITER &it) {
  assert(!is_out_of_range(it));
  it.idx++;
}

void fail() {
#ifndef NDEBUG
  const char *MSG = "\nFail statement reached. Call stack:\n\n";
#else
  const char *MSG = "\nFail statement reached\n";
#endif

  soft_fail(MSG);
}

void runtime_check(OBJ cond) {
  assert(is_bool(cond));

  if (!get_bool(cond)) {
#ifndef NDEBUG
    fputs("\nAssertion failed. Call stack:\n\n", stderr);
#else
    fputs("\nAssertion failed\n", stderr);
#endif
    fflush(stderr);
    print_call_stack();
    *(char *)0 = 0; // Causing a runtime crash, useful for debugging
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_const_uint8_seq(const uint8* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint16_seq(const uint16* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint32_seq(const uint32* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int8_seq(const int8* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int16_seq(const int16* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int32_seq(const int32* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int64_seq(const int64* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_obj_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer.objs[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}
