#include "lib.h"


static void obj_store_insert_into_hashtable(OBJ_STORE *store, uint32 index, uint32 hashcode) {
  uint32 hash_idx = hashcode % (store->capacity / 2);
  assert(hash_idx == (hashcode & store->index_mask));
  store->buckets[index] = store->hashtable[hash_idx];
  store->hashtable[hash_idx] = index;
}

static void obj_store_remove_from_hashtable(OBJ_STORE *store, uint32 index) {
  uint32 *hashtable = store->hashtable;
  uint32 *buckets = store->buckets;

  uint32 hashcode = store->hashcode_or_next_free[index];
  uint32 hash_idx = hashcode % (store->capacity / 2);
  assert(hash_idx == (hashcode & store->index_mask));
  uint32 idx = store->hashtable[hash_idx];
  assert(idx != 0xFFFFFFFF);

  if (idx == index) {
    hashtable[hash_idx] = buckets[index];
    buckets[index] = 0xFFFFFFFF; //## NOT STRICTLY NECESSARY, COMMENT OUT ONCE TESTED
    return;
  }

  uint32 prev_idx = idx;
  idx = buckets[idx];
  while (idx != index) {
    prev_idx = idx;
    idx = buckets[idx];
    assert(idx != 0xFFFFFFFF);
  }
  buckets[prev_idx] = buckets[index];
  buckets[index] = 0xFFFFFFFF; //## NOT STRICTLY NECESSARY, COMMENT OUT ONCE TESTED
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_resize(OBJ_STORE *store, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  assert(min_capacity > store->capacity);

  uint32 capacity = store->capacity;
  uint32 new_capacity = 2 * capacity;
  uint32 new_index_mask = (store->index_mask << 1) | 1;
  while (new_capacity < min_capacity) {
    new_capacity *= 2;
    new_index_mask = (new_index_mask << 1) | 1;
  }

  OBJ *values = extend_state_mem_blanked_obj_array(mem_pool, store->values, capacity, new_capacity);
  //## BUG BUG BUG: store->values IS NEVER RELEASED
  store->values = values;

  uint32 *hashcode_or_next_free = extend_state_mem_uint32_array(mem_pool, store->hashcode_or_next_free, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    hashcode_or_next_free[i] = i + 1;
  //## BUG BUG BUG: store->hashcode_or_next_free IS NEVER RELEASED
  store->hashcode_or_next_free = hashcode_or_next_free;

  release_state_mem_uint32_array(mem_pool, store->hashtable, capacity / 2);
  store->hashtable = alloc_state_mem_oned_uint32_array(mem_pool, new_capacity / 2);

  release_state_mem_uint32_array(mem_pool, store->buckets, capacity);
  store->buckets = alloc_state_mem_uint32_array(mem_pool, new_capacity);

  store->capacity = new_capacity;
  store->index_mask = new_index_mask;

  for (uint32 i=0 ; i < capacity ; i++) {
    OBJ value = values[i];
    if (!is_blank(value))
      obj_store_insert_into_hashtable(store, i, hashcode_or_next_free[i]);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_init(OBJ_STORE *store, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_SIZE = 256;

  store->values = alloc_state_mem_blanked_obj_array(mem_pool, INIT_SIZE);

  uint32 *uint32_array = alloc_state_mem_uint32_array(mem_pool, INIT_SIZE);
  for (uint32 i=0 ; i < INIT_SIZE ; i++)
    uint32_array[i] = i + 1;
  store->hashcode_or_next_free = uint32_array;

  uint32_array = alloc_state_mem_uint32_array(mem_pool, INIT_SIZE / 2);
  memset(uint32_array, 0xFF, INIT_SIZE / 2 * sizeof(uint32));
  store->hashtable = uint32_array;

  store->buckets = alloc_state_mem_uint32_array(mem_pool, INIT_SIZE);

  store->capacity = INIT_SIZE;
  store->index_mask = 0x7F;
  store->count = 0;
  store->first_free_surr = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value, uint32 hashcode) {
  uint32 hash_idx = hashcode % (store->capacity / 2);
  assert(hash_idx == (hashcode & store->index_mask));

  uint32 index = store->hashtable[hash_idx];
  //## MAYBE THESE WOULD SPEED UP THE CODE A TINY BIT? (ALREADY TRIED, NO MEASURABLE EFFECT)
  // OBJ *values = store->values;
  // uint32 *hashcode_or_next_free = store->hashcode_or_next_free;
  for ( ; ; ) {
    if (index == 0xFFFFFFFF)
      return 0xFFFFFFFF;
    assert(!is_blank(store->values[index]));
    if (store->hashcode_or_next_free[index] == hashcode && are_eq(value, store->values[index]))
      return index;
    index = store->buckets[index];
  }
}

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value) {
  uint32 hashcode = compute_hashcode(value);
  return obj_store_value_to_surr(store, value, hashcode);
}

OBJ obj_store_surr_to_value(OBJ_STORE *store, uint32 surr) {
  return store->values[surr];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_clear(OBJ_STORE *store, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = store->capacity;

  OBJ *values = store->values;
  for (uint32 i=0 ; i < capacity ; i++) {
    OBJ *ptr = values + i;
    OBJ obj = *ptr;
    if (!is_blank(obj)) {
      //## BUG BUG BUG: AND WHEN IS THE MEMORY FREED?
      *ptr = make_blank_obj();
    }
  }

  uint32 *hashcode_or_next_free = store->hashcode_or_next_free;
  for (uint32 i=0 ; i < capacity ; i++)
    hashcode_or_next_free[i] = i + 1;

  memset(store->hashtable, 0xFF, capacity / 2 * sizeof(uint32));

#ifndef NDEBUG
  memset(store->buckets, 0xFF, capacity * sizeof(uint32));
#endif

  store->count = 0;
  store->first_free_surr = 0;
}

void obj_store_remove(OBJ_STORE *store, uint32 surr, STATE_MEM_POOL *mem_pool) {
  assert(surr < store->capacity && !is_blank(store->values[surr]));

  obj_store_remove_from_hashtable(store, surr);

  //## BUG BUG BUG: AND WHEN IS THE MEMORY FREED?
  store->values[surr] = make_blank_obj();
  store->hashcode_or_next_free[surr] = store->first_free_surr;
  store->first_free_surr = surr;
  store->count--;
}

void obj_store_insert(OBJ_STORE *store, OBJ value, uint32 hashcode, uint32 surr, STATE_MEM_POOL *mem_pool) {
  assert(store->first_free_surr == surr);
  assert(surr < store->capacity);
  assert(is_blank(store->values[surr]));
  assert(hashcode == compute_hashcode(value));

  store->count++;
  store->first_free_surr = store->hashcode_or_next_free[surr];
  store->values[surr] = copy_to_pool(mem_pool, value);
  store->hashcode_or_next_free[surr] = hashcode;

  obj_store_insert_into_hashtable(store, surr, hashcode);
}

static uint32 obj_store_insert(OBJ_STORE *store, OBJ value, STATE_MEM_POOL *mem_pool) {
  assert(obj_store_value_to_surr(store, value) == 0xFFFFFFFF);

  uint32 capacity = store->capacity;
  uint32 count = store->count;
  assert(count <= capacity);
  if (count == capacity)
    obj_store_resize(store, count + 1, mem_pool);
  uint32 surr = store->first_free_surr;
  uint32 hcode = compute_hashcode(value);
  obj_store_insert(store, value, hcode, surr, mem_pool);
  return surr;
}

uint32 obj_store_lookup_or_insert(OBJ_STORE *store, OBJ value, STATE_MEM_POOL *mem_pool) {
  uint32 surr = obj_store_value_to_surr(store, value);
  if (surr == 0xFFFFFFFF)
    surr = obj_store_insert(store, value, mem_pool);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_next_free_surr(OBJ_STORE *store, uint32 last_surr) {
  assert(last_surr == 0xFFFFFFFF || last_surr >= store->capacity || is_blank(store->values[last_surr]));

  if (last_surr == 0xFFFFFFFF)
    return store->first_free_surr;

  if (last_surr >= store->capacity)
    return last_surr + 1;

  return store->hashcode_or_next_free[last_surr];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ obj_store_surr_to_obj(void *store, uint32 surr) {
  return obj_store_surr_to_value((OBJ_STORE *) store, surr);
}

////////////////////////////////////////////////////////////////////////////////

void obj_store_remove_untyped(void *ptr, uint32 surr, STATE_MEM_POOL *mem_pool) {
  OBJ_STORE *store = (OBJ_STORE *) ptr;
  if (surr != 0xFFFFFFFF) {
    assert(surr < store->capacity && !is_blank(store->values[surr]));
    obj_store_remove(store, surr, mem_pool);
  }
  else
    obj_store_clear(store, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void obj_store_print_collisions_histogram(OBJ_STORE *store) {
  unordered_map<uint32, uint32> counters;

  for (uint32 i=0 ; i < store->capacity ; i++) {
    OBJ value = store->values[i];
    if (!is_blank(value)) {
      uint32 hashcode = compute_hashcode(value);
      counters[hashcode]++;
    }
  }

  uint32 histogram[9];
  for (uint32 i=0 ; i < 9 ; i++)
    histogram[i] = 0;

  for (auto it=counters.begin() ; it != counters.end() ; it++) {
    uint32 count = it->second;
    assert(count > 0);
    uint32 index = count - 1;
    if (index > 8)
      index = 8;
    histogram[index]++;
  }

  printf("  ");
  for (uint32 i=0 ; i < 9 ; i++)
    printf("%8d", histogram[i]);
  puts("");
}
#endif
