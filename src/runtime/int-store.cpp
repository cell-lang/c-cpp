#include "lib.h"


uint64 *alloc_state_mem_uint64_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0);
  uint32 byte_size = size * null_round_up_8(sizeof(uint64));
  uint64 *ptr = (uint64 *) alloc_state_mem_block(mem_pool, byte_size);
  // memset(ptr, 0, byte_size);
  // for (uint32 i=0 ; i < size ; i++)
  //   assert(ptr[i] == 0);
  return ptr;
}

////////////////////////////////////////////////////////////////////////////////

int64 *alloc_state_mem_int64_array(STATE_MEM_POOL *mem_pool, uint32 size) {
  assert(size > 0);
  uint32 byte_size = size * null_round_up_8(sizeof(int64));
  int64 *ptr = (int64 *) alloc_state_mem_block(mem_pool, byte_size);
  // memset(ptr, 0, byte_size);
  // for (uint32 i=0 ; i < size ; i++)
  //   assert(ptr[i] == 0);
  return ptr;
}

int64 *extend_state_mem_int64_array(STATE_MEM_POOL *mem_pool, int64 *ptr, uint32 size, uint32 new_size) {
  assert(size > 0 & new_size > 0 & size % 2 == 0 & new_size % 2 == 0);
  uint32 byte_size = null_round_up_8(size * sizeof(int64));
  uint32 new_byte_size = null_round_up_8(size *sizeof(int64));
  int64 *new_ptr = (int64 *) alloc_state_mem_block(mem_pool, new_byte_size);
  memcpy(new_ptr, ptr, byte_size);
  // memset(new_ptr + size, 0, new_byte_size - byte_size);
  // for (uint32 i=0 ; i < new_size ; i++)
  //   assert(new_ptr[i] == (i < capacity ? ptr[i] : 0));
  for (uint32 i=0 ; i < size ; i++)
    assert(new_ptr[i] == ptr[i]);
  release_state_mem_block(mem_pool, ptr, byte_size);
  return new_ptr;

}

void release_state_mem_int64_array(STATE_MEM_POOL *mem_pool, int64 *ptr, uint32 size) {
  release_state_mem_block(mem_pool, ptr, size * null_round_up_8(sizeof(int64)));
}

////////////////////////////////////////////////////////////////////////////////

uint32 *alloc_state_mem_uint32_array(STATE_MEM_POOL *mem_pool, uint32 size);
void release_state_mem_uint32_array(STATE_MEM_POOL *mem_pool, uint32 *ptr, uint32 size);

////////////////////////////////////////////////////////////////////////////////

uint8 *alloc_state_mem_uint8_array(STATE_MEM_POOL *mem_pool, uint32 size);
uint8 *extend_state_mem_zeroed_uint8_array(STATE_MEM_POOL *mem_pool, uint8 *ptr, uint32 size, uint32 new_size);
void release_state_mem_uint8_array(STATE_MEM_POOL *mem_pool, uint8 *ptr, uint32 size);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const uint32 INIT_SIZE = 256;
const uint32 INV_IDX   = 0x3FFFFFFF;

const uint32 HASHTABLE_SCALE = 4;

////////////////////////////////////////////////////////////////////////////////

