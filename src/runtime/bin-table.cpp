#include "lib.h"
#include "one-way-bin-table.h"


inline bool bin_table_reverse_one_way_table_has_been_built(BIN_TABLE *table) {
  return counter_is_cleared(&table->col_2_counter);
}

inline void bin_table_build_reverse_one_way_table(BIN_TABLE *table) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
  one_way_bin_table_build_reverse(&table->forward, &table->backward, mem_pool);
  counter_clear(&table->col_2_counter, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_init(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_init(&table->forward, mem_pool);
  one_way_bin_table_init(&table->backward, mem_pool);
  counter_init(&table->col_2_counter, mem_pool);
  table->mem_pool_offset = ((uint8 *) table) - ((uint8 *) mem_pool);

  assert(bin_table_mem_pool(table) == mem_pool);
}

uint32 bin_table_size(BIN_TABLE *table) {
  return table->forward.count;
}

bool bin_table_contains(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return one_way_bin_table_contains(&table->forward, arg1, arg2);
}

bool bin_table_contains_1(BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_contains_key(&table->forward, arg1);
}

bool bin_table_contains_2(BIN_TABLE *table, uint32 arg2) {
  if (!bin_table_reverse_one_way_table_has_been_built(table))
    bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_contains_key(&table->backward, arg2);
}

uint32 bin_table_count_1(BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_get_count(&table->forward, arg1);
}

uint32 bin_table_count_2(BIN_TABLE *table, uint32 arg2) {
  if (bin_table_reverse_one_way_table_has_been_built(table))
    return one_way_bin_table_get_count(&table->backward, arg2);
  else
    return counter_read(&table->col_2_counter, arg2);
}

uint32 bin_table_restrict_1(BIN_TABLE *table, uint32 arg1, uint32 *arg2s) {
  return one_way_bin_table_restrict(&table->forward, arg1, arg2s);
}

uint32 bin_table_restrict_2(BIN_TABLE *table, uint32 arg2, uint32 *arg1s) {
  if (!bin_table_reverse_one_way_table_has_been_built(table))
    bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_restrict(&table->backward, arg2, arg1s);
}

UINT32_ARRAY bin_table_range_restrict_1(BIN_TABLE *table, uint32 arg1, uint32 first, uint32 *arg2s, uint32 capacity) {
  return one_way_bin_table_range_restrict(&table->forward, arg1, first, arg2s, capacity);
}

UINT32_ARRAY bin_table_range_restrict_2(BIN_TABLE *table, uint32 arg2, uint32 first, uint32 *arg1s, uint32 capacity) {
  if (!bin_table_reverse_one_way_table_has_been_built(table))
    bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_range_restrict(&table->backward, arg2, first, arg1s, capacity);
}

uint32 bin_table_lookup_1(BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_lookup(&table->forward, arg1);
}

uint32 bin_table_lookup_2(BIN_TABLE *table, uint32 arg2) {
  if (!bin_table_reverse_one_way_table_has_been_built(table))
    bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_lookup(&table->backward, arg2);
}

uint32 bin_table_lookup_unstable_surr(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return one_way_bin_table_lookup_unstable_surr(&table->forward, arg1, arg2);
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_insert(BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  //## WHY IS THIS VERSION FASTER?
  if (!one_way_bin_table_contains(&table->forward, arg1, arg2)) {
    one_way_bin_table_insert_unique(&table->forward, arg1, arg2, mem_pool);
    if (bin_table_reverse_one_way_table_has_been_built(table))
      one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
    else
      counter_incr(&table->col_2_counter, arg2, mem_pool);
    return true;
  }
  // if (one_way_bin_table_insert(&table->forward, arg1, arg2, mem_pool)) {
  //   //## NOT SURE IF THE UNIQUE VERSION IS ANY BETTER (OR MAYBE EVEN SLIGHTLY WORSE?) THAN THE STANDARD ONE
  //   one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
  //   return true;
  // }
  return false;
}

bool bin_table_delete(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  bool found = one_way_bin_table_delete(&table->forward, arg1, arg2);
  if (found) {
    if (bin_table_reverse_one_way_table_has_been_built(table)) {
      assert(one_way_bin_table_contains(&table->backward, arg2, arg1));
      one_way_bin_table_delete(&table->backward, arg2, arg1);
      assert(!one_way_bin_table_contains(&table->backward, arg2, arg1));
    }
    else {
      STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
      counter_decr(&table->col_2_counter, arg2, mem_pool);
    }
  }
  return found;
}

void bin_table_delete_1(BIN_TABLE *table, uint32 arg1) {
  uint32 count = bin_table_count_1(table, arg1);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *arg2s = count <= 1024 ? inline_array : new_uint32_array(count);
    one_way_bin_table_delete_by_key(&table->forward, arg1, arg2s);

    if (bin_table_reverse_one_way_table_has_been_built(table)) {
      for (uint32 i=0 ; i < count ; i++) {
        bool found = one_way_bin_table_delete(&table->backward, arg2s[i], arg1);
        assert(found);
      }
    }
    else {
      STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
      for (uint32 i=0 ; i < count ; i++)
        counter_decr(&table->col_2_counter, arg2s[i], mem_pool);
    }
  }
}

void bin_table_delete_2(BIN_TABLE *table, uint32 arg2) {
  assert(table->backward.count == 0 || table->backward.count == table->forward.count);

  if (!bin_table_reverse_one_way_table_has_been_built(table))
    bin_table_build_reverse_one_way_table(table);

  uint32 count = bin_table_count_2(table, arg2);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *arg1s;
    if (count <= 1024)
      arg1s = inline_array;
    else
      arg1s = new_uint32_array(count);

    one_way_bin_table_delete_by_key(&table->backward, arg2, arg1s);

    for (uint32 i=0 ; i < count ; i++) {
      bool found = one_way_bin_table_delete(&table->forward, arg1s[i], arg2);
      assert(found);
    }
  }
}

