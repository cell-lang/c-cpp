#include "lib.h"


uint32 surr_set_size(SURR_SET *surr_set) {
  return surr_set->count;
}

void surr_set_init(SURR_SET *surr_set) {
  surr_set->bitmap = new_uint64_array(256);
  surr_set->capacity = 256;
  surr_set->count = 0;
}

void surr_set_clear(SURR_SET *surr_set) {
  memset(surr_set->bitmap, 0, surr_set->capacity * sizeof(uint64));
  surr_set->count = 0;
}

bool surr_set_try_insert(SURR_SET *surr_set, uint32 surr) {
  uint32 word_idx = surr / 64;
  uint32 bit_idx = surr % 64;
  uint32 capacity = surr_set->capacity;
  uint64 *bitmap = surr_set->bitmap;
  if (word_idx >= capacity) {
    uint32 new_capacity = 2 * capacity;
    while (word_idx >= new_capacity)
      new_capacity *= 2;
    uint64 *new_bitmap = new_uint64_array(new_capacity);
    memcpy(new_bitmap, bitmap, capacity * sizeof(uint64));
    memset(new_bitmap + capacity, 0, (new_capacity - capacity) * sizeof(uint64));
    surr_set->capacity = new_capacity;
    surr_set->bitmap = new_bitmap;
    capacity = new_capacity;
    bitmap = new_bitmap;
  }
  uint64 word = bitmap[word_idx];
  if (word >> bit_idx != 0)
    return false;
  word |= 1ULL << bit_idx;
  bitmap[word_idx] = word;
  surr_set->count++;
  return true;
}
