#include "lib.h"


// int64 int_store_surr_to_value(INT_STORE *store, uint32 surr);
uint32 int_store_next_free_surr(INT_STORE *, uint32 index);

void int_store_insert(INT_STORE *, int64 value, uint32 index, STATE_MEM_POOL *);
void int_store_resize(INT_STORE *, uint32 min_capacity, STATE_MEM_POOL *);

////////////////////////////////////////////////////////////////////////////////

static INT_STORE_AUX_INSERT_ENTRY *new_insert_entry_array(uint32 size) {
  return (INT_STORE_AUX_INSERT_ENTRY *) new_obj(null_round_up_8(size * sizeof(INT_STORE_AUX_INSERT_ENTRY)));
}

////////////////////////////////////////////////////////////////////////////////

static uint32 quick_hashcode(int64 value) {
  uint32 hashcode = (uint32) (value ^ (value >> 32));
  return hashcode;
}

////////////////////////////////////////////////////////////////////////////////

void int_store_aux_init(INT_STORE_AUX *store_aux) {
  store_aux->capacity = INLINE_AUX_SIZE;
  store_aux->count = 0;

  store_aux->entries = store_aux->entries_buffer;

  store_aux->hashtable = NULL;
  store_aux->buckets = NULL;

  store_aux->hash_range = 0;
  store_aux->last_surr = 0xFFFFFFFF;
}

