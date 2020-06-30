#include "lib.h"
#include "extern.h"


void init(STREAM &s) {
  s.buffer = s.inline_buffer;
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

OBJ build_seq(OBJ *elems, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  return make_slice(elems, length);
}

OBJ build_seq_copy(OBJ *elems, uint32 length) {
  if (length == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(length);
  memcpy(seq->buffer, elems, length * sizeof(OBJ));
  return make_seq(seq, length);
}

OBJ build_seq(STREAM &s) {
  if (s.count == 0)
    return make_empty_seq();

  //## COULD IT BE OPTIMIZED?

  return build_seq_copy(s.buffer, s.count);
}


OBJ build_set(OBJ *elts, uint32 size) {
  if (size == 0)
    return make_empty_rel();

  if (size == 2) {
    OBJ elt0 = elts[0];
    OBJ elt1 = elts[1];

    int cr = comp_objs(elt0, elt1);

    if (cr == 0) {
      SET_OBJ *set = new_set(1);
      set->buffer[0] = elt0;
      return make_set(set);
    }

    SET_OBJ *set = new_set(2);
    if (cr > 0) { // elts[0] < elts[1]
      set->buffer[0] = elt0;
      set->buffer[1] = elt1;
    }
    else { // elts[0] > elts[1]
      set->buffer[0] = elt1;
      set->buffer[1] = elt0;
    }
    return make_set(set);
  }

  // if (size == 3) {
  //   OBJ elt0 = elts[0];
  //   OBJ elt1 = elts[1];
  //   OBJ elt2 = elts[2];

  //   int cr01 = comp_objs(elt0, elt1);

  //   if (cr01 == 0) {
  //     int cr12 = comp_objs(elt1, elt2);

  //     if (cr12 == 0) {
  //       // elt0 == elt1 == elt2
  //       SET_OBJ *set = new_set(1);
  //       set->buffer[0] = elt0;
  //       return make_set(set);
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
  //     return make_set(set);
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
  //     return make_set(set);
  //   }
  //   else if (cr12 < 0) {
  //     int cr02 = comp_objs(elt0, elt2);

  //     if (cr02 > 0) {
  //       // elt0 < elt2 < elt1
  //       SET_OBJ *set = new_set(3);
  //       set->buffer[0] = elt0;
  //       set->buffer[1] = elt2;
  //       set->buffer[2] = elt1;
  //       return make_set(set);
  //     }
  //     else if (cr02 < 0) {
  //       // elt2 < elt0 < elt1
  //       SET_OBJ *set = new_set(3);
  //       set->buffer[0] = elt2;
  //       set->buffer[1] = elt0;
  //       set->buffer[2] = elt1;
  //       return make_set(set);
  //     }
  //     else {
  //       // elt0 < elt1, elt0 == elt2
  //       SET_OBJ *set = new_set(2);
  //       set->buffer[0] = elt0;
  //       set->buffer[1] = elt1;
  //       return make_set(set);
  //     }
  //   }
  //   else {
  //     // elt0 < elt1, elts[1] == elt2
  //     SET_OBJ *set = new_set(2);
  //     set->buffer[0] = elt0;
  //     set->buffer[1] = elt1;
  //     return make_set(set);
  //   }
  // }

  if (size > 1)
    size = sort_unique(elts, size);

  SET_OBJ *set = new_set(size);
  OBJ *es = set->buffer;
  for (uint32 i=0 ; i < size ; i++)
    es[i] = elts[i];

  return make_set(set);
}

OBJ build_set(STREAM &s) {
  // assert((s.count == 0 && s.capacity == 0 && s.buffer == NULL) || (s.count > 0 && s.capacity > 0 && s.buffer != NULL));

  uint32 count = s.count;
  if (count == 0)
    return make_empty_rel();

  return build_set(s.buffer, count);
}

OBJ int_to_float(OBJ obj) {
  return make_float(get_int(obj));
}

OBJ blank_array(int64 size) {
  if (size > 0xFFFFFFFF)
    impl_fail("Maximum permitted array size exceeded");

  if (size <= 0) //## I DON'T LIKE THIS
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(size);
  OBJ *buffer = seq->buffer;
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

  OBJ *ptr = get_seq_buffer_ptr(seq);
  return make_slice(ptr + idx_first, len);
}

OBJ extend_sequence(OBJ seq, OBJ *new_elems, uint32 count) {
  assert(!is_empty_seq(seq));
  assert(((uint64) get_seq_length(seq) + count <= 0xFFFFFFFF));

  uint32 length = get_seq_length(seq);
  uint32 new_length = length + count;

  if (get_physical_type(seq) == TYPE_NE_SEQ) {
    SEQ_OBJ *seq_ptr = get_seq_ptr(seq);

    uint32 capacity = seq_ptr->capacity;
    uint32 used = seq_ptr->used;

    bool ends_at_last_elem = length == used;
    bool has_needed_spare_capacity = used + count <= capacity;
    bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

    if (can_be_extended) {
      memcpy(seq_ptr->buffer + used, new_elems, sizeof(OBJ) * count);
      seq_ptr->used += count;
      return make_seq(seq_ptr, new_length);
    }
  }

  OBJ *buffer = get_seq_buffer_ptr(seq);

  SEQ_OBJ *new_seq_ptr = new_seq(new_length, next_size(length, new_length));
  OBJ *new_buffer = new_seq_ptr->buffer;

  memcpy(new_buffer, buffer, sizeof(OBJ) * length);
  memcpy(new_buffer + length, new_elems, sizeof(OBJ) * count);

  return make_seq(new_seq_ptr, new_length);
}

OBJ append_to_seq(OBJ seq, OBJ obj) { // Obj must be reference counted already
  if (is_empty_seq(seq))
    return build_seq_copy(&obj, 1);

  // Checking that the new sequence doesn't overflow
  if (!(get_seq_length(seq) < 0xFFFFFFFFU))
    impl_fail("Resulting sequence is too large");

  return extend_sequence(seq, &obj, 1);
}

OBJ update_seq_at(OBJ seq, OBJ idx, OBJ value) { // Value must be already reference counted
  uint32 len = get_seq_length(seq);
  int64 int_idx = get_int(idx);

  if (int_idx < 0 | int_idx >= len)
    soft_fail("Invalid sequence index");

  OBJ *src_ptr = get_seq_buffer_ptr(seq);
  SEQ_OBJ *new_seq_ptr = new_seq(len);

  new_seq_ptr->buffer[int_idx] = value;
  for (uint32 i=0 ; i < len ; i++)
    if (i != int_idx)
      new_seq_ptr->buffer[i] = src_ptr[i];

  return make_seq(new_seq_ptr, len);
}

OBJ join_seqs(OBJ left, OBJ right) {
  // No need to check the parameters here

  uint64 right_len = get_seq_length(right);
  if (right_len == 0)
    return left;

  uint64 left_len = get_seq_length(left);
  if (left_len == 0)
    return right;

  if (left_len + right_len > 0xFFFFFFFF)
    impl_fail("_cat_(): Resulting sequence is too large");

  return extend_sequence(left, get_seq_buffer_ptr(right), right_len);
}

OBJ rev_seq(OBJ seq) {
  // No need to check the parameters here

  uint32 len = get_seq_length(seq);
  if (len <= 1)
    return seq;

  OBJ *elems = get_seq_buffer_ptr(seq);

  SEQ_OBJ *rs = new_seq(len);
  OBJ *rev_elems = rs->buffer;
  for (uint32 i=0 ; i < len ; i++)
    rev_elems[len-i-1] = elems[i];

  return make_seq(rs, len);
}

void set_at(OBJ seq, uint32 idx, OBJ value) { // Value must be already reference counted
  // This is not called directly by the user, so asserts should be sufficient
  assert(idx < get_seq_length(seq));

  OBJ *target = get_seq_buffer_ptr(seq) + idx;
  *target = value;
}

OBJ internal_sort(OBJ set) {
  if (is_empty_rel(set))
    return make_empty_seq();

  SET_OBJ *s = get_set_ptr(set);
  uint32 size = s->size;
  OBJ *src = s->buffer;

  SEQ_OBJ *seq = new_seq(size);
  OBJ *dest = seq->buffer;
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
  OBJ str_obj = str_to_obj(raw_str);
  return str_obj;
}

void get_set_iter(SET_ITER &it, OBJ set) {
  it.idx = 0;
  if (!is_empty_rel(set)) {
    SET_OBJ *ptr = get_set_ptr(set);
    it.buffer = ptr->buffer;
    it.size = ptr->size;
  }
  else {
    it.buffer = 0;  //## NOT STRICTLY NECESSARY
    it.size = 0;
  }
}

void get_seq_iter(SEQ_ITER &it, OBJ seq) {
  it.idx = 0;
  if (!is_empty_seq(seq)) {
    it.buffer = get_seq_buffer_ptr(seq);
    it.len = get_seq_length(seq);
  }
  else {
    it.buffer = 0; //## NOT STRICTLY NECESSARY
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

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint16_seq(const uint16* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_uint32_seq(const uint32* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int8_seq(const int8* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int16_seq(const int16* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int32_seq(const int32* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}

OBJ build_const_int64_seq(const int64* buffer, uint32 len) {
  if (len == 0)
    return make_empty_seq();

  SEQ_OBJ *seq = new_seq(len);

  for (int i=0 ; i < len ; i++)
    seq->buffer[i] = make_int(buffer[i]);

  return make_seq(seq, len);
}
