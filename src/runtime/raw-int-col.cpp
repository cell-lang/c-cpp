#include "lib.h"


void raw_int_col_init(UNARY_TABLE *master_table, RAW_INT_COL *column, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = master_table->capacity;
  column->array = alloc_state_mem_int64_array(mem_pool, capacity);
#ifndef NDEBUG
  for (uint32 i=0 ; i < capacity ; i++)
    column->array[i] = 0;
  column->capacity = capacity;
#endif
}

void raw_int_col_resize(RAW_INT_COL *column, uint32 capacity, uint32 new_capacity, STATE_MEM_POOL *mem_pool) {
  assert(capacity == column->capacity);

  if (new_capacity > capacity) {
    column->array = extend_state_mem_int64_array(mem_pool, column->array, capacity, new_capacity);
#ifndef NDEBUG
    for (uint32 i=capacity ; i < new_capacity ; i++)
      column->array[i] = 0;
#endif
  }
  else {
    int64 *array = column->array;
    int64 *new_array = alloc_state_mem_int64_array(mem_pool, new_capacity);
    memcpy(new_array, array, new_capacity * sizeof(int64));
    column->array = new_array;
    release_state_mem_int64_array(mem_pool, array, capacity);
  }

#ifndef NDEBUG
  column->capacity = new_capacity;
#endif
}

////////////////////////////////////////////////////////////////////////////////

int64 raw_int_col_lookup(UNARY_TABLE *master_table, RAW_INT_COL *column, uint32 idx) {
  assert(master_table->capacity == column->capacity);

  if (unary_table_contains(master_table, idx)) {
    return column->array[idx];
  }

  soft_fail(NULL); //## ADD MESSAGE
}

int64 raw_int_col_lookup_unchecked(UNARY_TABLE *master_table, RAW_INT_COL *column, uint32 idx) {
  assert(master_table->capacity == column->capacity);
  assert(unary_table_contains(master_table, idx));

  return column->array[idx];
}

////////////////////////////////////////////////////////////////////////////////

void raw_int_col_insert(RAW_INT_COL *column, uint32 idx, int64 value, STATE_MEM_POOL *mem_pool) {
  assert(idx < column->capacity);

  column->array[idx] = value;
}

void raw_int_col_update(UNARY_TABLE *master_table, RAW_INT_COL *column, uint32 idx, int64 value, STATE_MEM_POOL *mem_pool) {
  assert(master_table->capacity == column->capacity);
  assert(idx < column->capacity && unary_table_contains(master_table, idx));

  column->array[idx] = value;
}

////////////////////////////////////////////////////////////////////////////////

void raw_int_col_copy_to(UNARY_TABLE *master_table, RAW_INT_COL *column, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  assert(master_table->capacity == column->capacity);

  int64 *array = column->array;

  uint32 left = master_table->count;
  uint64 *bitmap = master_table->bitmap;
  for (uint32 word_idx=0 ; left > 0 ; word_idx++) {
    uint64 word = bitmap[word_idx];
    for (uint32 bit_idx=0 ; word != 0 ; bit_idx++) {
      if (word & 1 != 0) {
        left--;
        uint32 key_surr = 64 * word_idx + bit_idx;

        OBJ key = surr_to_obj(store, key_surr);
        OBJ value = make_int(array[key_surr]);
        append(*strm_1, key);
        append(*strm_2, value);


      }
      word >>= 1;
    }
  }
}

void raw_int_col_write(WRITE_FILE_STATE *write_state, UNARY_TABLE *master_table, RAW_INT_COL *column, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip) {
  assert(master_table->capacity == column->capacity);

  int64 *array = column->array;

  uint32 left = master_table->count;
  uint64 *bitmap = master_table->bitmap;
  for (uint32 word_idx=0 ; left > 0 ; word_idx++) {
    uint64 word = bitmap[word_idx];
    for (uint32 bit_idx=0 ; word != 0 ; bit_idx++) {
      if (word & 1 != 0) {
        left--;
        uint32 key_surr = 64 * word_idx + bit_idx;

        OBJ key = surr_to_obj(store, key_surr);
        OBJ value = make_int(column->array[key_surr]); //## NO NEED TO CONVERT THIS TO OBJ
        write_str(write_state, "\n    ");
        write_obj(write_state, flip ? value : key);
        write_str(write_state, " -> ");
        write_obj(write_state, flip ? key : value);
        if (left > 0)
          write_str(write_state, ",");

      }
      word >>= 1;
    }
  }

}
