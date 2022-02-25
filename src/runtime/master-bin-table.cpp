#include "lib.h"
#include "one-way-bin-table.h"


inline uint32 unpack_arg1(uint64 args) {
  return (uint32) (args >> 32);
}

inline uint32 unpack_arg2(uint64 args) {
  return (uint32) args;
}

inline bool is_empty(uint64 slot) {
  return get_high_32(slot) == 0xFFFFFFFF;
}

inline uint64 pack_args(uint32 arg1, uint32 arg2) {
  uint64 args = (((uint64) arg1) << 32) | arg2;
  assert(unpack_arg1(args) == arg1);
  assert(unpack_arg2(args) == arg2);
  assert(!is_empty(args));
  return args;
}

inline uint64 empty_slot(uint32 next) {
  uint64 slot = pack(next, 0xFFFFFFFF);
  assert(is_empty(slot));
  return slot;
}

inline uint32 get_next_free(uint64 slot) {
  assert(get_high_32(slot) == 0xFFFFFFFF);
  assert(get_low_32(slot) <= 16 * 1024 * 1024);
  return get_low_32(slot);
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
      slots[i] = empty_slot(i + 1);
    table->slots = slots;
    table->capacity = new_capacity;
  }

  table->first_free = get_next_free(slots[first_free]);
  uint64 args = pack_args(arg1, arg2);
  slots[first_free] = args;
  return first_free;
}

static void master_bin_table_release_surr(MASTER_BIN_TABLE *table, uint32 surr) {
  uint64 *slot_ptr = table->slots + surr;
  *slot_ptr = table->first_free;
  table->first_free = surr;
}

////////////////////////////////////////////////////////////////////////////////

const uint32 INIT_SIZE = 256;

void master_bin_table_init(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  loaded_one_way_bin_table_init(&table->forward, mem_pool);
  one_way_bin_table_init(&table->backward, mem_pool);

  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, INIT_SIZE);
  for (uint32 i=0 ; i < INIT_SIZE ; i++)
    slots[i] = empty_slot(i + 1);

  table->slots = slots;
  table->capacity = INIT_SIZE;
  table->first_free = 0;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_size(MASTER_BIN_TABLE *table) {
  return table->forward.count;
}

uint32 master_bin_table_count_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_get_count(&table->forward, arg1);
}

uint32 master_bin_table_count_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_get_count(&table->backward, arg2);
}

bool master_bin_table_contains(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return one_way_bin_table_contains(&table->forward, arg1, arg2);
}

bool master_bin_table_contains_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_contains_key(&table->forward, arg1);
}

bool master_bin_table_contains_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_contains_key(&table->backward, arg2);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_restrict_1(MASTER_BIN_TABLE *table, uint32 arg1, uint32 *arg2s, uint32 *surrs) {
  return loaded_one_way_bin_table_restrict(&table->forward, arg1, arg2s, surrs);
}

uint32 master_bin_table_restrict_2(MASTER_BIN_TABLE *table, uint32 arg2, uint32 *arg1s) {
  return one_way_bin_table_restrict(&table->backward, arg2, arg1s);
}

uint32 master_bin_table_lookup_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_lookup(&table->forward, arg1);
}

uint32 master_bin_table_lookup_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_lookup(&table->backward, arg2);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_lookup_surrogate(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  uint32 surr = loaded_one_way_bin_table_payload(&table->forward, arg1, arg2);
#ifndef NDEBUG
  if (surr != 0xFFFFFFFF) {
    assert(surr < table->capacity);
    uint64 slot = table->slots[surr];
    assert(unpack_arg1(slot) == arg1);
    assert(unpack_arg2(slot) == arg2);
  }
#endif
  return surr;
}

uint32 master_bin_table_get_arg_1(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity);
  uint64 slot = table->slots[surr];
  assert(master_bin_table_lookup_surrogate(table, unpack_arg1(slot), unpack_arg2(slot)) == surr);
  return unpack_arg1(slot);
}

