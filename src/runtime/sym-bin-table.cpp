#include "lib.h"
#include "one-way-bin-table.h"


void sym_bin_table_init(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  bin_table_init(table, mem_pool);
}

uint32 sym_bin_table_size(BIN_TABLE *table) {
  return table->forward.count;
}

bool sym_bin_table_contains(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  sort_u32(arg1, arg2);
  return one_way_bin_table_contains(&table->forward, arg1, arg2);
}

bool sym_bin_table_contains_1(BIN_TABLE *table, uint32 arg) {
  return one_way_bin_table_contains_key(&table->forward, arg) || one_way_bin_table_contains_key(&table->backward, arg);
}

uint32 sym_bin_table_count(BIN_TABLE *table, uint32 arg) {
  return one_way_bin_table_get_count(&table->forward, arg) + one_way_bin_table_get_count(&table->backward, arg);
}

uint32 sym_bin_table_count_lower(BIN_TABLE *table, uint32 arg) {
  return one_way_bin_table_get_count(&table->forward, arg);
}

uint32 sym_bin_table_restrict(BIN_TABLE *table, uint32 arg, uint32 *other_args) {
  uint32 fwd_count = one_way_bin_table_restrict(&table->forward, arg, other_args);
  uint32 bkw_count = one_way_bin_table_restrict(&table->backward, arg, other_args + fwd_count);
  return fwd_count + bkw_count;
}

UINT32_ARRAY sym_bin_table_range_restrict(BIN_TABLE *table, uint32 arg, uint32 first, uint32 *other_args, uint32 capacity) {
  uint32 fwd_count = one_way_bin_table_get_count(&table->forward, arg);
  if (first < fwd_count)
    return one_way_bin_table_range_restrict(&table->forward, arg, first, other_args, capacity);
  assert(first < fwd_count + one_way_bin_table_get_count(&table->backward, arg));
  return one_way_bin_table_range_restrict(&table->backward, arg, first - fwd_count, other_args, capacity);
}

UINT32_ARRAY sym_bin_table_range_restrict_lower(BIN_TABLE *table, uint32 lower_arg, uint32 first, uint32 *other_args, uint32 capacity) {
  return one_way_bin_table_range_restrict(&table->forward, lower_arg, first, other_args, capacity);
}

uint32 sym_bin_table_lookup(BIN_TABLE *table, uint32 arg) {
  uint32 other_arg = one_way_bin_table_lookup(&table->forward, arg);
  assert(other_arg == 0xFFFFFFFF || other_arg >= arg);
  assert(other_arg == 0xFFFFFFFF || one_way_bin_table_lookup(&table->backward, arg) == 0xFFFFFFFF);
  assert(other_arg == 0xFFFFFFFF || one_way_bin_table_lookup(&table->backward, other_arg) == 0xFFFFFFFF);
  if (other_arg == 0xFFFFFFFF)
    other_arg = one_way_bin_table_lookup(&table->backward, arg);
  else if (one_way_bin_table_lookup(&table->backward, arg) != 0xFFFFFFFF)
    internal_fail();
  return other_arg;
}

uint32 sym_bin_table_lookup_unstable_surr(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  sort_u32(arg1, arg2);
  return one_way_bin_table_lookup_unstable_surr(&table->forward, arg1, arg2);
}

////////////////////////////////////////////////////////////////////////////////

bool sym_bin_table_insert(BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  sort_u32(arg1, arg2);
  if (!one_way_bin_table_contains(&table->forward, arg1, arg2)) {
    one_way_bin_table_insert_unique(&table->forward, arg1, arg2, mem_pool);
    if (arg1 != arg2)
      one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
    return true;
  }
  return false;
}

bool sym_bin_table_delete(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  sort_u32(arg1, arg2);
  bool found = one_way_bin_table_delete(&table->forward, arg1, arg2);
  if (found & arg1 != arg2) {
    bool found_ = one_way_bin_table_delete(&table->backward, arg2, arg1);
    assert(found_ == found);
  }
  return found;
}

void sym_bin_table_delete_1(BIN_TABLE *table, uint32 arg) {
  uint32 inline_array[1024];

  uint32 count = one_way_bin_table_get_count(&table->forward, arg);
  if (count > 0) {
    uint32 *other_args = count <= 1024 ? inline_array : new_uint32_array(count);
    one_way_bin_table_delete_by_key(&table->forward, arg, other_args);

    for (uint32 i=0 ; i < count ; i++) {
      bool found = one_way_bin_table_delete(&table->backward, other_args[i], arg);
      assert(found);
    }
  }

  count = one_way_bin_table_get_count(&table->backward, arg);
  if (count > 0) {
    uint32 *other_args = count <= 1024 ? inline_array : new_uint32_array(count); //## BAD BAD BAD: NOT REUSING PREVIOUSLY ALLOCATED ARRAY
    one_way_bin_table_delete_by_key(&table->backward, arg, other_args);

    for (uint32 i=0 ; i < count ; i++) {
      bool found = one_way_bin_table_delete(&table->forward, other_args[i], arg);
      assert(found);
    }
  }
}

void sym_bin_table_clear(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_clear(&table->forward, mem_pool);
  one_way_bin_table_clear(&table->backward, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

void sym_bin_table_copy_to(BIN_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  bin_table_copy_to(table, surr_to_obj, store, surr_to_obj, store, strm_1, strm_2);
}

void sym_bin_table_write(WRITE_FILE_STATE *write_state, BIN_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store) {
  bin_table_write(write_state, table, surr_to_obj, store, surr_to_obj, store, false, false);
}
