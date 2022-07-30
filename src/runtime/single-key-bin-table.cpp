#include "lib.h"
#include "one-way-bin-table.h"


inline bool single_key_bin_table_reverse_one_way_table_has_been_built(SINGLE_KEY_BIN_TABLE *table) {
  // assert(table->backward.count == 0 || table->backward.count == table->count);
  return counter_is_cleared(&table->col_2_counter);
}

inline void single_key_bin_table_build_reverse_one_way_table(SINGLE_KEY_BIN_TABLE *table) {
  uint32 left = table->count;
  uint32 *forward_array = table->forward_array;
  for (uint32 arg1=0 ; left > 0 ; arg1++) {
    assert(arg1 < table->capacity);
    uint32 arg2 = forward_array[arg1];
    if (arg2 != 0xFFFFFFFF) {
      one_way_bin_table_insert_unique(&table->backward, arg2, arg1, table->mem_pool);
      left--;
    }
  }
  counter_clear(&table->col_2_counter, table->mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_init(SINGLE_KEY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_SIZE = 256;

  table->forward_array = alloc_state_mem_oned_uint32_array(mem_pool, INIT_SIZE);
  one_way_bin_table_init(&table->backward, mem_pool);
  table->mem_pool = mem_pool;
  counter_init(&table->col_2_counter, mem_pool);
  table->capacity = INIT_SIZE;
  table->count = 0;

#ifndef NDEBUG
  for (uint32 i=0 ; i < INIT_SIZE ; i++)
    assert(table->forward_array[i] == 0xFFFFFFFF);
#endif
}

uint32 single_key_bin_table_size(SINGLE_KEY_BIN_TABLE *table) {
  return table->count;
}

bool single_key_bin_table_contains(SINGLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return arg1 < table->capacity && table->forward_array[arg1] == arg2;
}

bool single_key_bin_table_contains_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1) {
  return arg1 < table->capacity && table->forward_array[arg1] != 0xFFFFFFFF;
}

bool single_key_bin_table_contains_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2) {
  if (!single_key_bin_table_reverse_one_way_table_has_been_built(table))
    single_key_bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_contains_key(&table->backward, arg2);
}

uint32 single_key_bin_table_count_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1) {
  return single_key_bin_table_contains_1(table, arg1) ? 1 : 0;
}

uint32 single_key_bin_table_count_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2) {
  if (single_key_bin_table_reverse_one_way_table_has_been_built(table))
    return one_way_bin_table_get_count(&table->backward, arg2);
  else
    return counter_read(&table->col_2_counter, arg2);
}

uint32 single_key_bin_table_restrict_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 *arg2s) {
  if (arg1 < table->capacity) {
    uint32 maybe_arg2 = table->forward_array[arg1];
    if (maybe_arg2 != 0xFFFFFFFF) {
      arg2s[0] = maybe_arg2;
      return 1;
    }
  }
  return 0;
}

uint32 single_key_bin_table_restrict_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2, uint32 *arg1s) {
  if (!single_key_bin_table_reverse_one_way_table_has_been_built(table))
    single_key_bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_restrict(&table->backward, arg2, arg1s);
}

UINT32_ARRAY single_key_bin_table_range_restrict_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 first, uint32 *arg2s, uint32 capacity) {
  assert(first == 0 && capacity > 0);
  uint32 count = single_key_bin_table_restrict_1(table, arg1, arg2s);
  UINT32_ARRAY result;
  result.array = arg2s;
  result.size = count;
  return result;
}

UINT32_ARRAY single_key_bin_table_range_restrict_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2, uint32 first, uint32 *arg1s, uint32 capacity) {
  if (!single_key_bin_table_reverse_one_way_table_has_been_built(table))
    single_key_bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_range_restrict(&table->backward, arg2, first, arg1s, capacity);
}

uint32 single_key_bin_table_lookup_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1) {
  return arg1 < table->capacity ? table->forward_array[arg1] : 0xFFFFFFFF;
}

