#include "lib.h"
#include "one-way-bin-table.h"


void double_key_bin_table_init(DOUBLE_KEY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_SIZE = 256;

  table->forward_array = alloc_state_mem_oned_uint32_array(mem_pool, INIT_SIZE);
  table->backward_array = alloc_state_mem_oned_uint32_array(mem_pool, INIT_SIZE);
  table->forward_capacity = INIT_SIZE;
  table->backward_capacity = INIT_SIZE;
  table->count = 0;
  table->mem_pool_offset = ((uint8 *) table) - ((uint8 *) mem_pool);

  assert(double_key_bin_table_mem_pool(table) == mem_pool);
}

uint32 double_key_bin_table_size(DOUBLE_KEY_BIN_TABLE *table) {
  return table->count;
}

bool double_key_bin_table_contains(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return arg1 < table->forward_capacity && table->forward_array[arg1] == arg2;
}

bool double_key_bin_table_contains_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1) {
  return arg1 < table->forward_capacity && table->forward_array[arg1] != 0xFFFFFFFF;
}

bool double_key_bin_table_contains_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2) {
  return arg2 < table->backward_capacity && table->backward_array[arg2] != 0xFFFFFFFF;
}

uint32 double_key_bin_table_count_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1) {
  return double_key_bin_table_contains_1(table, arg1) ? 1 : 0;
}

uint32 double_key_bin_table_count_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2) {
  return double_key_bin_table_contains_2(table, arg2) ? 1 : 0;
}

uint32 double_key_bin_table_restrict_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 *arg2s) {
  if (arg1 < table->forward_capacity) {
    uint32 arg2 = table->forward_array[arg1];
    if (arg2 != 0xFFFFFFFF) {
      arg2s[0] = arg2;
      return 1;
    }
  }
  return 0;
}

uint32 double_key_bin_table_restrict_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2, uint32 *arg1s) {
  if (arg2 < table->backward_capacity) {
    uint32 arg1 = table->backward_array[arg2];
    if (arg1 != 0xFFFFFFFF) {
      arg1s[0] = arg1;
      return 1;
    }
  }
  return 0;
}

UINT32_ARRAY double_key_bin_table_range_restrict_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 first, uint32 *arg2s, uint32 capacity) {
  assert(first == 0 && capacity > 0);
  uint32 count = double_key_bin_table_restrict_1(table, arg1, arg2s);
  UINT32_ARRAY result;
  result.array = arg2s;
  result.size = count;
  return result;
}

UINT32_ARRAY double_key_bin_table_range_restrict_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2, uint32 first, uint32 *arg1s, uint32 capacity) {
  assert(first == 0 && capacity > 0);
  uint32 count = double_key_bin_table_restrict_2(table, arg2, arg1s);
  UINT32_ARRAY result;
  result.array = arg1s;
  result.size = count;
  return result;
}

uint32 double_key_bin_table_lookup_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1) {
  return arg1 < table->forward_capacity ? table->forward_array[arg1] : 0xFFFFFFFF;
}

uint32 double_key_bin_table_lookup_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2) {
  return arg2 < table->backward_capacity ? table->backward_array[arg2] : 0xFFFFFFFF;
}

////////////////////////////////////////////////////////////////////////////////

void double_key_bin_table_insert(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  uint32 forward_capacity = table->forward_capacity;
  uint32 backward_capacity = table->backward_capacity;
  uint32 *forward_array = table->forward_array;
  uint32 *backward_array = table->backward_array;

  if (arg1 >= forward_capacity) {
    //## BAD BAD BAD: JUST WRITE A FUNCTION THAT DOES THIS DOUBLING OF CAPACITY AND USE IT EVERYWHERE
    uint32 new_forward_capacity = 2 * forward_capacity;
    while (arg1 >= new_forward_capacity)
      new_forward_capacity *= 2;
    forward_array = extend_state_mem_oned_uint32_array(mem_pool, forward_array, forward_capacity, new_forward_capacity);
    table->forward_array = forward_array;
    table->forward_capacity = new_forward_capacity;
    // capacity = new_capacity; ## NOT NECESSARY
  }

  if (arg2 >= backward_capacity) {
    //## BAD BAD BAD: JUST WRITE A FUNCTION THAT DOES THIS DOUBLING OF CAPACITY AND USE IT EVERYWHERE
    uint32 new_backward_capacity = 2 * backward_capacity;
    while (arg1 >= new_backward_capacity)
      new_backward_capacity *= 2;
    backward_array = extend_state_mem_oned_uint32_array(mem_pool, backward_array, backward_capacity, new_backward_capacity);
    table->backward_array = backward_array;
    table->backward_capacity = new_backward_capacity;
    // capacity = new_capacity; ## NOT NECESSARY
  }

  uint32 fwd_data = forward_array[arg1];
  uint32 bkw_data = backward_array[arg2];
  if (fwd_data != 0xFFFFFFFF | bkw_data != 0xFFFFFFFF)
    soft_fail(NULL); //## BAD BAD BAD: THIS SHOULD NOT HAPPEN EXCEPT DURING LOADING, SO MAYBE THIS IS NOT THE RIGHT PLACE FOR THIS CHECK?

  forward_array[arg1] = arg2;
  backward_array[arg2] = arg1;
  table->count++;
}

