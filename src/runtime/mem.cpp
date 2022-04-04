#include "lib.h"


//## BUG: ALL THE *_obj_mem_size RETURN A 64-BIT INTEGER, BUT
//## I THINK THEY'RE DOING THEIR CALCULATION IN 32-BIT ARITHMETIC

uint64 seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) + (capacity - 1) * sizeof(OBJ);
}

uint64 uint8_seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) - sizeof(OBJ) + capacity * sizeof(uint8);
}

uint64 int8_seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) - sizeof(OBJ) + capacity * sizeof(int8);
}

uint64 int16_seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) - sizeof(OBJ) + capacity * sizeof(int16);
}

uint64 int32_seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) - sizeof(OBJ) + capacity * sizeof(int32);
}

uint64 int64_seq_obj_mem_size(uint64 capacity) {
  assert(capacity > 0);
  return sizeof(SEQ_OBJ) - sizeof(OBJ) + capacity * sizeof(int64);
}

uint64 set_obj_mem_size(uint64 size) {
  assert(size > 0);
  return sizeof(SET_OBJ) + (size - 1) * sizeof(OBJ);
}

uint64 bin_tree_set_obj_mem_size() {
  return sizeof(TREE_SET_NODE);
}

uint64 bin_tree_map_obj_mem_size() {
  return sizeof(TREE_MAP_NODE);
}

uint64 bin_rel_obj_mem_size(uint64 size) {
  assert(size > 0);
  //## IF THE MAP HAS ONLY ONE ENTRY, THERE WOULD BE NO NEED FOR THE RIGHT-TO-LEFT INDEX
  //## IT COULD PROBABLY ALSO BE ELIMINATED FOR VERY SMALL SIZES
  return sizeof(BIN_REL_OBJ) + (2 * size - 1) * sizeof(OBJ) + size * sizeof(uint32);
}

uint64 tern_rel_obj_mem_size(uint64 size) {
  assert(size > 0);
  return sizeof(TERN_REL_OBJ) + (3 * size - 1) * sizeof(OBJ) + 2 * size * sizeof(uint32);
}

uint64 map_obj_mem_size(uint64 size) {
  assert(size > 0);
  return bin_rel_obj_mem_size(size);
}

