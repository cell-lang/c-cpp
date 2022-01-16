#include "lib.h"



OBJ_STORE_AUX_BATCH_RELEASE_ENTRY *new_batch_release_entry_array(uint32 size) {
  return (OBJ_STORE_AUX_BATCH_RELEASE_ENTRY *) new_obj(null_round_up_8(size * sizeof(OBJ_STORE_AUX_BATCH_RELEASE_ENTRY)));
}

OBJ_STORE_AUX_INSERT_ENTRY *new_insert_entry_array(uint32 size) {
  return (OBJ_STORE_AUX_INSERT_ENTRY *) new_obj(null_round_up_8(size * sizeof(OBJ_STORE_AUX_INSERT_ENTRY)));
}

////////////////////////////////////////////////////////////////////////////////

void init_obj_store_aux(OBJ_STORE_AUX *store_aux) {
  store_aux->deferred_capacity = INLINE_AUX_SIZE;
  store_aux->deferred_count = 0;
  store_aux->deferred_release_surrs = store_aux->deferred_release_buffer;

  store_aux->batch_deferred_capacity = INLINE_AUX_SIZE;
  store_aux->batch_deferred_count = 0;
  store_aux->batch_deferred_release_entries = store_aux->batch_deferred_release_buffer;

  store_aux->capacity = INLINE_AUX_SIZE;
  store_aux->count = 0;

  store_aux->entries = store_aux->entries_buffer;

  store_aux->hashtable = NULL;
  store_aux->buckets = NULL;

  store_aux->hash_range = 0;
  store_aux->last_surr = 0xFFFFFFFF;
}

////////////////////////////////////////////////////////////////////////////////

static void insert_into_hashtable(OBJ_STORE_AUX *store_aux, uint32 surr, uint32 hashcode) {
  uint32 hash_idx = hashcode % store_aux->hash_range;
  store_aux->buckets[surr] = store_aux->hashtable[hash_idx];
  store_aux->hashtable[hash_idx] = surr;
}

static void resize(OBJ_STORE_AUX *store_aux) {
  uint32 capacity = store_aux->capacity;
  uint32 new_capacity = 2 * capacity;

  OBJ_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;
  OBJ_STORE_AUX_INSERT_ENTRY *new_entries = new_insert_entry_array(new_capacity);
  memcpy(new_entries, entries, capacity * sizeof(OBJ_STORE_AUX_INSERT_ENTRY));

  store_aux->capacity = new_capacity;
  store_aux->entries = new_entries;

  uint32 *hashtable = new_uint32_array(capacity); // capacity == new_capacity / 2
  memset(hashtable, 0xFF, capacity * sizeof(uint32));
  for (uint32 i=0 ; i < capacity ; i++)
    assert(hashtable[i] == 0xFFFFFFFF);

  store_aux->hash_range = capacity; // capacity == new_capacity / 2
  store_aux->hashtable = hashtable;
  store_aux->buckets = new_uint32_array(new_capacity);

  assert(store_aux->count == capacity);
  for (uint32 i=0 ; i < capacity ; i++)
    insert_into_hashtable(store_aux, i, entries[i].hashcode);
}

////////////////////////////////////////////////////////////////////////////////

void mark_for_deferred_release(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, uint32 surr) {
  if (!try_releasing(store, surr)) { //## THIS DOESN'T ACTUALLY RELEASE THE MEMORY IF THE REFERENCE COUNT DROPS TO ZERO
    uint32 capacity = store_aux->deferred_capacity;
    uint32 count = store_aux->deferred_count;
    uint32 *surrs = store_aux->deferred_release_surrs;
    assert(count <= capacity);
    if (count == capacity) {
      uint32 *new_surrs = new_uint32_array(2 * capacity);
      memcpy(new_surrs, surrs, capacity * sizeof(uint32));
      capacity = 2 * capacity;
      store_aux->deferred_capacity = capacity;
    }
    surrs[count++] = surr;
    store_aux->deferred_count = count;
  }
}

void mark_for_batch_deferred_release(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, uint32 surr, uint32 count) {
  if (!try_releasing(store, surr, count)) { //## THIS DOESN'T ACTUALLY RELEASE THE MEMORY IF THE REFERENCE COUNT DROPS TO ZERO
    uint32 capacity = store_aux->batch_deferred_capacity;
    uint32 count = store_aux->batch_deferred_count;
    OBJ_STORE_AUX_BATCH_RELEASE_ENTRY *entries = store_aux->batch_deferred_release_entries;
    assert(count <= capacity);
    if (count == capacity) {
      OBJ_STORE_AUX_BATCH_RELEASE_ENTRY *new_entries = new_batch_release_entry_array(2 * capacity);
      memcpy(new_entries, entries, capacity * sizeof(uint32));
      store_aux->batch_deferred_capacity = 2 * capacity;
      store_aux->batch_deferred_release_entries = new_entries;
      entries = new_entries;
    }
    OBJ_STORE_AUX_BATCH_RELEASE_ENTRY entry;
    entry.surr = surr;
    entry.count = count;
    entries[count++] = entry;
    store_aux->batch_deferred_count = count;
  }
}

