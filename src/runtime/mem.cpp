#include "lib.h"


uint64 set_obj_mem_size(uint64 size) {
  assert(size > 0);
  return sizeof(SET_OBJ) + (size - 1) * sizeof(OBJ);
}

uint64 seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) + (capacity - 1) * sizeof(OBJ);
}

uint64 bin_rel_obj_mem_size(uint64 size) {
  assert(size > 0);
  return sizeof(BIN_REL_OBJ) + (2 * size - 1) * sizeof(OBJ) + size * sizeof(uint32);
}

uint32 tern_rel_obj_mem_size(uint64 size) {
  assert(size > 0);
  return sizeof(TERN_REL_OBJ) + (3 * size - 1) * sizeof(OBJ) + 2 * size * sizeof(uint32);
}

uint64 map_obj_mem_size(uint64 size) {
  assert(size > 0);
  return bin_rel_obj_mem_size(size);
}

uint64 tag_obj_mem_size() {
  return sizeof(TAG_OBJ);
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_left_col_array_ptr(BIN_REL_OBJ *rel) {
  return rel->buffer;
}

OBJ *get_right_col_array_ptr(BIN_REL_OBJ *rel) {
  return rel->buffer + rel->size;
}

uint32 *get_right_to_left_indexes(BIN_REL_OBJ *rel) {
  return (uint32 *) (rel->buffer + 2 * rel->size);
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_col_array_ptr(TERN_REL_OBJ *rel, int idx) {
  assert(idx >= 0 & idx <= 2);
  return rel->buffer + idx * rel->size;
}

uint32 *get_rotated_index(TERN_REL_OBJ *rel, int amount) {
  assert(amount == 1 | amount == 2);
  uint32 size = rel->size;
  uint32 *base_ptr = (uint32 *) (rel->buffer + 3 * size);
  return base_ptr + (amount-1) * size;
}

////////////////////////////////////////////////////////////////////////////////

uint32 seq_capacity(uint64 byte_size) {
  return (byte_size - sizeof(SEQ_OBJ)) / sizeof(OBJ) + 1;
}

////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ *new_seq(uint32 length) {
  assert(length > 0);

  if (length > 0xFFFFFFF)
    impl_fail("Maximum permitted sequence length (2^28-1) exceeded");

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(length));
  seq->capacity = length;
  seq->size = length;
  return seq;
}

SEQ_OBJ *new_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);

  if (capacity > 0xFFFFFFF)
    impl_fail("Maximum permitted sequence length (2^28-1) exceeded");

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->size = length;
  return seq;
}

SET_OBJ *new_set(uint32 size) {
  SET_OBJ *set = (SET_OBJ *) new_obj(set_obj_mem_size(size));
  set->size = size;
  return set;
}

BIN_REL_OBJ *new_map(uint32 size) {
  assert(size > 0);

  BIN_REL_OBJ *map = (BIN_REL_OBJ *) new_obj(map_obj_mem_size(size));
  map->size = size;
  uint32 *rev_idxs = get_right_to_left_indexes(map);
  rev_idxs[0] = INVALID_INDEX;
  return map;
}

BIN_REL_OBJ *new_bin_rel(uint32 size) {
  assert(size > 0);

  BIN_REL_OBJ *rel = (BIN_REL_OBJ *) new_obj(bin_rel_obj_mem_size(size));
  rel->size = size;
  return rel;
}

TERN_REL_OBJ *new_tern_rel(uint32 size) {
  assert(size > 0);

  TERN_REL_OBJ *rel = (TERN_REL_OBJ *) new_obj(tern_rel_obj_mem_size(size));
  rel->size = size;
  return rel;
}

TAG_OBJ *new_tag_obj() {
  TAG_OBJ *tag_obj = (TAG_OBJ *) new_obj(tag_obj_mem_size());
  tag_obj->unused_field = 0;
  return tag_obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *new_obj_array(uint32 size) {
  return (OBJ *) new_obj(size * sizeof(OBJ));
}

OBJ* resize_obj_array(OBJ* buffer, uint32 size, uint32 new_size) {
  OBJ *new_array = new_obj_array(new_size);
  memcpy(new_array, buffer, size * sizeof(OBJ));
  return new_array;
}

bool *new_bool_array(uint32 size) {
  return (bool *) new_obj(size * sizeof(bool));
}

double *new_double_array(uint32 size) {
  return (double *) new_obj(size * sizeof(double));
}

int64 *new_int64_array(uint32 size) {
  return (int64 *) new_obj(size * sizeof(int64));
}

int32 *new_int32_array(uint32 size) {
  return (int32 *) new_obj(size * sizeof(int32));
}

uint32 *new_uint32_array(uint32 size) {
  return (uint32 *) new_obj(size * sizeof(uint32));
}

int16 *new_int16_array(uint32 size) {
  return (int16 *) new_obj(size * sizeof(int16));
}

uint16 *new_uint16_array(uint32 size) {
  return (uint16 *) new_obj(size * sizeof(uint16));
}

int8 *new_int8_array(uint32 size) {
  return (int8 *) new_obj(size * sizeof(int8));
}

uint8 *new_uint8_array(uint32 size) {
  return (uint8 *) new_obj(size * sizeof(uint8));
}

////////////////////////////////////////////////////////////////////////////////

char *new_byte_array(uint32 size) {
  return (char *) new_obj(size);
}

void *new_void_array(uint32 size) {
  return new_obj(size);
}
