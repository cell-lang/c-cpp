#include "lib.h"


// void *alloc_state_mem_block(STATE_MEM_POOL *, uint32);
// void release_state_mem_block(STATE_MEM_POOL *, void *, uint32);


OBJ *alloc_state_mem_blanked_obj_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0);
  uint32 byte_size = size * null_round_up_8(sizeof(OBJ));
  OBJ *ptr = (OBJ *) alloc_state_mem_block(mem_pool, byte_size);
  memset(ptr, 0, byte_size);
  for (uint32 i=0 ; i < size ; i++)
    assert(is_blank(ptr[i]));
  return ptr;
}

OBJ *extend_state_mem_blanked_obj_array(STATE_MEM_POOL *mem_pool, OBJ *ptr, uint32 size, uint32 new_size) {
  assert(size > 0 && new_size > 0);
  uint32 byte_size = size * null_round_up_8(sizeof(OBJ));
  uint32 new_byte_size = new_size * null_round_up_8(sizeof(OBJ));
  OBJ *new_ptr = (OBJ *) alloc_state_mem_block(mem_pool, new_byte_size);
  memcpy(new_ptr, ptr, byte_size);
  memset(new_ptr + size, 0, new_byte_size - byte_size);
  for (uint32 i=0 ; i < new_size ; i++)
    assert(i < size ? are_shallow_eq(new_ptr[i], ptr[i]) : is_blank(new_ptr[i]));
  release_state_mem_block(mem_pool, ptr, byte_size);
  return new_ptr;
}

void release_state_mem_obj_array(STATE_MEM_POOL *mem_pool, OBJ *ptr, uint32 size) {
  release_state_mem_block(mem_pool, ptr, size * null_round_up_8(sizeof(OBJ)));
}

////////////////////////////////////////////////////////////////////////////////

double *alloc_state_mem_float_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0 && size % 2 == 0);
  return (double *) alloc_state_mem_block(mem_pool, null_round_up_8(size * sizeof(double)));
}

double *extend_state_mem_float_array(STATE_MEM_POOL *mem_pool, double *ptr, uint32 size, uint32 new_size) {
  assert(size > 0 & new_size > 0 & size % 2 == 0 & new_size % 2 == 0);
  uint32 byte_size = null_round_up_8(size * sizeof(double));
  uint32 new_byte_size = null_round_up_8(new_size * sizeof(double));
  double *new_ptr = (double *) alloc_state_mem_block(mem_pool, new_byte_size);
  memcpy(new_ptr, ptr, byte_size);
  // memset(new_ptr + size, 0, new_byte_size - byte_size);
  // for (uint32 i=0 ; i < new_size ; i++)
  //   assert(new_ptr[i] == (i < capacity ? ptr[i] : 0));
  for (uint32 i=0 ; i < size ; i++)
    assert(new_ptr[i] == ptr[i]);
  release_state_mem_block(mem_pool, ptr, byte_size);
  return new_ptr;
}

void release_state_mem_float_array(STATE_MEM_POOL *mem_pool, double *ptr, uint32 size) {
  release_state_mem_block(mem_pool, ptr, null_round_up_8(size * sizeof(double)));
}

////////////////////////////////////////////////////////////////////////////////

uint64 *alloc_state_mem_zeroed_uint64_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0);
  uint32 byte_size = null_round_up_8(size * sizeof(uint64));
  uint64 *ptr = (uint64 *) alloc_state_mem_block(mem_pool, byte_size);
  memset(ptr, 0, byte_size);
  for (int i=0 ; i < size ; i++)
    assert(ptr[i] == 0);
  return ptr;
}

void release_state_mem_uint64_array(STATE_MEM_POOL *mem_pool, uint64 *ptr, uint32 size) {
  release_state_mem_block(mem_pool, ptr, null_round_up_8(size * sizeof(uint64)));
}

////////////////////////////////////////////////////////////////////////////////

uint32 *alloc_state_mem_uint32_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0 && size % 2 == 0);
  return (uint32 *) alloc_state_mem_block(mem_pool, null_round_up_8(size * sizeof(uint32)));
}

uint32 *alloc_state_mem_oned_uint32_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0 && size % 2 == 0);
  uint32 byte_size = null_round_up_8(size * sizeof(uint32));
  uint32 *ptr = (uint32 *) alloc_state_mem_block(mem_pool, byte_size);
  memset(ptr, 0xFF, byte_size);
  for (int i=0 ; i < size ; i++)
    assert(ptr[i] == 0xFFFFFFFF);
  return ptr;
}

