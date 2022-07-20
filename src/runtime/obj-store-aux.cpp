#include "lib.h"


uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value, uint32 hashcode);
uint32 obj_store_next_free_surr(OBJ_STORE *store, uint32 index);

void obj_store_insert(OBJ_STORE *store, OBJ value, uint32 hashcode, uint32 surr, STATE_MEM_POOL *mem_pool);
void obj_store_resize(OBJ_STORE *store, uint32 min_capacity, STATE_MEM_POOL *mem_pool);

////////////////////////////////////////////////////////////////////////////////

static OBJ_STORE_AUX_INSERT_ENTRY *obj_store_aux_new_insert_entry_array(uint32 size) {
  return (OBJ_STORE_AUX_INSERT_ENTRY *) new_obj(null_round_up_8(size * sizeof(OBJ_STORE_AUX_INSERT_ENTRY)));
}

////////////////////////////////////////////////////////////////////////////////

void obj_store_aux_init(OBJ_STORE_AUX *store_aux) {
  store_aux->capacity = INLINE_AUX_SIZE;
  store_aux->count = 0;

  store_aux->entries = store_aux->entries_buffer;

  store_aux->hashtable = NULL;
  store_aux->buckets = NULL;

  store_aux->hash_range = 0;
  store_aux->last_surr = 0xFFFFFFFF;
}

void obj_store_aux_reset(OBJ_STORE_AUX *store_aux) {
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

static void obj_store_aux_insert_into_hashtable(OBJ_STORE_AUX *store_aux, uint32 surr, uint32 hashcode) {
  uint32 hash_idx = hashcode % store_aux->hash_range;
  store_aux->buckets[surr] = store_aux->hashtable[hash_idx];
  store_aux->hashtable[hash_idx] = surr;
}

static void obj_store_aux_resize(OBJ_STORE_AUX *store_aux) {
  uint32 capacity = store_aux->capacity;
  uint32 new_capacity = 2 * capacity;

  OBJ_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;
  OBJ_STORE_AUX_INSERT_ENTRY *new_entries = obj_store_aux_new_insert_entry_array(new_capacity);
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
    obj_store_aux_insert_into_hashtable(store_aux, i, entries[i].hashcode);
}

////////////////////////////////////////////////////////////////////////////////

void obj_store_aux_apply_insertions(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = store_aux->count;
  if (count > 0) {
    uint32 capacity = store->capacity;
    uint32 req_capacity = store->count + count;

    if (capacity < req_capacity)
      obj_store_resize(store, req_capacity, mem_pool);

    OBJ_STORE_AUX_INSERT_ENTRY *entries = store_aux->entries;
    for (uint32 i=0 ; i < count ; i++) {
      OBJ_STORE_AUX_INSERT_ENTRY entry = entries[i];
      obj_store_insert(store, entry.obj, entry.hashcode, entry.surr, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_aux_value_to_surr(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, OBJ value, uint32 hashcode) {
  uint32 surr = obj_store_value_to_surr(store, value, hashcode);
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
OBJ obj_store_aux_surr_to_value(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, uint32 surr) {
  // for (int i=0 ; i < count ; i++)
  //   if (surrogates[i] == surr)
  //     return values[i];
  // return store.surrToValue(surr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_aux_insert(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, OBJ value, uint32 hashcode) {
  uint32 capacity = store_aux->capacity;
  uint32 count = store_aux->count;
  assert(count <= capacity);

  if (count == capacity) {
    obj_store_aux_resize(store_aux);
    capacity = 2 * capacity;
    assert(capacity == store_aux->capacity);
  }

  uint32 surr = obj_store_next_free_surr(store, store_aux->last_surr);
  assert(surr <= store->capacity + count);
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

uint32 obj_store_aux_lookup_or_insert_value(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, OBJ value, STATE_MEM_POOL *mem_pool) {
  uint32 hashcode = compute_hashcode(value);
  uint32 surr = obj_store_aux_value_to_surr(store, store_aux, value, hashcode);
  if (surr != 0xFFFFFFFF)
    return surr;
  else
    return obj_store_aux_insert(store, store_aux, value, hashcode);
}
