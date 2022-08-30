#include "lib.h"


// const uint32 INIT_SIZE = 256;

////////////////////////////////////////////////////////////////////////////////

// inline uint32 hash_index(int64 value, uint32 capacity) {
//   uint32 hashcode = (uint32) (value ^ (value >> 32));
//   return hashcode % (capacity / HASHTABLE_SCALE);
// }

////////////////////////////////////////////////////////////////////////////////

// inline bool is_empty(uint64 slot) {
//   uint32 tag = (uint32) (slot >> 62);
//   assert(tag == 0 | tag == 1 | tag == 2);
//   return tag == 2;
//   // return (slot >>> 62) == 2;
// }

// inline int64 get_value(uint64 slot, INT_STORE *store) {
//   assert(!is_empty(slot));
//   uint32 tag = (uint32) (slot >> 62);
//   assert(tag == 0 | tag == 1);
//   uint32 lsword = (uint32) (slot & 0xFFFFFFFF);
//   return tag == 0 ? (int32) lsword : store->large_ints[lsword];
// }

// inline uint32 get_next_in_bucket(uint64 slot) {
//   assert(!is_empty(slot));
//   return (uint32) ((slot >> 32) & 0x3FFFFFFF);
// }

// inline uint32 get_next_free(uint64 slot) {
//   assert(is_empty(slot));
//   return (uint32) ((slot >> 32) & 0x3FFFFFFF);
// }

////////////////////////////////////////////////////////////////////////////////

// inline uint64 empty_slot(uint32 next) {
//   assert(next <= 0x1FFFFFFF);
//   return (((uint64) next) | (2ULL << 30)) << 32;
// }

// inline uint64 filled_value_slot(int32 value, uint32 next) {
//   uint32 unsigned_value = (uint32) value;
//   uint64 slot = ((uint64) unsigned_value) | (((uint64) next) << 32);
//   assert(!is_empty(slot));
//   assert(get_value(slot, NULL) == value);
//   assert(get_next_in_bucket(slot) == next);
//   return slot;
// }

// inline uint64 filled_idx_slot(uint32 idx, uint32 next) {
//   uint64 slot = ((uint64) idx) | (((uint64) next) << 32) | (1L << 62);
//   assert(!is_empty(slot));
//   // assert(get_value(slot) == large_ints[idx]); //## THIS CHECK SHOULD BE REPLACED, NOT ELIMINATED
//   assert(get_next_in_bucket(slot) == next);
//   return slot;
// }

