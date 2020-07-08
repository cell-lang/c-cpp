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

OBJ in_place_concat_uint8(SEQ_OBJ *seq_ptr, uint32 length, uint8 *new_elts, uint32 count) {
  assert(no_sum32_overflow(length, count));

  uint32 new_length = length + count;
  uint32 used = seq_ptr->used;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = length == used;
  bool has_needed_spare_capacity = used + count <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended) {
    memcpy(seq_ptr->buffer.uint8s + used, new_elts, count * sizeof(uint8));
    seq_ptr->used += count;
    return make_seq_uint8(seq_ptr, new_length);
  }

  uint8 *elts = seq_ptr->buffer.uint8s;

  SEQ_OBJ *new_seq_ptr = new_uint8_seq(new_length, next_capacity(capacity, new_length));
  uint8 *new_seq_elts = new_seq_ptr->buffer.uint8s;

  memcpy(new_seq_elts, elts, length * sizeof(uint8));
  memcpy(new_seq_elts + length, new_elts, count * sizeof(uint8));

  return make_seq_uint8(new_seq_ptr, new_length);
}

OBJ in_place_concat_obj(SEQ_OBJ *seq_ptr, uint32 length, OBJ *new_elts, uint32 count) {
  assert(no_sum32_overflow(length, count));

  uint32 new_length = length + count;
  uint32 used = seq_ptr->used;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = length == used;
  bool has_needed_spare_capacity = used + count <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended) {
    memcpy(seq_ptr->buffer.objs + used, new_elts, count * sizeof(OBJ));
    seq_ptr->used += count;
    return make_seq(seq_ptr, new_length);
  }

  OBJ *elts = seq_ptr->buffer.objs;

  SEQ_OBJ *new_seq_ptr = new_obj_seq(new_length, next_capacity(capacity, new_length));
  OBJ *new_seq_elts = new_seq_ptr->buffer.objs;

  memcpy(new_seq_elts, elts, length * sizeof(OBJ));
  memcpy(new_seq_elts + length, new_elts, count * sizeof(OBJ));

  return make_seq(new_seq_ptr, new_length);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ in_place_concat_obj_uint8(SEQ_OBJ *seq_ptr, uint32 length, uint8 *new_elts, uint32 count) {
  assert(no_sum32_overflow(length, count));

  uint32 new_length = length + count;
  uint32 used = seq_ptr->used;
  uint32 capacity = seq_ptr->capacity;

  bool ends_at_last_elem = length == used;
  bool has_needed_spare_capacity = used + count <= capacity;
  bool can_be_extended = ends_at_last_elem & has_needed_spare_capacity;

  if (can_be_extended) {
    OBJ *dest = seq_ptr->buffer.objs + used;
    for (int i=0 ; i < count ; i++)
      dest[i] = make_int(new_elts[i]);
    seq_ptr->used += count;
    return make_seq(seq_ptr, new_length);
  }

  OBJ *elts = seq_ptr->buffer.objs;

  SEQ_OBJ *new_seq_ptr = new_obj_seq(new_length, next_capacity(capacity, new_length));
  OBJ *new_seq_elts = new_seq_ptr->buffer.objs;

  memcpy(new_seq_elts, elts, length * sizeof(OBJ));
  OBJ *dest = new_seq_elts + length;
  for (int i=0 ; i < count ; i++)
    dest[i] = make_int(new_elts[i]);

  return make_seq(new_seq_ptr, new_length);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ concat_uint8(uint8 *elts1, uint32 len1, uint8 *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_uint8_seq(len, next_capacity(16, len));
  memcpy(new_seq_ptr->buffer.uint8s, elts1, len1 * sizeof(uint8));
  memcpy(new_seq_ptr->buffer.uint8s + len1, elts2, len2 * sizeof(uint8));
  return make_seq_uint8(new_seq_ptr, len1 + len2);
}

OBJ concat_obj(OBJ *elts1, uint32 len1, OBJ *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_obj_seq(len, next_capacity(16, len));
  memcpy(new_seq_ptr->buffer.objs, elts1, len1 * sizeof(OBJ));
  memcpy(new_seq_ptr->buffer.objs + len1, elts2, len2 * sizeof(OBJ));
  return make_seq(new_seq_ptr, len1 + len2);
}

OBJ concat_uint8_obj(uint8 *elts1, uint32 len1, OBJ *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_obj_seq(len, next_capacity(16, len));
  OBJ *dest = new_seq_ptr->buffer.objs;
  for (int i=0 ; i < len1 ; i++)
    dest[i] = make_int(elts1[i]);
  memcpy(new_seq_ptr->buffer.objs + len1, elts2, len2 * sizeof(OBJ));
  return make_seq(new_seq_ptr, len1 + len2);
}

OBJ concat_obj_uint8(OBJ *elts1, uint32 len1, uint8 *elts2, uint32 len2) {
  assert(no_sum32_overflow(len1, len2));

  uint32 len = len1 + len2;
  SEQ_OBJ *new_seq_ptr = new_obj_seq(len, next_capacity(16, len));
  memcpy(new_seq_ptr->buffer.objs, elts1, len1 * sizeof(OBJ));
  OBJ *dest = new_seq_ptr->buffer.objs;
  for (int i=0 ; i < len2 ; i++)
    dest[i + len1] = make_int(elts2[i]);
  return make_seq(new_seq_ptr, len1 + len2);
}