uint32 master_bin_table_get_arg_2(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity);
  uint64 slot = table->slots[surr];
  assert(master_bin_table_lookup_surrogate(table, unpack_arg1(slot), unpack_arg2(slot)) == surr);
  return unpack_arg2(slot);
}

////////////////////////////////////////////////////////////////////////////////

// Code to recover the surrogate:
//   int32 code = master_bin_table_insert_ex(table, arg1, arg2);
//   uint32 surr12 = code >= 0 ? code : -code - 1;
//   bool was_new = code >= 0;
int32 master_bin_table_insert_ex(MASTER_BIN_TABLE *table, int arg1, int arg2, STATE_MEM_POOL *mem_pool) {
  if (master_bin_table_contains(table, arg1, arg2)) {
    uint32 surr = master_bin_table_lookup_surrogate(table, arg1, arg2);
    assert(surr != 0xFFFFFFFF);
    return -((int32) surr) - 1;
  }

  assert(!one_way_bin_table_contains(&table->forward, arg1, arg2));
  assert(!one_way_bin_table_contains(&table->backward, arg2, arg1));

  uint32 idx = master_bin_table_alloc_index(table, arg1, arg2, mem_pool);
  loaded_one_way_bin_table_insert_unique(&table->forward, arg1, arg2, idx, mem_pool);
  one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
  return idx;
}

bool master_bin_table_insert(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  int32 code = master_bin_table_insert_ex(table, arg1, arg2, mem_pool);
  return code >= 0;
}

void master_bin_table_clear(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  loaded_one_way_bin_table_clear(&table->forward);
  one_way_bin_table_clear(&table->backward);

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
    slots[i] = empty_slot(i + 1);
}

bool master_bin_table_delete(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  if (one_way_bin_table_delete(&table->backward, arg2, arg1)) {
    uint32 surr = loaded_one_way_bin_table_payload(&table->forward, arg1, arg2);
    assert(surr != 0xFFFFFFFF);
    master_bin_table_release_surr(table, surr);
    bool found = loaded_one_way_bin_table_delete(&table->forward, arg1, arg2);
    assert(found);
    return true;
  }
  else
    return false;
}

void master_bin_table_delete_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  uint32 count = one_way_bin_table_get_count(&table->forward, arg1);
  if (count > 0) {
    uint32 *arg2s = new_uint32_array(count);
    uint32 *surrs = new_uint32_array(count);
    loaded_one_way_bin_table_delete_by_key(&table->forward, arg1, arg2s, surrs);

    for (uint32 i=0 ; i < count ; i++) {
      one_way_bin_table_delete(&table->backward, arg2s[i], arg1);
      master_bin_table_release_surr(table, surrs[i]);
    }
  }
}

void master_bin_table_delete_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  uint32 count = one_way_bin_table_get_count(&table->backward, arg2);
  if (count > 0) {
    uint32 *arg1s = new_uint32_array(count);
    one_way_bin_table_delete_by_key(&table->backward, arg2, arg1s);

    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr = loaded_one_way_bin_table_delete(&table->forward, arg1s[i], arg2);
      assert(surr != 0xFFFFFFFF && surr < table->capacity);
      assert(unpack_arg1(table->slots[surr]) == arg1s[i]);
      assert(unpack_arg2(table->slots[surr]) == arg2);
      master_bin_table_release_surr(table, surr);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool master_bin_table_col_1_is_key(MASTER_BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->forward);
}

bool master_bin_table_col_2_is_key(MASTER_BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->backward);
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_copy_to(MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  MASTER_BIN_TABLE_ITER iter;
  master_bin_table_iter_init(table, &iter);
  while (!master_bin_table_iter_is_out_of_range(&iter)) {
    uint32 arg1 = master_bin_table_iter_get_1(&iter);
    uint32 arg2 = master_bin_table_iter_get_2(&iter);
    OBJ obj1 = surr_to_obj_1(store_1, arg1);
    OBJ obj2 = surr_to_obj_2(store_2, arg2);
    append(*strm_1, obj1);
    append(*strm_2, obj2);
    master_bin_table_iter_move_forward(&iter);
  }
}