void bin_table_clear(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_clear(&table->forward, mem_pool);
  one_way_bin_table_clear(&table->backward, mem_pool);
  counter_clear(&table->col_2_counter, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_col_1_is_key(BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->forward);
}

bool bin_table_col_2_is_key(BIN_TABLE *table) {
  assert(table->backward.count == 0 || table->backward.count == table->forward.count);

  if (table->forward.count > 0 && table->backward.count == 0)
    bin_table_build_reverse_one_way_table(table);

  return one_way_bin_table_is_map(&table->backward);
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_copy_to(BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  uint32 count = bin_table_size(table);
  uint32 read = 0;
  for (uint32 arg1=0 ; read < count ; arg1++) {
    uint32 count_1 = bin_table_count_1(table, arg1);
    if (count_1 > 0) {
      read += count_1;
      uint32 read_1 = 0;
      do {
        uint32 buffer[64];
        UINT32_ARRAY array = bin_table_range_restrict_1(table, arg1, read_1, buffer, 64);
        read_1 += array.size;
        for (uint32 i=0 ; i < array.size ; i++) {
          uint32 arg2 = array.array[i];

          OBJ obj1 = surr_to_obj_1(store_1, arg1);
          OBJ obj2 = surr_to_obj_2(store_2, arg2);
          append(*strm_1, obj1);
          append(*strm_2, obj2);

        }
      } while (read_1 < count_1);
    }
  }
}

void bin_table_write(WRITE_FILE_STATE *write_state, BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool as_map, bool flipped) {
  uint32 count = bin_table_size(table);
  uint32 read = 0;
  for (uint32 arg1=0 ; read < count ; arg1++) {
    uint32 count_1 = bin_table_count_1(table, arg1);
    if (count_1 > 0) {
      // read += count_1;
      uint32 read_1 = 0;
      do {
        uint32 buffer[64];
        UINT32_ARRAY array = bin_table_range_restrict_1(table, arg1, read_1, buffer, 64);
        read_1 += array.size;
        for (uint32 i=0 ; i < array.size ; i++) {
          read++;
          uint32 arg2 = array.array[i];

          OBJ obj1 = surr_to_obj_1(store_1, arg1);
          OBJ obj2 = surr_to_obj_2(store_2, arg2);
          write_str(write_state, "\n    ");
          write_obj(write_state, flipped ? obj2 : obj1);
          write_str(write_state, as_map ? " -> " : ", ");
          write_obj(write_state, flipped ? obj1 : obj2);
          if (read < count)
            write_str(write_state, as_map ? "," : ";");

        }
      } while (read_1 < count_1);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

STATE_MEM_POOL *bin_table_mem_pool(BIN_TABLE *table) {
  return (STATE_MEM_POOL *) (((uint8 *) table) - table->mem_pool_offset);
}