// inline uint64 reindexed_slot(uint64 slot, uint32 next) {
//   uint32 tag = (uint32) (slot >> 62);
//   assert(tag == 0 | tag == 1);
//   return tag == 0 ? filled_value_slot((uint32) slot, next) : filled_idx_slot((uint32) slot, next);
// }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void int_store_resize(INT_STORE *store, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  assert(min_capacity > store->capacity);

  uint32 curr_capacity = store->capacity;
  uint32 new_capacity = 2 * curr_capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;
  store->capacity = new_capacity;

  int64 *curr_surr_to_value_array = store->surr_to_value_array;
  int64 *new_surr_to_value_array = alloc_state_mem_int64_array(mem_pool, new_capacity); //## REMEMBER TO RELEASE
  store->surr_to_value_array = new_surr_to_value_array;

  uint32 *curr_partial_value_to_surr_array = store->partial_value_to_surr_array;
  uint32 *new_partial_value_to_surr_array = alloc_state_mem_uint32_array(mem_pool, new_capacity); //## REMEMBER TO RELEASE
  store->partial_value_to_surr_array = new_partial_value_to_surr_array;

  memset(new_partial_value_to_surr_array, 0xFF, sizeof(uint32) * new_capacity);
  map_i64_surr_clear(&store->value_to_surr_map);

  for (uint32 i=0 ; i < curr_capacity ; i++) {
    int64 value = curr_surr_to_value_array[i];
    new_surr_to_value_array[i] = value;
    if (value >= 0 & value < new_capacity)
      new_partial_value_to_surr_array[value] = i;
    else
      map_i64_surr_insert_new(&store->value_to_surr_map, value, i);
  }

  for (uint32 i=curr_capacity ; i < new_capacity ; i++)
    new_surr_to_value_array[i] = i + 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void int_store_init(INT_STORE *store, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_CAPACITY = 256;

  int64 *surr_to_value_array = alloc_state_mem_int64_array(mem_pool, INIT_CAPACITY);
  store->surr_to_value_array = surr_to_value_array;
  for (int i=0 ; i < INIT_CAPACITY ; i++)
    surr_to_value_array[i] = i + 1;

  uint32 *partial_value_to_surr_array = alloc_state_mem_uint32_array(mem_pool, INIT_CAPACITY);
  store->partial_value_to_surr_array = partial_value_to_surr_array;
  memset(partial_value_to_surr_array, 0xFF, sizeof(uint32) * INIT_CAPACITY);

  map_i64_surr_init(&store->value_to_surr_map);

  store->capacity = INIT_CAPACITY;
  store->count = 0;
  store->first_free_surr = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 int_store_value_to_surr(INT_STORE *store, int64 value) {
  if (value >= 0 && value < store->capacity)
    return store->partial_value_to_surr_array[value];
  else
    return map_i64_surr_lookup(&store->value_to_surr_map, value);

  // uint32 hash_idx = hash_index(value, store->capacity);
  // uint32 idx = store->hashtable[hash_idx];
  // if (idx == INV_IDX)
  //   return 0xFFFFFFFF;
  // uint64 *slots = store->slots;
  // do {
  //   uint64 slot = slots[idx];
  //   if (get_value(slot, store) == value) //## WE CAN PROBABLY IMPROVE HERE: value_is(slot, value, store)
  //     return idx;
  //   idx = get_next_in_bucket(slot);
  // } while (idx != INV_IDX);
  // return 0xFFFFFFFF;
}

int64 int_store_surr_to_value(INT_STORE *store, uint32 surr) {
  assert(surr < store->capacity);
  return store->surr_to_value_array[surr];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void int_store_clear(INT_STORE *store) {
  uint32 capacity = store->capacity;

  int64 *surr_to_value_array = store->surr_to_value_array;
  for (int i=0 ; i < capacity ; i++)
    surr_to_value_array[i] = i + 1;

  memset(store->partial_value_to_surr_array, 0xFF, sizeof(uint32) * capacity);

  map_i64_surr_clear(&store->value_to_surr_map);

  store->count = 0;
  store->first_free_surr = 0;
}

void int_store_remove(INT_STORE *store, uint32 index) {
  int64 *surr_to_value_array = store->surr_to_value_array;
  int64 value = surr_to_value_array[index];
  surr_to_value_array[index] = store->first_free_surr;
  store->first_free_surr = index;
  store->count--;

  if (value >= 0 && value < store->capacity)
    store->partial_value_to_surr_array[value] = 0xFFFFFFFF;
  else
    map_i64_surr_delete(&store->value_to_surr_map, value);
}

void int_store_insert(INT_STORE *store, int64 value, uint32 surr, STATE_MEM_POOL *mem_pool) {
  assert(store->first_free_surr == surr);
  assert(surr < store->capacity);
  assert(int_store_value_to_surr(store, value) == 0xFFFFFFFF);
  assert(store->surr_to_value_array[surr] >= 0);
  assert(store->surr_to_value_array[surr] <= store->capacity);

  int64 *surr_to_value_array = store->surr_to_value_array;
  uint32 next_free_surr = (uint32) surr_to_value_array[surr];
  store->first_free_surr = next_free_surr;
  store->count++;

  surr_to_value_array[surr] = value;

  if (value >= 0 && value < store->capacity)
    store->partial_value_to_surr_array[value] = surr;
  else
    map_i64_surr_insert_new(&store->value_to_surr_map, value, surr);
}

static uint32 int_store_insert(INT_STORE *store, int64 value, STATE_MEM_POOL *mem_pool) {
  assert(int_store_value_to_surr(store, value) == 0xFFFFFFFF);

  uint32 capacity = store->capacity;
  uint32 count = store->count;
  assert(count <= capacity);
  if (count == capacity)
    int_store_resize(store, count + 1, mem_pool);
  uint32 surr = store->first_free_surr;
  int_store_insert(store, value, surr, mem_pool);
  return surr;
}

uint32 int_store_lookup_or_insert(INT_STORE *store, int64 value, STATE_MEM_POOL *mem_pool) {
  uint32 surr = int_store_value_to_surr(store, value);
  if (surr == 0xFFFFFFFF)
    surr = int_store_insert(store, value, mem_pool);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 int_store_next_free_surr(INT_STORE *store, uint32 surr) {
  assert(surr == 0xFFFFFFFF || surr >= store->capacity || (store->surr_to_value_array[surr] >= 0 && store->surr_to_value_array[surr] <= store->capacity));

  if (surr == 0xFFFFFFFF)
    return store->first_free_surr;

  if (surr >= store->capacity)
    return surr + 1;

  return store->surr_to_value_array[surr];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ int_store_surr_to_obj(void *store, uint32 surr) {
  return make_int(int_store_surr_to_value((INT_STORE *) store, surr));
}

////////////////////////////////////////////////////////////////////////////////

void int_store_remove_untyped(void *ptr, uint32 surr, STATE_MEM_POOL *) {
  INT_STORE *store = (INT_STORE *) ptr;
  if (surr != 0xFFFFFFFF) {
    assert(int_store_value_to_surr(store, int_store_surr_to_value(store, surr)) == surr);
    int_store_remove(store, surr);
  }
  else
    int_store_clear(store);
}