inline uint32 hash_index(int64 value, uint32 capacity) {
  uint32 hashcode = (uint32) (value ^ (value >> 32));
  return hashcode % (capacity / HASHTABLE_SCALE);
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_empty(uint64 slot) {
  uint32 tag = (uint32) (slot >> 62);
  assert(tag == 0 | tag == 1 | tag == 2);
  return tag == 2;
  // return (slot >>> 62) == 2;
}

inline uint64 get_value(uint64 slot, INT_STORE *store) {
  assert(!is_empty(slot));
  uint32 tag = (uint32) (slot >> 62);
  assert(tag == 0 | tag == 1);
  uint32 lsword = (uint32) (slot & 0xFFFFFFFF);
  return tag == 0 ? (int32) lsword : store->large_ints[lsword];
}

inline uint32 get_next_in_bucket(uint64 slot) {
  assert(!is_empty(slot));
  return (uint32) ((slot >> 32) & 0x3FFFFFFF);
}

inline uint32 get_next_free(uint64 slot) {
  assert(is_empty(slot));
  return (uint32) ((slot >> 32) & 0x3FFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////

inline uint64 empty_slot(uint32 next) {
  assert(next <= 0x1FFFFFFF);
  return (((uint64) next) | (2ULL << 30)) << 32;
}

inline uint64 filled_value_slot(int32 value, uint32 next) {
  uint32 unsigned_value = (uint32) value;
  uint64 slot = ((uint64) unsigned_value) | (((uint64) next) << 32);
  assert(!is_empty(slot));
  assert(get_value(slot, NULL) == value);
  assert(get_next_in_bucket(slot) == next);
  return slot;
}

inline uint64 filled_idx_slot(uint32 idx, uint32 next) {
  uint64 slot = ((uint64) idx) | (((uint64) next) << 32) | (1L << 62);
  assert(!is_empty(slot));
  // assert(get_value(slot) == large_ints[idx]); //## THIS CHECK SHOULD BE REPLACED, NOT ELIMINATED
  assert(get_next_in_bucket(slot) == next);
  return slot;
}

inline uint64 reindexed_slot(uint64 slot, uint32 next) {
  uint32 tag = (uint32) (slot >> 62);
  assert(tag == 0 | tag == 1);
  return tag == 0 ? filled_value_slot((uint32) slot, next) : filled_idx_slot((uint32) slot, next);
}

////////////////////////////////////////////////////////////////////////////////

static void int_store_release_slot(INT_STORE *store, uint32 index) {
  uint64 *slots = store->slots;
  uint32 *hashtable = store->hashtable;

  uint64 slot = slots[index];
  uint32 hash_idx = hash_index(get_value(slot, store), store->capacity);

  uint32 idx = hashtable[hash_idx];
  assert(idx != INV_IDX);

  if (idx == index) {
    hashtable[hash_idx] = get_next_in_bucket(slot);
  }
  else {
    for ( ; ; ) {
      slot = slots[idx];
      uint32 next_idx = get_next_in_bucket(slot);
      if (next_idx == index) {
        slots[idx] = reindexed_slot(slot, get_next_in_bucket(slots[next_idx]));
        break;
      }
      idx = next_idx;
    }
  }

  slots[index] = empty_slot(store->first_free);
  store->first_free = index;
  store->count--;

  // protected void free(int index) {
  //   long slot = slots[index];
  //   int hashIdx = hashIdx(value(slot));

  //   int idx = hashtable[hashIdx];
  //   Miscellanea._assert(idx != INV_IDX);

  //   if (idx == index) {
  //     hashtable[hashIdx] = next(slot);
  //   }
  //   else {
  //     for ( ; ; ) {
  //       slot = slots[idx];
  //       int next = next(slot);
  //       if (next == index) {
  //         slots[idx] = reindexedSlot(slot, next(slots[next]));
  //         break;
  //       }
  //       idx = next;
  //     }
  //   }

  //   slots[index] = emptySlot(firstFree);
  //   firstFree = index;
  //   count--;
  // }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void int_store_resize(INT_STORE *store, STATE_MEM_POOL *mem_pool, uint32 min_capacity) {
  assert(min_capacity > store->capacity);

  uint32 curr_capacity = store->capacity;
  uint32 new_capacity = 2 * curr_capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;
  store->capacity = new_capacity;

  uint64 *curr_slots = store->slots; //## REMEMBER TO RELEASE
  uint64 *new_slots = alloc_state_mem_uint64_array(mem_pool, new_capacity);
  store->slots = new_slots;

  uint32 *curr_htable = store->hashtable; //## REMEMBER TO RELEASE
  uint32 new_htable_size = new_capacity / HASHTABLE_SCALE;
  uint32 *new_htable = alloc_state_mem_uint32_array(mem_pool, new_htable_size);
  store->hashtable = new_htable;

  for (uint32 i=0 ; i < new_htable_size ; i++)
    new_htable[i] = INV_IDX;

  for (uint32 i=0 ; i < curr_capacity ; i++) {
    uint64 slot = curr_slots[i];
    uint32 hash_idx = hash_index(get_value(slot, store), new_capacity);
    new_slots[i] = reindexed_slot(slot, new_htable[hash_idx]);
    new_htable[hash_idx] = i;
  }

  for (uint32 i=curr_capacity ; i < new_capacity ; i++)
    new_slots[i] = empty_slot(i + 1);

  uint8 *curr_refs_counters = store->refs_counters; //## REMEMBER TO RELEASE
  uint8 *new_refs_counters = extend_state_mem_zeroed_uint8_array(mem_pool, curr_refs_counters, curr_capacity, new_capacity);
  store->refs_counters = new_refs_counters;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void int_store_init(INT_STORE *store, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_SIZE = 256;

  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, INIT_SIZE);
  store->slots = slots;
  for (int i=0 ; i < INIT_SIZE ; i++)
    slots[i] = empty_slot(i + 1);
  for (int i=0 ; i < INIT_SIZE ; i++)
    assert(is_empty(slots[i]));

  uint32 *hashtable = alloc_state_mem_uint32_array(mem_pool, INIT_SIZE / HASHTABLE_SCALE);
  store->hashtable = hashtable;
  for (uint32 i=0 ; i < INIT_SIZE / HASHTABLE_SCALE ; i++)
    hashtable[i] = INV_IDX;

  uint8 *refs_counters = alloc_state_mem_uint8_array(mem_pool, INIT_SIZE);
  store->refs_counters = refs_counters;
  memset(refs_counters, 0, INIT_SIZE * sizeof(uint8));

  store->capacity = INIT_SIZE;
  store->count = 0;
  store->first_free = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 int_store_value_to_surr(INT_STORE *store, int64 value) {
  uint32 hash_idx = hash_index(value, store->capacity);
  uint32 idx = store->hashtable[hash_idx];
  if (idx == INV_IDX)
    return 0xFFFFFFFF;
  uint64 *slots = store->slots;
  do {
    uint64 slot = slots[idx];
    if (get_value(slot, store) == value) //## WE CAN PROBABLY IMPROVE HERE: value_is(slot, value, store)
      return idx;
    idx = get_next_in_bucket(slot);
  } while (idx != INV_IDX);
  return 0xFFFFFFFF;
}

int64 int_store_surr_to_value(INT_STORE *store, uint32 surr) {
  return get_value(store->slots[surr], store);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void int_store_add_ref(INT_STORE *store, uint32 index) {
  uint8 *refs_counters = store->refs_counters;
  uint32 count = refs_counters[index] + 1;
  if (count == 256) {
    store->extra_refs[index]++;
    count -= 64;
  }
  refs_counters[index] = count;
}

void int_store_release(INT_STORE *store, uint32 index) {
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
  else if (count == 0)
    int_store_release_slot(store, index);
  refs_counters[index] = count;
}

void int_store_release(INT_STORE *store, uint32 index, uint32 amount) {
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
    int_store_release_slot(store, index);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// struct INT_STORE {
//   // Bits  0 - 31: 32-bit value, or index of 64-bit value
//   // Bits 32 - 61: index of next value in the bucket if used or next free index otherwise
//   // Bits 62 - 63: tag: 00 used (32 bit), 01 used (64 bit), 10 free
//   uint64 *slots;

//   unordered_map<uint32, int64> large_ints;

//   // INV_IDX when there's no value in that bucket
//   uint32 *hashtable;

//   uint8  *refs_counters;
//   unordered_map<uint32, uint32> extra_refs;

//   uint32 capacity;
//   uint32 count;
//   uint32 first_free;
// };


void int_store_insert(INT_STORE *store, STATE_MEM_POOL *mem_pool, int64 value, uint32 index) {
  assert(store->first_free == index);
  assert(index < store->capacity);
  assert(is_empty(store->slots[index]));
  assert(store->refs_counters[index] == 0);

  uint64 *slots = store->slots;
  uint32 *hashtable = store->hashtable;

  uint64 slot = slots[index];

  uint32 hash_idx = hash_index(value, store->capacity);
  uint32 head = hashtable[hash_idx];

  hashtable[hash_idx] = index;
  store->count++;
  store->first_free = get_next_free(slot);

  if (value == (int32) value) {
    slots[index] = filled_value_slot((int32) value, head);
  }
  else {
    //## IMPLEMENT IMPLEMENT IMPLEMENT
    //## int idx64 = largeInts.insert(value); <-- THIS WAS THE JAVA VERSION
    store->large_ints[index] = value;
    uint32 idx64 = index;
    slots[index] = filled_idx_slot(idx64, head);
  }

  assert(!is_empty(slots[index]));
  assert(get_value(slots[index], store) == value);
}

uint32 int_store_insert_or_add_ref(INT_STORE *store, STATE_MEM_POOL *mem_pool, int64 value) {
  uint32 surr = int_store_value_to_surr(store, value);
  if (surr == 0xFFFFFFFF) {
    uint32 capacity = store->capacity;
    uint32 count = store->count;
    assert(count <= capacity);
    if (count == capacity)
      int_store_resize(store, mem_pool, count + 1);
    surr = store->first_free;
    int_store_insert(store, mem_pool, value, surr);
  }
  int_store_add_ref(store, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 int_store_next_free_idx(INT_STORE *store, uint32 index) {
  assert(index == 0xFFFFFFFF || index >= store->capacity || is_empty(store->slots[index]));

  if (index == 0xFFFFFFFF)
    return store->first_free;

  if (index >= store->capacity)
    return index + 1;

  return get_next_free(store->slots[index]);
}

////////////////////////////////////////////////////////////////////////////////

bool int_store_try_releasing(INT_STORE *store, uint32 index, uint32 amount) {
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

bool int_store_try_releasing(INT_STORE *store, uint32 index) {
  return int_store_try_releasing(store, index, 1);
}

////////////////////////////////////////////////////////////////////////////////

OBJ int_store_surr_to_obj(void *store, uint32 surr) {
  return make_int(int_store_surr_to_value((INT_STORE *) store, surr));
}
