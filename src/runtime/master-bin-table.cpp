#include "lib.h"


inline uint32 unpack_arg1(uint64 args) {
  return (uint32) (args >> 32);
}

inline uint32 unpack_arg2(uint64 args) {
  return (uint32) args;
}

inline uint64 pack_args(uint32 arg1, uint32 arg2) {
  uint64 args = (((uint64) arg1) << 32) | arg2;
  assert(unpack_arg1(args) == arg1);
  assert(unpack_arg2(args) == arg2);
  return args;
}

////////////////////////////////////////////////////////////////////////////////

static uint32 master_bin_table_alloc_index(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;
  uint32 first_free = table->first_free;
  uint64 *slots = table->slots;

  if (first_free >= capacity) {
    uint32 new_capacity = 2 * capacity;
    slots = extend_state_mem_uint64_array(mem_pool, slots, capacity, new_capacity);
    for (uint32 i=capacity ; i < new_capacity ; i++)
      slots[i] = i + 1;
    table->slots = slots;
  }

  table->first_free = (uint32) slots[first_free];
  uint64 args = pack_args(arg1, arg2);
  slots[first_free] = args;
  table->args_to_idx[args] = first_free;
  return first_free;
}

static void master_bin_table_release_index(MASTER_BIN_TABLE *table, uint32 idx) {
  uint64 *slot_ptr = table->slots + idx;
  table->args_to_idx.erase(*slot_ptr);
  *slot_ptr = table->first_free;
  table->first_free = idx;
}

////////////////////////////////////////////////////////////////////////////////

const uint32 INIT_SIZE = 256;

void master_bin_table_init(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  bin_table_init(&table->plain_table, mem_pool);

  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, INIT_SIZE);
  for (uint32 i=0 ; i < INIT_SIZE ; i++)
    slots[i] = i + 1;

  table->slots = slots;
  table->capacity = INIT_SIZE;
  table->first_free = 0;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_size(MASTER_BIN_TABLE *table) {
  return bin_table_size(&table->plain_table);
}

uint32 master_bin_table_count_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return bin_table_count_1(&table->plain_table, arg1);
}

uint32 master_bin_table_count_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return bin_table_count_2(&table->plain_table, arg2);
}

bool master_bin_table_contains(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return bin_table_contains(&table->plain_table, arg1, arg2);
}

bool master_bin_table_contains_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return bin_table_contains_1(&table->plain_table, arg1);
}

bool master_bin_table_contains_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return bin_table_contains_2(&table->plain_table, arg2);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_restrict_1(MASTER_BIN_TABLE *table, uint32 arg1, uint32 *args2) {
  return bin_table_restrict_1(&table->plain_table, arg1, args2);
}

uint32 master_bin_table_restrict_2(MASTER_BIN_TABLE *table, uint32 arg2, uint32 *args1) {
  return bin_table_restrict_2(&table->plain_table, arg2, args1);
}

uint32 master_bin_table_lookup_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return bin_table_lookup_1(&table->plain_table, arg1);
}

uint32 master_bin_table_lookup_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return bin_table_lookup_2(&table->plain_table, arg2);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_lookup_surrogate(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  uint64 args = pack_args(arg1, arg2);
  return table->args_to_idx.count(args) != 0 ? table->args_to_idx[args] : 0xFFFFFFFF;
}

uint32 master_bin_table_get_arg_1(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity);
  uint64 slot = table->slots[surr];
  assert(table->args_to_idx.count(slot) > 0 && table->args_to_idx[slot] == surr);
  return unpack_arg1(slot);
}

uint32 master_bin_table_get_arg_2(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity);
  uint64 slot = table->slots[surr];
  assert(table->args_to_idx.count(slot) > 0 && table->args_to_idx[slot] == surr);
  return unpack_arg2(slot);
}

////////////////////////////////////////////////////////////////////////////////

// Code to recover the surrogate:
//   int32 code = master_bin_table_insert_ex(table, arg1, arg2);
//   uint32 surr12 = code >= 0 ? code : -code - 1;
//   bool was_new = code >= 0;
int32 master_bin_table_insert_ex(MASTER_BIN_TABLE *table, int arg1, int arg2, STATE_MEM_POOL *mem_pool) {
  if (bin_table_contains(&table->plain_table, arg1, arg2)) {
    assert(table->args_to_idx.count(pack_args(arg1, arg2)) != 0);
    int32 idx = (int32) table->args_to_idx[pack_args(arg1, arg2)];
    return -idx - 1;
  }

  uint32 idx = master_bin_table_alloc_index(table, arg1, arg2, mem_pool);
  bool is_new = bin_table_insert(&table->plain_table, arg1, arg2, mem_pool);
  assert(is_new);
  return idx;
}

bool master_bin_table_insert(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  int32 code = master_bin_table_insert_ex(table, arg1, arg2, mem_pool);
  return code >= 0;
}

void master_bin_table_clear(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  bin_table_clear(&table->plain_table, mem_pool);

  table->args_to_idx.clear();

  uint32 capacity = table->capacity;
  uint64 *slots = table->slots;

  if (capacity != INIT_SIZE) {
    release_state_mem_uint64_array(mem_pool, slots, capacity);

    capacity = INIT_SIZE;
    slots = alloc_state_mem_uint64_array(mem_pool, INIT_SIZE);

    table->capacity = INIT_SIZE;
    table->slots = slots;
  }

  table->first_free = 0;
  for (uint32 i=0 ; i < capacity ; i++)
    slots[i] = i + 1;
}