void int_store_aux_reset(INT_STORE_AUX *store_aux) {
  uint32 count = store_aux->count;
  if (count > 0) {
    assert(store_aux->last_surr != 0xFFFFFFFF);

    store_aux->count = 0;
    store_aux->last_surr = 0xFFFFFFFF;

    if (count > INLINE_AUX_SIZE) {
      store_aux->capacity = INLINE_AUX_SIZE;

      store_aux->entries = store_aux->entries_buffer;

      store_aux->hashtable = NULL;
      store_aux->buckets = NULL;

      store_aux->hash_range = 0;
    }
  }

  assert(store_aux->capacity == INLINE_AUX_SIZE);
  assert(store_aux->count == 0);

  assert(store_aux->entries == store_aux->entries_buffer);

  assert(store_aux->hashtable == NULL);
  assert(store_aux->buckets == NULL);

  assert(store_aux->hash_range == 0);
  assert(store_aux->last_surr == 0xFFFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////

static void int_store_aux_insert_into_hashtable(INT_STORE_AUX *store_aux, uint32 surr, uint32 hashcode) {
  uint32 hash_idx = hashcode % store_aux->hash_range;
  store_aux->buckets[surr] = store_aux->hashtable[hash_idx];
  store_aux->hashtable[hash_idx] = surr;
}

static void int_store_aux_resize(INT_STORE_AUX *store_aux) {
  uint32 capacity = store_aux->capacity;
  uint32 new_capacity = 2 * capacity;

  INT_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;
  INT_STORE_AUX_INSERT_ENTRY *new_entries = new_insert_entry_array(new_capacity);
  memcpy(new_entries, entries, capacity * sizeof(INT_STORE_AUX_INSERT_ENTRY));

  store_aux->capacity = new_capacity;
  store_aux->entries = new_entries;

  uint32 *hashtable = new_uint32_array(capacity); // capacity == new_capacity / 2
  memset(hashtable, 0xFF, capacity * sizeof(uint32));
#ifndef NDEBUG
  for (uint32 i=0 ; i < capacity ; i++)
    assert(hashtable[i] == 0xFFFFFFFF);
#endif

  uint32 *buckets = new_uint32_array(new_capacity);  // new_capacity == 2 * capacity
  memset(buckets, 0xFF, new_capacity * sizeof(uint32));
#ifndef NDEBUG
  for (uint32 i=0 ; i < new_capacity ; i++)
    assert(buckets[i] == 0xFFFFFFFF);
#endif
  store_aux->buckets = buckets;

  store_aux->hash_range = capacity; // capacity == new_capacity / 2
  store_aux->hashtable = hashtable;

  assert(store_aux->count == capacity);
  for (uint32 i=0 ; i < capacity ; i++)
    int_store_aux_insert_into_hashtable(store_aux, i, entries[i].hashcode);
}

////////////////////////////////////////////////////////////////////////////////

void int_store_aux_apply_insertions(INT_STORE *store, INT_STORE_AUX *store_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = store_aux->count;
  if (count > 0) {
    uint32 capacity = store->capacity;
    uint32 req_capacity = store->count + count;

    if (capacity < req_capacity)
      int_store_resize(store, req_capacity, mem_pool);

    INT_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;
    for (uint32 i=0 ; i < count ; i++) {
      INT_STORE_AUX_INSERT_ENTRY entry = entries[i];
      int_store_insert(store, entry.value, entry.surr, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 int_store_aux_value_to_surr(INT_STORE *store, INT_STORE_AUX *store_aux, int64 value) {
  uint32 hashcode = quick_hashcode(value);
  uint32 surr = int_store_value_to_surr(store, value);
  if (surr != 0xFFFFFFFF)
    return surr;

  uint32 count = store_aux->count;
  if (count > 0) {
    uint32 hash_range = store_aux->hash_range;

    INT_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;

    if (hash_range == 0) {
      for (uint32 i=0 ; i < count ; i++) {
        INT_STORE_AUX_INSERT_ENTRY *entry = entries + i;
        if (hashcode == entry->hashcode && value == entry->value)
          return entry->surr;
      }
    }
    else {
      uint32 *hashtable = store_aux->hashtable;
      uint32 *buckets = store_aux->buckets;

      uint32 hash_idx = hashcode % hash_range;
      uint32 idx = hashtable[hash_idx];
      while (idx != 0xFFFFFFFF) {
        INT_STORE_AUX_INSERT_ENTRY *entry = entries + idx;
        if (value == entry->value)
          return entry->surr;
        idx = buckets[idx];
      }

#ifndef NDEBUG
      for (uint32 i=0 ; i < count ; i++)
        assert(entries[i].value != value);
#endif
    }
  }

  return 0xFFFFFFFF;
}

// Inefficient, but used only for debugging
int64 int_store_aux_surr_to_value(INT_STORE *store, INT_STORE_AUX *store_aux, uint32 surr) {
  // for (int i=0 ; i < count ; i++)
  //   if (surrogates[i] == surr)
  //     return values[i];
  // return store.surrToValue(surr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 int_store_aux_insert(INT_STORE *store, INT_STORE_AUX *store_aux, int64 value) {
  uint32 hashcode = quick_hashcode(value);

  uint32 capacity = store_aux->capacity;
  uint32 count = store_aux->count;
  assert(count <= capacity);

  if (count == capacity) {
    int_store_aux_resize(store_aux);
    capacity = 2 * capacity;
    assert(capacity == store_aux->capacity);
  }

  uint32 surr = int_store_next_free_surr(store, store_aux->last_surr);
  store_aux->last_surr = surr;

  INT_STORE_AUX_INSERT_ENTRY *entry_ptr = store_aux->entries + count;
  entry_ptr->value = value;
  //## THESE TWO WRITES SHOULD BE MERGED
  entry_ptr->hashcode = hashcode;
  entry_ptr->surr = surr;
  store_aux->count = count + 1;

  if (count > INLINE_AUX_SIZE) {
    int_store_aux_insert_into_hashtable(store_aux, count, hashcode);
  }
  // if (count >= 16) {
  //   if (count >= hashRange) {
  //     if (hashRange != 0) {
  //       Array.fill(hashtable, hashRange, -1);
  //       hashRange *= 2;
  //     }
  //     else
  //       hashRange = 16;

  //     for (int i=0 ; i < count ; i++)
  //       insertIntoHashtable(i, hashcodes[i]);
  //   }
  //   insertIntoHashtable(count, hashcode);
  // }
  // count++;

  return surr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 int_store_aux_lookup_or_insert_value(INT_STORE *store, INT_STORE_AUX *store_aux, int64 value, STATE_MEM_POOL *mem_pool) {
  uint32 surr = int_store_aux_value_to_surr(store, store_aux, value);
  if (surr != 0xFFFFFFFF)
    return surr;
  else
    return int_store_aux_insert(store, store_aux, value);
}
