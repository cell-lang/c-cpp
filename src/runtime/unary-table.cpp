#include "lib.h"


const uint32 UNARY_TABLE_INIT_SIZE = 4;


void unary_table_init(UNARY_TABLE *table, STATE_MEM_POOL *mem_pool) {
  table->bitmap = alloc_state_mem_zeroed_uint64_array(mem_pool, UNARY_TABLE_INIT_SIZE);
  table->capacity = 64 * UNARY_TABLE_INIT_SIZE;
  table->count = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_contains(UNARY_TABLE *table, uint32 value) {
  if (value >= table->capacity)
    return false;
  uint32 word_idx = value / 64;
  uint32 bit_idx = value % 64;
  assert(64 * word_idx + bit_idx == value);
  return ((table->bitmap[word_idx] >> bit_idx) & 1) != 0;
}

uint64 unary_table_size(UNARY_TABLE *table) {
  return table->count;
}

uint32 unary_table_count(UNARY_TABLE *table, uint32 value) {
  return unary_table_contains(table, value) ? 1 : 0;
}

uint32 unary_table_capacity(UNARY_TABLE *table) {
  return table->capacity;
}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_insert(UNARY_TABLE *table, uint32 value, STATE_MEM_POOL *mem_pool) {
  uint64 *bitmap = table->bitmap;
  uint32 capacity = table->capacity;

  if (value >= capacity) {
    uint32 new_capacity = 2 * capacity;
    while (value >= new_capacity)
      new_capacity *= 2;
    bitmap = extend_state_mem_zeroed_uint64_array(mem_pool, bitmap, capacity / 64, new_capacity / 64);
    table->bitmap = bitmap;
    table->capacity = new_capacity;
  }

  uint32 word_idx = value / 64;
  uint32 bit_idx = value % 64;
  assert(64 * word_idx + bit_idx == value);

  uint64 word = bitmap[word_idx];
  uint64 bit = 1ULL << bit_idx;

  if ((word & bit) == 0) {
    bitmap[word_idx] = word | bit;
    table->count++;
    return true;
  }
  else
    return false;
}

bool unary_table_delete(UNARY_TABLE *table, uint32 value) {
  uint32 capacity = table->capacity;
  if (value < capacity) {
    uint32 word_idx = value / 64;
    uint32 bit_idx = value % 64;
    assert(64 * word_idx + bit_idx == value);

    uint64 *bitmap = table->bitmap;
    uint64 word = bitmap[word_idx];
    uint64 bit = 1ULL << bit_idx;

    if ((word & bit) != 0) {
      bitmap[word_idx] = word & ~bit;
      table->count--;
      return true;
    }
  }
  return false;
}

void unary_table_clear(UNARY_TABLE *table) {
  if (table->count > 0) {
    table->count = 0;
    memset(table->bitmap, 0, table->capacity / 8);
  }
}

////////////////////////////////////////////////////////////////////////////////

void unary_table_copy_to(UNARY_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *stream) {
  uint32 left = table->count;
  uint64 *bitmap = table->bitmap;
  for (uint32 word_idx=0 ; left > 0 ; word_idx++) {
    uint64 word = bitmap[word_idx];
    for (uint32 bit_idx=0 ; word != 0 ; bit_idx++) {
      if (word & 1 != 0) {
        left--;
        uint32 surr = 64 * word_idx + bit_idx;

        OBJ obj = surr_to_obj(store, surr);
        append(*stream, obj);

      }
      word >>= 1;
    }
  }
}

void unary_table_write(WRITE_FILE_STATE *write_state, UNARY_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store) {
  uint32 left = table->count;
  uint64 *bitmap = table->bitmap;
  for (uint32 word_idx=0 ; left > 0 ; word_idx++) {
    uint64 word = bitmap[word_idx];
    for (uint32 bit_idx=0 ; word != 0 ; bit_idx++) {
      if (word & 1 != 0) {
        left--;
        uint32 surr = 64 * word_idx + bit_idx;

        OBJ obj = surr_to_obj(store, surr);
        write_str(write_state, "\n    ");
        write_obj(write_state, obj);
        if (left > 0)
          write_str(write_state, ",");

      }
      word >>= 1;
    }
  }
}