bool double_key_bin_table_delete(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  if (arg1 < table->forward_capacity) {
    uint32 *forward_array = table->forward_array;
    uint32 data = forward_array[arg1];
    assert(data == 0xFFFFFFFF || (table->backward_array[data] == arg1));
    if (data == arg2) {
      forward_array[arg1] = 0xFFFFFFFF;
      table->backward_array[arg2] = 0xFFFFFFFF;
      table->count--;
      return true;
    }
  }
  return false;
}

void double_key_bin_table_delete_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1) {
  if (arg1 < table->forward_capacity) {
    uint32 *forward_array = table->forward_array;
    uint32 arg2 = forward_array[arg1];
    if (arg2 != 0xFFFFFFFF) {
      forward_array[arg1] = 0xFFFFFFFF;
      table->backward_array[arg2] = 0xFFFFFFFF;
      table->count--;
    }
  }
}

void double_key_bin_table_delete_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2) {
  if (arg2 < table->backward_capacity) {
    uint32 *backward_array = table->backward_array;
    uint32 arg1 = backward_array[arg2];
    if (arg1 != 0xFFFFFFFF) {
      table->forward_array[arg1] = 0xFFFFFFFF;
      backward_array[arg2] = 0xFFFFFFFF;
      table->count--;
    }
  }
}

void double_key_bin_table_clear(DOUBLE_KEY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  if (table->count > 0) {
    //## BAD BAD BAD: MAYBE I SHOULD REDUCE THE CAPACITY HERE
    memset(table->forward_array, 0xFF, table->forward_capacity * sizeof(uint32));
    memset(table->backward_array, 0xFF, table->backward_capacity * sizeof(uint32));
    table->count = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool double_key_bin_table_col_1_is_key(DOUBLE_KEY_BIN_TABLE *table) {
  return true;
}

bool double_key_bin_table_col_2_is_key(DOUBLE_KEY_BIN_TABLE *table) {
  return true;
}

////////////////////////////////////////////////////////////////////////////////

//## ALMOST IDENTICAL TO THE SINGLE-KEY TABLE VERSION
void double_key_bin_table_copy_to(DOUBLE_KEY_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  uint32 left = table->count;
  uint32 *forward_array = table->forward_array;
  for (uint32 arg1=0 ; left > 0 ; arg1++) {
    assert(arg1 < table->forward_capacity);
    uint32 arg2 = forward_array[arg1];
    if (arg2 != 0xFFFFFFFF) {
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      append(*strm_1, obj1);
      append(*strm_2, obj2);

      left--;
    }
  }
}

//## ALMOST IDENTICAL TO THE SINGLE-KEY TABLE VERSION
void double_key_bin_table_write(WRITE_FILE_STATE *write_state, DOUBLE_KEY_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool as_map, bool flipped) {
  uint32 left = table->count;
  uint32 *forward_array = table->forward_array;
  for (uint32 arg1=0 ; left > 0 ; arg1++) {
    assert(arg1 < table->forward_capacity);
    uint32 arg2 = forward_array[arg1];
    if (arg2 != 0xFFFFFFFF) {
      OBJ obj1 = surr_to_obj_1(store_1, arg1);
      OBJ obj2 = surr_to_obj_2(store_2, arg2);
      write_str(write_state, "\n    ");
      write_obj(write_state, flipped ? obj2 : obj1);
      write_str(write_state, as_map ? " -> " : ", ");
      write_obj(write_state, flipped ? obj1 : obj2);
      if (--left > 0)
        write_str(write_state, as_map ? "," : ";");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

STATE_MEM_POOL *double_key_bin_table_mem_pool(DOUBLE_KEY_BIN_TABLE *table) {
  return (STATE_MEM_POOL *) (((uint8 *) table) - table->mem_pool_offset);
}