uint64 boxed_obj_mem_size() {
  return sizeof(BOXED_OBJ);
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_left_col_array_ptr(BIN_REL_OBJ *rel) {
  return rel->buffer;
}

OBJ *get_right_col_array_ptr(BIN_REL_OBJ *rel, uint32 size) {
  return rel->buffer + size;
}

uint32 *get_right_to_left_indexes(BIN_REL_OBJ *rel, uint32 size) {
  return (uint32 *) (rel->buffer + 2 * size);
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_col_array_ptr(TERN_REL_OBJ *rel, uint32 size, int idx) {
  assert(idx >= 0 & idx <= 2);
  return rel->buffer + idx * size;
}

uint32 *get_rotated_index(TERN_REL_OBJ *rel, uint32 size, int amount) {
  assert(amount == 1 | amount == 2);
  uint32 *base_ptr = (uint32 *) (rel->buffer + 3 * size);
  return base_ptr + (amount-1) * size;
}

////////////////////////////////////////////////////////////////////////////////

uint32 seq_capacity(uint64 byte_size) {
  return (byte_size - sizeof(SEQ_OBJ)) / sizeof(OBJ) + 1;
}

////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ *new_obj_seq(uint32 length) {
  assert(length > 0);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(length));
  seq->capacity = length;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_obj_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_float_seq(uint32 length) {
  assert(length > 0);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(length));
  seq->capacity = length;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_float_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_uint8_seq(uint32 length) {
  return new_uint8_seq(length, (length + 7) & ~7);
}

SEQ_OBJ *new_uint8_seq(uint32 length, uint32 capacity) {
  assert(length > 0);
  assert(capacity >= length);
  assert(capacity % 8 == 0);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(uint8_seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_int8_seq(uint32 length) {
  return new_int8_seq(length, (length + 3) & ~3);
}

SEQ_OBJ *new_int8_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);
  assert(capacity % 8 == 0);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(int8_seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_int16_seq(uint32 length) {
  return new_int16_seq(length, (length + 3) & ~3);
}

SEQ_OBJ *new_int16_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);
  assert(capacity % 4 == 0);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(int16_seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_int32_seq(uint32 length) {
  return new_int32_seq(length, (length + 3) & ~3);
}

SEQ_OBJ *new_int32_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);
  assert(capacity % 2 == 0);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(int32_seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}

SEQ_OBJ *new_int64_seq(uint32 length) {
  return new_int64_seq(length, (length + 3) & ~3);
}

SEQ_OBJ *new_int64_seq(uint32 length, uint32 capacity) {
  assert(length > 0 & capacity >= length);
  assert(length < 32 | 2 * length >= capacity);

  SEQ_OBJ *seq = (SEQ_OBJ *) new_obj(int64_seq_obj_mem_size(capacity));
  seq->capacity = capacity;
  seq->used = length;
  return seq;
}


SET_OBJ *new_set(uint32 size) {
  return (SET_OBJ *) new_obj(set_obj_mem_size(size));
}

TREE_SET_NODE *new_bin_tree_set() {
  return (TREE_SET_NODE *) new_obj(bin_tree_set_obj_mem_size());
}

BIN_REL_OBJ *new_map(uint32 size) {
  assert(size > 0);

  BIN_REL_OBJ *map = (BIN_REL_OBJ *) new_obj(map_obj_mem_size(size));
  uint32 *rev_idxs = get_right_to_left_indexes(map, size);

  // By setting the first two elements of the right-to-left index
  // to the same value, we mark the index as not yet built
  rev_idxs[0] = 0;
  if (size > 1)
    rev_idxs[1] = 0;

  return map;
}

TREE_MAP_NODE *new_bin_tree_map() {
  return (TREE_MAP_NODE *) new_obj(bin_tree_map_obj_mem_size());
}

BIN_REL_OBJ *new_bin_rel(uint32 size) {
  assert(size > 0);
  return (BIN_REL_OBJ *) new_obj(bin_rel_obj_mem_size(size));
}

TERN_REL_OBJ *new_tern_rel(uint32 size) {
  assert(size > 0);
  return (TERN_REL_OBJ *) new_obj(tern_rel_obj_mem_size(size));
}

BOXED_OBJ *new_boxed_obj() {
  BOXED_OBJ *tag_obj = (BOXED_OBJ *) new_obj(boxed_obj_mem_size());
  return tag_obj;
}

////////////////////////////////////////////////////////////////////////////////

static uint64 total_count_new_obj_array     = 0;
static uint64 total_count_new_bool_array    = 0;
static uint64 total_count_new_double_array  = 0;
static uint64 total_count_new_int64_array   = 0;
static uint64 total_count_new_uint64_array  = 0;
static uint64 total_count_new_int32_array   = 0;
static uint64 total_count_new_uint32_array  = 0;
static uint64 total_count_new_int16_array   = 0;
static uint64 total_count_new_uint16_array  = 0;
static uint64 total_count_new_int8_array    = 0;
static uint64 total_count_new_uint8_array   = 0;


void print_temp_mem_alloc_stats() {
  printf("Temporary allocation details:\n");
  printf("  new_obj_array:    %llu\n", total_count_new_obj_array);
  printf("  new_bool_array:   %llu\n", total_count_new_bool_array);
  printf("  new_double_array: %llu\n", total_count_new_double_array);
  printf("  new_int64_array:  %llu\n", total_count_new_int64_array);
  printf("  new_uint64_array: %llu\n", total_count_new_uint64_array);
  printf("  new_int32_array:  %llu\n", total_count_new_int32_array);
  printf("  new_uint32_array: %llu\n", total_count_new_uint32_array);
  printf("  new_int16_array:  %llu\n", total_count_new_int16_array);
  printf("  new_uint16_array: %llu\n", total_count_new_uint16_array);
  printf("  new_int8_array:   %llu\n", total_count_new_int8_array);
  printf("  new_uint8_array:  %llu\n", total_count_new_uint8_array);
}


OBJ *new_obj_array(uint32 size) {
  total_count_new_obj_array += size * sizeof(OBJ);
  return (OBJ *) new_obj(size * sizeof(OBJ));
}

bool *new_bool_array(uint32 size) {
  total_count_new_bool_array += size * sizeof(bool);
  return (bool *) new_obj(size * sizeof(bool));
}

double *new_float_array(uint32 size) {
  total_count_new_double_array += size * sizeof(double);
  return (double *) new_obj(size * sizeof(double));
}

int64 *new_int64_array(uint32 size) {
  total_count_new_int64_array += size * sizeof(int64);
  return (int64 *) new_obj(size * sizeof(int64));
}

uint64 *new_uint64_array(uint32 size) {
  total_count_new_uint64_array += size * sizeof(uint64);
  return (uint64 *) new_obj(size * sizeof(uint64));
}

int32 *new_int32_array(uint32 size) {
  total_count_new_int32_array += size * sizeof(int32);
  return (int32 *) new_obj(size * sizeof(int32));
}

uint32 *new_uint32_array(uint32 size) {
  total_count_new_uint32_array += size * sizeof(uint32);
  return (uint32 *) new_obj(size * sizeof(uint32));
}

int16 *new_int16_array(uint32 size) {
  total_count_new_int16_array += size * sizeof(int16);
  return (int16 *) new_obj(size * sizeof(int16));
}

uint16 *new_uint16_array(uint32 size) {
  total_count_new_uint16_array += size * sizeof(uint16);
  return (uint16 *) new_obj(size * sizeof(uint16));
}

int8 *new_int8_array(uint32 size) {
  total_count_new_int8_array += size * sizeof(int8);
  return (int8 *) new_obj(size * sizeof(int8));
}

uint8 *new_uint8_array(uint32 size) {
  total_count_new_uint8_array += size * sizeof(uint8);
  return (uint8 *) new_obj(size * sizeof(uint8));
}

////////////////////////////////////////////////////////////////////////////////

char *new_byte_array(uint32 size) {
  return (char *) new_obj(size);
}

void *new_void_array(uint32 size) {
  return new_obj(size);
}

////////////////////////////////////////////////////////////////////////////////

OBJ *resize_obj_array(OBJ* array, uint32 size, uint32 new_size) {
  assert(new_size > size);
  OBJ *new_array = new_obj_array(new_size);
  memcpy(new_array, array, size * sizeof(OBJ));
  return new_array;
}

double *resize_float_array(double* array, uint32 size, uint32 new_size) {
  assert(new_size > size);
  double *new_array = new_float_array(new_size);
  memcpy(new_array, array, size * sizeof(double));
  return new_array;
}

uint64 *resize_uint64_array(uint64 *array, uint32 size, uint32 new_size) {
  assert(new_size > size);
  uint64 *new_array = new_uint64_array(new_size);
  memcpy(new_array, array, size * sizeof(uint64));
  return new_array;
}

int64 *resize_int64_array(int64 *array, uint32 size, uint32 new_size) {
  assert(new_size > size);
  int64 *new_array = new_int64_array(new_size);
  memcpy(new_array, array, size * sizeof(int64));
  return new_array;
}

uint32 *resize_uint32_array(uint32 *array, uint32 size, uint32 new_size) {
  assert(new_size > size);
  uint32 *new_array = new_uint32_array(new_size);
  memcpy(new_array, array, size * sizeof(uint32));
  return new_array;
}