void master_bin_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool flipped) {
  uint32 count = master_bin_table_size(table);
  bool is_map = flipped ? master_bin_table_col_2_is_key(table) : master_bin_table_col_1_is_key(table);

  uint32 idx = 0;

  MASTER_BIN_TABLE_ITER iter;
  master_bin_table_iter_init(table, &iter);

  while (!master_bin_table_iter_is_out_of_range(&iter)) {
    uint32 arg1 = master_bin_table_iter_get_1(&iter);
    uint32 arg2 = master_bin_table_iter_get_2(&iter);
    OBJ obj1 = surr_to_obj_1(store_1, arg1);
    OBJ obj2 = surr_to_obj_2(store_2, arg2);

    write_str(write_state, "\n    ");
    write_obj(write_state, flipped ? obj2 : obj1);
    write_str(write_state, is_map ? " -> " : ", ");
    write_obj(write_state, flipped ? obj1 : obj2);
    if (++idx != count)
      write_str(write_state, is_map ? "," : ";");

    master_bin_table_iter_move_forward(&iter);
  }
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_iter_init_empty(MASTER_BIN_TABLE_ITER *iter) {
#ifndef NDEBUG
  iter->slots = NULL;
  iter->index = 0;
#endif
  iter->left = 0;
}

void master_bin_table_iter_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER *iter) {
  uint32 count = master_bin_table_size(table);
  if (count > 0) {
    uint64 *slots = table->slots;
    uint32 index = 0;
    while (is_empty(*slots)) {
      slots++;
      index++;
    }
    iter->slots = slots;
    iter->index = index;
  }
#ifndef NDEBUG
  else {
    iter->slots = NULL;
    iter->index = 0;
  }
#endif
  iter->left = count;
}

void master_bin_table_iter_move_forward(MASTER_BIN_TABLE_ITER *iter) {
  assert(!master_bin_table_iter_is_out_of_range(iter));
  uint64 *slots = iter->slots + 1;
  uint32 index = iter->index + 1;
  while (is_empty(*slots)) {
    slots++;
    index++;
  }
  iter->slots = slots;
  iter->index = index;
  iter->left--;
}

bool master_bin_table_iter_is_out_of_range(MASTER_BIN_TABLE_ITER *iter) {
  return iter->left == 0;
}

uint32 master_bin_table_iter_get_1(MASTER_BIN_TABLE_ITER *iter) {
  assert(!master_bin_table_iter_is_out_of_range(iter));
  return unpack_arg1(*iter->slots);
}

uint32 master_bin_table_iter_get_2(MASTER_BIN_TABLE_ITER *iter) {
  assert(!master_bin_table_iter_is_out_of_range(iter));
  return unpack_arg2(*iter->slots);
}

uint32 master_bin_table_iter_get_surr(MASTER_BIN_TABLE_ITER *iter) {
  assert(!master_bin_table_iter_is_out_of_range(iter));
  return iter->index;
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_iter_1_init_empty(MASTER_BIN_TABLE_ITER_1 *iter) {
#ifndef NDEBUG
  iter->arg2s = NULL;
  iter->surrs = NULL;
#endif
  iter->left = 0;
}

void master_bin_table_iter_1_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER_1 *iter, uint32 arg1) {
  uint32 count = master_bin_table_count_1(table, arg1);
  if (count > 0) {
    uint32 *arg2s = count <= BIN_TABLE_ITER_INLINE_SIZE ? iter->inline_array : new_uint32_array(count);
    bin_table_restrict_1((BIN_TABLE *) table, arg1, arg2s); //## DON'T LIKE THIS CAST, REFACTOR ASAP
    iter->arg2s = arg2s;
    iter->surrs = NULL; //## NOT STRICTLY NECESSARY, SHOULD ONLY BE DONE FOR DEBUGGING
  }
#ifndef NDEBUG
  else {
    iter->arg2s = NULL;
    iter->surrs = NULL;
  }
#endif
  iter->left = count;
}