uint32 single_key_bin_table_lookup_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2) {
  if (!single_key_bin_table_reverse_one_way_table_has_been_built(table))
    single_key_bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_lookup(&table->backward, arg2);
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_insert(SINGLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;
  uint32 *forward_array = table->forward_array;

  if (arg1 >= capacity) {
    //## BAD BAD BAD: JUST WRITE A FUNCTION THAT DOES THIS DOUBLING OF CAPACITY AND USE IT EVERYWHERE
    uint32 new_capacity = 2 * capacity;
    while (arg1 >= new_capacity)
      new_capacity *= 2;
    forward_array = extend_state_mem_oned_uint32_array(mem_pool, forward_array, capacity, new_capacity);
    table->forward_array = forward_array;
    table->capacity = new_capacity;
    // capacity = new_capacity; ## NOT NECESSARY
  }

  uint32 content = forward_array[arg1];
  if (content != 0xFFFFFFFF)
    soft_fail(NULL); //## BAD BAD BAD: THIS SHOULD NOT HAPPEN EXCEPT DURING LOADING, SO MAYBE THIS IS NOT THE RIGHT PLACE FOR THIS CHECK?

  forward_array[arg1] = arg2;
  table->count++;

  if (single_key_bin_table_reverse_one_way_table_has_been_built(table))
    one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
  else
    counter_incr(&table->col_2_counter, arg2, mem_pool);
}

bool single_key_bin_table_delete(SINGLE_KEY_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  if (arg1 < table->capacity) {
    uint32 *forward_array = table->forward_array;
    uint32 content = forward_array[arg1];
    if (content == arg2) {
      forward_array[arg1] = 0xFFFFFFFF;
      table->count--;

      if (single_key_bin_table_reverse_one_way_table_has_been_built(table)) {
        assert(one_way_bin_table_contains(&table->backward, arg2, arg1));
        one_way_bin_table_delete(&table->backward, arg2, arg1);
        assert(!one_way_bin_table_contains(&table->backward, arg2, arg1));
      }
      else
        counter_decr(&table->col_2_counter, arg2, table->mem_pool);

      return true;
    }
  }
  return false;
}

void single_key_bin_table_delete_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1) {
  if (arg1 < table->capacity) {
    uint32 *forward_array = table->forward_array;
    uint32 arg2 = forward_array[arg1];
    if (arg2 != 0xFFFFFFFF) {
      forward_array[arg1] = 0xFFFFFFFF;
      table->count--;

      if (single_key_bin_table_reverse_one_way_table_has_been_built(table)) {
        assert(one_way_bin_table_contains(&table->backward, arg2, arg1));
        one_way_bin_table_delete(&table->backward, arg2, arg1);
        assert(!one_way_bin_table_contains(&table->backward, arg2, arg1));
      }
      else
        counter_decr(&table->col_2_counter, arg2, table->mem_pool);
    }
  }
}

void single_key_bin_table_delete_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2) {
  assert(table->backward.count == 0 || table->backward.count == table->count);

  if (!single_key_bin_table_reverse_one_way_table_has_been_built(table))
    single_key_bin_table_build_reverse_one_way_table(table);

  uint32 count = single_key_bin_table_count_2(table, arg2);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *arg1s;
    if (count <= 1024)
      arg1s = inline_array;
    else
      arg1s = new_uint32_array(count);

    one_way_bin_table_delete_by_key(&table->backward, arg2, arg1s);

    uint32 *forward_array = table->forward_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = arg1s[i];
      assert(arg1 < table->capacity && forward_array[arg1] == arg2);
      forward_array[arg1] = 0xFFFFFFFF;
    }

    assert(table->count >= count);
    table->count -= count;
  }
}

void single_key_bin_table_clear(SINGLE_KEY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  memset(table->forward_array, 0xFF, table->capacity * sizeof(uint32)); //## BAD BAD BAD: MAYBE I SHOULD REDUCE THE CAPACITY HERE
  table->count = 0;
  one_way_bin_table_clear(&table->backward, mem_pool);
  counter_clear(&table->col_2_counter, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

bool single_key_bin_table_col_1_is_key(SINGLE_KEY_BIN_TABLE *table) {
  return true;
}

bool single_key_bin_table_col_2_is_key(SINGLE_KEY_BIN_TABLE *table) {
  assert(table->backward.count == 0 || table->backward.count == table->count);

  if (!single_key_bin_table_reverse_one_way_table_has_been_built(table))
    single_key_bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_is_map(&table->backward);
}

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_copy_to(SINGLE_KEY_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  uint32 left = table->count;
  uint32 *forward_array = table->forward_array;
  for (uint32 arg1=0 ; left > 0 ; arg1++) {
    assert(arg1 < table->capacity);
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

void single_key_bin_table_write(WRITE_FILE_STATE *write_state, SINGLE_KEY_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool as_map, bool flipped) {
  uint32 left = table->count;
  uint32 *forward_array = table->forward_array;
  for (uint32 arg1=0 ; left > 0 ; arg1++) {
    assert(arg1 < table->capacity);
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