uint32 *extend_state_mem_uint32_array(STATE_MEM_POOL *mem_pool, uint32 *ptr, uint32 size, uint32 new_size) {
  assert(size > 0 & new_size > 0 & size % 2 == 0 & new_size % 2 == 0);
  uint32 byte_size = null_round_up_8(size * sizeof(uint32));
  uint32 new_byte_size = null_round_up_8(new_size * sizeof(uint32));
  uint32 *new_ptr = (uint32 *) alloc_state_mem_block(mem_pool, new_byte_size);
  memcpy(new_ptr, ptr, byte_size);
  // memset(new_ptr + size, 0, new_byte_size - byte_size);
  // for (uint32 i=0 ; i < new_size ; i++)
  //   assert(new_ptr[i] == (i < capacity ? ptr[i] : 0));
  for (uint32 i=0 ; i < size ; i++)
    assert(new_ptr[i] == ptr[i]);
  release_state_mem_block(mem_pool, ptr, byte_size);
  return new_ptr;
}

void release_state_mem_uint32_array(STATE_MEM_POOL *mem_pool, uint32 *ptr, uint32 size) {
  release_state_mem_block(mem_pool, ptr, round_up_8(size * sizeof(uint32)));
}

////////////////////////////////////////////////////////////////////////////////

uint8 *alloc_state_mem_uint8_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0);
  return (uint8 *) alloc_state_mem_block(mem_pool, size * sizeof(uint8));
}

uint8 *extend_state_mem_zeroed_uint8_array(STATE_MEM_POOL *mem_pool, uint8 *ptr, uint32 size, uint32 new_size) {
  assert(size > 0 & new_size > 0 & size % 8 == 0 & new_size % 8 == 0);
  uint32 byte_size = null_round_up_8(size * sizeof(uint8));
  uint32 new_byte_size = null_round_up_8(new_size * sizeof(uint8));
  uint8 *new_ptr = (uint8 *) alloc_state_mem_block(mem_pool, new_byte_size);
  memcpy(new_ptr, ptr, byte_size);
  memset(new_ptr + size, 0, new_byte_size - byte_size);
  for (uint32 i=0 ; i < new_size ; i++)
    assert(new_ptr[i] == (i < size ? ptr[i] : 0));
  release_state_mem_block(mem_pool, ptr, byte_size);
  return new_ptr;
}