void master_bin_table_iter_1_init_surrs(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER_1 *iter, uint32 arg1) {
  uint32 count = master_bin_table_count_1(table, arg1);
  if (count > 0) {
    uint32 *arg2s;
    if (2 * count <= BIN_TABLE_ITER_INLINE_SIZE)
      arg2s = iter->inline_array;
    else
      arg2s = new_uint32_array(2 * count);
    uint32 *surrs = arg2s + count;
    master_bin_table_restrict_1(table, arg1, arg2s, surrs);
    iter->arg2s = arg2s;
    iter->surrs = surrs;
  }
#ifndef NDEBUG
  else {
    iter->arg2s = NULL;
    iter->surrs = NULL;
  }
#endif
  iter->left = count;
}

void master_bin_table_iter_1_move_forward(MASTER_BIN_TABLE_ITER_1 *iter) {
  assert(!master_bin_table_iter_1_is_out_of_range(iter));
  iter->arg2s++;
  iter->surrs++;
  iter->left--;
}

bool master_bin_table_iter_1_is_out_of_range(MASTER_BIN_TABLE_ITER_1 *iter) {
  return iter->left == 0;
}

uint32 master_bin_table_iter_1_get_1(MASTER_BIN_TABLE_ITER_1 *iter) {
  assert(!master_bin_table_iter_1_is_out_of_range(iter));
  return *iter->arg2s;
}

uint32 master_bin_table_iter_1_get_surr(MASTER_BIN_TABLE_ITER_1 *iter) {
  assert(!master_bin_table_iter_1_is_out_of_range(iter));
  return *iter->surrs;
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_iter_2_init_empty(MASTER_BIN_TABLE_ITER_2 *iter) {
#ifndef NDEBUG
  iter->forward = NULL;
  iter->arg1s = NULL;
  iter->arg2 = 0xFFFFFFFF;
#endif
  iter->left = 0;
}

void master_bin_table_iter_2_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER_2 *iter, uint32 arg2) {
  uint32 count = master_bin_table_count_2(table, arg2);
  if (count > 0) {
    uint32 *arg1s = count <= BIN_TABLE_ITER_INLINE_SIZE ? iter->inline_array : new_uint32_array(count);
    master_bin_table_restrict_2(table, arg2, arg1s);
    iter->forward = &table->forward;
    iter->arg1s = arg1s;
    iter->arg2 = arg2;
  }
#ifndef NDEBUG
  else {
    iter->forward = NULL;
    iter->arg1s = NULL;
    iter->arg2 = 0xFFFFFFFF;
  }
#endif
  iter->left = count;
}

void master_bin_table_iter_2_move_forward(MASTER_BIN_TABLE_ITER_2 *iter) {
  assert(!master_bin_table_iter_2_is_out_of_range(iter));
  iter->arg1s++;
  iter->left--;
}

bool master_bin_table_iter_2_is_out_of_range(MASTER_BIN_TABLE_ITER_2 *iter) {
  return iter->left == 0;
}

uint32 master_bin_table_iter_2_get_1(MASTER_BIN_TABLE_ITER_2 *iter) {
  assert(!master_bin_table_iter_2_is_out_of_range(iter));
  return *iter->arg1s;
}

uint32 master_bin_table_iter_2_get_surr(MASTER_BIN_TABLE_ITER_2 *iter) {
  assert(!master_bin_table_iter_2_is_out_of_range(iter));
  uint32 arg1 = *iter->arg1s;
  uint32 arg2 = iter->arg2;
  return loaded_one_way_bin_table_payload(iter->forward, arg1, arg2);
}