void apply_deferred_releases(OBJ_STORE *store, OBJ_STORE_AUX *store_aux) {
  uint32 count = store_aux->deferred_count;
  if (count > 0) {
    uint32 *surrs = store_aux->deferred_release_surrs;
    for (uint32 i=0 ; i < count ; i++)
      release(store, surrs[i]);

    store_aux->deferred_count = 0;
    if (count > INLINE_AUX_SIZE) {
      store_aux->deferred_capacity = INLINE_AUX_SIZE;
      store_aux->deferred_release_surrs = store_aux->deferred_release_buffer;
    }
  }

  count = store_aux->batch_deferred_count;
  if (count > 0) {
    OBJ_STORE_AUX_BATCH_RELEASE_ENTRY *entries = store_aux->batch_deferred_release_entries;
    for (uint32 i=0 ; i < count ; i++) {
      OBJ_STORE_AUX_BATCH_RELEASE_ENTRY entry = entries[i];
      release(store, entry.surr, entry.count);
    }

    store_aux->batch_deferred_count = 0;
    if (count > INLINE_AUX_SIZE) {
      store_aux->deferred_capacity = INLINE_AUX_SIZE;
      store_aux->batch_deferred_release_entries = store_aux->batch_deferred_release_buffer;
    }
  }
}

void obj_store_apply(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = store_aux->count;
  if (count > 0) {
    uint32 capacity = store->capacity;
    uint32 req_capacity = store->count + count;

    if (capacity < req_capacity)
      resize(store, mem_pool, req_capacity);

    OBJ_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;
    for (uint32 i=0 ; i < count ; i++) {
      OBJ_STORE_AUX_INSERT_ENTRY entry = entries[i];
      insert(store, mem_pool, entry.obj, entry.hashcode, entry.surr);
    }
  }
}

void obj_store_reset(OBJ_STORE_AUX *store_aux) {
  uint32 count = store_aux->count;
  if (count > 0) {
    store_aux->count = 0;

    if (count > INLINE_AUX_SIZE) {
      store_aux->capacity = INLINE_AUX_SIZE;

      store_aux->entries = store_aux->entries_buffer;

      store_aux->hashtable = NULL;
      store_aux->buckets = NULL;

      store_aux->hash_range = 0;
      store_aux->last_surr = 0xFFFFFFFF;
    }
  }

  assert(store_aux->deferred_capacity == INLINE_AUX_SIZE);
  assert(store_aux->deferred_count == 0);
  assert(store_aux->deferred_release_surrs == store_aux->deferred_release_buffer);

  assert(store_aux->batch_deferred_capacity == INLINE_AUX_SIZE);
  assert(store_aux->batch_deferred_count == 0);
  assert(store_aux->batch_deferred_release_entries == store_aux->batch_deferred_release_buffer);

  assert(store_aux->capacity == INLINE_AUX_SIZE);
  assert(store_aux->count == 0);

  assert(store_aux->entries == store_aux->entries_buffer);

  assert(store_aux->hashtable == NULL);
  assert(store_aux->buckets == NULL);

  assert(store_aux->hash_range == 0);
  assert(store_aux->last_surr == 0xFFFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 value_to_surr(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, OBJ value) {
  uint32 hashcode = compute_hashcode(value);
  uint32 surr = value_to_surr(store, value, hashcode);
  if (surr != 0xFFFFFFFF)
    return surr;

  uint32 count = store_aux->count;
  if (count > 0) {
    uint32 hash_range = store_aux->hash_range;

    OBJ_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;

    if (hash_range == 0) {
      for (uint32 i=0 ; i < count ; i++) {
        OBJ_STORE_AUX_INSERT_ENTRY *entry = entries + i;
        if (hashcode == entry->hashcode && are_eq(value, entry->obj))
          return entry->surr;
      }
    }
    else {
      uint32 *hashtable = store_aux->hashtable;
      uint32 *buckets = store_aux->buckets;

      uint32 hash_idx = hashcode % hash_range;
      uint32 idx = hashtable[hash_idx];
      while (idx != 0xFFFFFFFF) {
        OBJ_STORE_AUX_INSERT_ENTRY *entry = entries + idx;
        if (hashcode == entry->hashcode && are_eq(value, entry->obj))
          return entry->surr;
        idx = buckets[idx];
      }
    }
  }

  return 0xFFFFFFFF;
}

// Inefficient, but used only for debugging
OBJ surr_to_value(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, uint32 surr) {
  // for (int i=0 ; i < count ; i++)
  //   if (surrogates[i] == surr)
  //     return values[i];
  // return store.surrToValue(surr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 insert(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, OBJ value) {
  uint32 hashcode = compute_hashcode(value);

  uint32 capacity = store_aux->capacity;
  uint32 count = store_aux->count;
  assert(count <= capacity);

  if (count == capacity) {
    resize(store_aux);
    capacity = 2 * capacity;
    assert(capacity == store_aux->capacity);
  }

  uint32 surr = next_free_idx(store, store_aux->last_surr);
  store_aux->last_surr = surr;

  OBJ_STORE_AUX_INSERT_ENTRY *entry_ptr = store_aux->entries + count;
  entry_ptr->obj = value;
  //## THESE TWO WRITES SHOULD BE MERGED
  entry_ptr->hashcode = hashcode;
  entry_ptr->surr = surr;
  store_aux->count = ++count;

  if (count > INLINE_AUX_SIZE) {

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

uint32 lookup_or_insert_value(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, STATE_MEM_POOL *mem_pool, OBJ value) {
  uint32 surr = value_to_surr(store, store_aux, value);
  if (surr != 0xFFFFFFFF)
    return surr;
  else
    return insert(store, store_aux, value);
}