void release_state_mem_uint8_array(STATE_MEM_POOL *mem_pool, uint8 *ptr, uint32 size) {
  release_state_mem_block(mem_pool, ptr, round_up_8(size * sizeof(uint8)));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void obj_store_insert_into_hashtable(OBJ_STORE *store, uint32 index, uint32 hashcode) {
  uint32 hash_idx = hashcode % (store->capacity / 2);
  store->buckets[index] = store->hashtable[hash_idx];
  store->hashtable[hash_idx] = index;
}

static void obj_store_remove_from_hashtable(OBJ_STORE *store, uint32 index) {
  uint32 *hashtable = store->hashtable;
  uint32 *buckets = store->buckets;

  uint32 hashcode = store->hashcode_or_next_free[index];
  uint32 hash_idx = hashcode % (store->capacity / 2);
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

static void obj_store_release_obj_at(OBJ_STORE *store, uint32 index) {
  assert(!is_blank(store->values[index]));

  obj_store_remove_from_hashtable(store, index);
  //## BUG BUG BUG: AND WHEN IS THE MEMORY FREED?
  store->values[index] = make_blank_obj();
  store->hashcode_or_next_free[index] = store->first_free;
  store->first_free = index;
  store->count--;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_resize(OBJ_STORE *store, STATE_MEM_POOL *mem_pool, uint32 min_capacity) {
  assert(min_capacity > store->capacity);

  uint32 capacity = store->capacity;
  uint32 new_capacity = 2 * capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;

  OBJ *values = extend_state_mem_blanked_obj_array(mem_pool, store->values, capacity, new_capacity);
  //## BUG BUG BUG: store->values IS NEVER RELEASED
  store->values = values;

  uint32 *hashcode_or_next_free = extend_state_mem_uint32_array(mem_pool, store->hashcode_or_next_free, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    hashcode_or_next_free[i] = i + 1;
  //## BUG BUG BUG: store->hashcode_or_next_free IS NEVER RELEASED
  store->hashcode_or_next_free = hashcode_or_next_free;

  //## BUG BUG BUG: store->ref_counters IS NEVER RELEASED
  uint8 *refs_counters = extend_state_mem_zeroed_uint8_array(mem_pool, store->refs_counters, capacity, new_capacity);
  store->refs_counters = refs_counters;

  release_state_mem_uint32_array(mem_pool, store->hashtable, capacity / 2);
  store->hashtable = alloc_state_mem_oned_uint32_array(mem_pool, new_capacity / 2);

  release_state_mem_uint32_array(mem_pool, store->buckets, capacity);
  store->buckets = alloc_state_mem_uint32_array(mem_pool, new_capacity);

  store->capacity = new_capacity;

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

  uint8 *uint8_array = alloc_state_mem_uint8_array(mem_pool, INIT_SIZE);
  memset(uint8_array, 0, INIT_SIZE * sizeof(uint8));
  store->refs_counters = uint8_array;

  store->capacity = INIT_SIZE;
  store->count = 0;
  store->first_free = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value, uint32 hashcode) {
  uint32 hash_idx = hashcode % (store->capacity / 2);
  uint32 index = store->hashtable[hash_idx];
  //## MAYBE THESE WOULD SPEED UP THE CODE A TINY BIT?
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

void obj_store_add_ref(OBJ_STORE *store, uint32 index) {
  uint8 *refs_counters = store->refs_counters;
  uint32 count = refs_counters[index] + 1;
  if (count == 256) {
    store->extra_refs[index]++;
    count -= 64;
  }
  refs_counters[index] = count;
}

void obj_store_release(OBJ_STORE *store, uint32 index) {
  uint8 *refs_counters = store->refs_counters;
  assert(refs_counters[index] > 0);
  uint32 count = refs_counters[index] - 1;
  if (count == 127) {
    if (store->extra_refs.count(index) > 0) {
      uint32 extra_count = store->extra_refs[index] - 1;
      if (extra_count == 0)
        store->extra_refs.erase(index);
      else
        store->extra_refs[index] = extra_count;
      count += 64;
    }
  }
  else if (count == 0) {
    obj_store_release_obj_at(store, index);
  }
  refs_counters[index] = count;
}

void obj_store_release(OBJ_STORE *store, uint32 index, uint32 amount) {
  uint8 *refs_counters = store->refs_counters;
  uint32 count = refs_counters[index];
  assert(count > 0);

  if (count < 128 || store->extra_refs.count(index) == 0) {
    count -= amount;
  }
  else {
    count += 64 * store->extra_refs[index];
    count -= amount;
    if (count >= 256) {
      uint32 new_extra_count = (count / 64) - 2;
      count -= 64 * new_extra_count;
      store->extra_refs[index] = new_extra_count;
    }
    else
      store->extra_refs.erase(index);
  }

  assert(count < 256);

  refs_counters[index] = count;
  if (count == 0)
    obj_store_release_obj_at(store, index);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_insert(OBJ_STORE *store, STATE_MEM_POOL *mem_pool, OBJ value, uint32 hashcode, uint32 index) {
  assert(store->first_free == index);
  assert(index < store->capacity);
  assert(is_blank(store->values[index]));
  assert(hashcode == compute_hashcode(value));

  store->count++;
  store->first_free = store->hashcode_or_next_free[index];
  store->values[index] = copy_to_pool(mem_pool, value);
  store->hashcode_or_next_free[index] = hashcode;

  obj_store_insert_into_hashtable(store, index, hashcode);
}

uint32 obj_store_insert_or_add_ref(OBJ_STORE *store, STATE_MEM_POOL *mem_pool, OBJ value) {
  uint32 surr = obj_store_value_to_surr(store, value);
  if (surr == 0xFFFFFFFF) {
    uint32 capacity = store->capacity;
    uint32 count = store->count;
    assert(count <= capacity);
    if (count == capacity)
      obj_store_resize(store, mem_pool, count + 1);
    surr = store->first_free;
    obj_store_insert(store, mem_pool, value, compute_hashcode(value), surr);
  }
  obj_store_add_ref(store, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_next_free_idx(OBJ_STORE *store, uint32 index) {
  assert(index == 0xFFFFFFFF || index >= store->capacity || is_blank(store->values[index]));

  if (index == 0xFFFFFFFF)
    return store->first_free;

  if (index >= store->capacity)
    return index + 1;

  return store->hashcode_or_next_free[index];
}

////////////////////////////////////////////////////////////////////////////////

bool obj_store_try_releasing(OBJ_STORE *store, uint32 index, uint32 amount) {
  uint8 *refs_counters = store->refs_counters;
  uint32 count = refs_counters[index];
  assert(count > 0);

  if (count < 128) {
    if (count <= amount)
      return false;
    count -= amount;
    assert(count > 0);
    refs_counters[index] = count;
    return true;
  }
  else if (store->extra_refs.count(index) > 0) {
    count += 64 * store->extra_refs[index];
    if (count <= amount)
      return false;
    count -= amount;
    assert(count > 0);
    if (count >= 256) {
      uint32 new_extra_count = (count / 64) - 2;
      count -= 64 * new_extra_count;
      store->extra_refs[index] = new_extra_count;
    }
    else
      store->extra_refs.erase(index);
    refs_counters[index] = count;
    return true;
  }
  else
    return false;
}

bool obj_store_try_releasing(OBJ_STORE *store, uint32 index) {
  return obj_store_try_releasing(store, index, 1);
}

////////////////////////////////////////////////////////////////////////////////

OBJ obj_store_surr_to_obj(void *store, uint32 surr) {
  return obj_store_surr_to_value((OBJ_STORE *) store, surr);
}