bool master_bin_table_delete(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  if (!bin_table_delete(&table->plain_table, arg1, arg2))
    return false;

  uint64 args = pack_args(arg1, arg2);
  assert(table->args_to_idx.count(args) != 0);
  uint32 idx = table->args_to_idx[args];
  table->args_to_idx.erase(args);
  master_bin_table_release_index(table, idx);
  return true;
}

void master_bin_table_delete_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  uint32 count = master_bin_table_count_1(table, arg1);
  if (count > 0) {
    uint32 *args2 = new_uint32_array(count);
    uint32 count_ = master_bin_table_restrict_1(table, arg1, args2);
    assert(count_ == count);
    bin_table_delete_1(&table->plain_table, arg1);

    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg2 = args2[i];
      uint64 args = pack_args(arg1, arg2);
      assert(table->args_to_idx.count(args) != 0);
      uint32 idx = table->args_to_idx[args];
      table->args_to_idx.erase(args);
      master_bin_table_release_index(table, idx);
    }
  }
}

void master_bin_table_delete_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  uint32 count = master_bin_table_count_2(table, arg2);
  if (count > 0) {
    uint32 *args1 = new_uint32_array(count);
    uint32 count_ = master_bin_table_restrict_2(table, arg2, args1);
    assert(count_ == count);
    bin_table_delete_2(&table->plain_table, arg2);

    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = args1[i];
      uint64 args = pack_args(arg1, arg2);
      assert(table->args_to_idx.count(args) != 0);
      uint32 idx = table->args_to_idx[args];
      table->args_to_idx.erase(args);
      master_bin_table_release_index(table, idx);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool master_bin_table_col_1_is_key(MASTER_BIN_TABLE *table) {
  return bin_table_col_1_is_key(&table->plain_table);
}

bool master_bin_table_col_2_is_key(MASTER_BIN_TABLE *table) {
  return bin_table_col_2_is_key(&table->plain_table);
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_copy_to(MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  bin_table_copy_to(&table->plain_table, surr_to_obj_1, store_1, surr_to_obj_2, store_2, strm_1, strm_2);
}

void master_bin_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool flipped) {
  bin_table_write(write_state, &table->plain_table, surr_to_obj_1, store_1, surr_to_obj_2, store_2, flipped);
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_iter_init_empty(MASTER_BIN_TABLE_ITER *iter) {
  bin_table_iter_init_empty(&iter->iter);
}

void master_bin_table_iter_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER *iter) {
  bin_table_iter_init(&table->plain_table, &iter->iter);
}

void master_bin_table_iter_move_forward(MASTER_BIN_TABLE_ITER *iter) {
  bin_table_iter_move_forward(&iter->iter);
}

bool master_bin_table_iter_is_out_of_range(MASTER_BIN_TABLE_ITER *iter) {
  return bin_table_iter_is_out_of_range(&iter->iter);
}

uint32 master_bin_table_iter_get_1(MASTER_BIN_TABLE_ITER *iter) {
  return bin_table_iter_get_1(&iter->iter);
}

uint32 master_bin_table_iter_get_2(MASTER_BIN_TABLE_ITER *iter) {
  return bin_table_iter_get_2(&iter->iter);
}

uint32 master_bin_table_iter_get_surr(MASTER_BIN_TABLE_ITER *iter) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_iter_1_init_empty(MASTER_BIN_TABLE_ITER_1 *iter) {
  bin_table_iter_1_init_empty(&iter->iter);
}

void master_bin_table_iter_1_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER_1 *iter, uint32 arg1) {
  bin_table_iter_1_init(&table->plain_table, &iter->iter, arg1);
}

void master_bin_table_iter_1_move_forward(MASTER_BIN_TABLE_ITER_1 *iter) {
  bin_table_iter_1_move_forward(&iter->iter);
}

bool master_bin_table_iter_1_is_out_of_range(MASTER_BIN_TABLE_ITER_1 *iter) {
  return bin_table_iter_1_is_out_of_range(&iter->iter);
}

uint32 master_bin_table_iter_1_get_1(MASTER_BIN_TABLE_ITER_1 *iter) {
  return bin_table_iter_1_get_1(&iter->iter);
}

uint32 master_bin_table_iter_1_get_surr(MASTER_BIN_TABLE_ITER_1 *iter) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_iter_2_init_empty(MASTER_BIN_TABLE_ITER_2 *iter) {
  bin_table_iter_2_init_empty(&iter->iter);
}

void master_bin_table_iter_2_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER_2 *iter, uint32 arg2) {
  bin_table_iter_2_init(&table->plain_table, &iter->iter, arg2);
}

void master_bin_table_iter_2_move_forward(MASTER_BIN_TABLE_ITER_2 *iter) {
  bin_table_iter_2_move_forward(&iter->iter);
}

bool master_bin_table_iter_2_is_out_of_range(MASTER_BIN_TABLE_ITER_2 *iter) {
  return bin_table_iter_2_is_out_of_range(&iter->iter);
}

uint32 master_bin_table_iter_2_get_1(MASTER_BIN_TABLE_ITER_2 *iter) {
  return bin_table_iter_2_get_1(&iter->iter);
}

uint32 master_bin_table_iter_2_get_surr(MASTER_BIN_TABLE_ITER_2 *iter) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}
