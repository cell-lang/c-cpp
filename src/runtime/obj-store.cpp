#include "lib.h"

bool is_store_blank(OBJ obj) {
  return obj.extra_data == 0;
}

OBJ make_store_blank(uint32 index_next) {
  OBJ obj;
  obj.core_data.int_ = index_next;
  obj.extra_data = 0;
  return obj;
}

uint32 index_next(OBJ *slot_ptr) {
  return slot_ptr->core_data.int_;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_resize(OBJ_STORE *store, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  assert(min_capacity > store->capacity);

  uint32 capacity = store->capacity;
  uint32 new_capacity = 2 * capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;

  OBJ *slots = extend_state_mem_obj_array(mem_pool, store->slots, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    slots[i] = make_store_blank(i + 1);
  store->slots = slots;

  quasi_map_hcode_surr_resize(&store->hashtable, new_capacity, mem_pool);

  store->capacity = new_capacity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_init(OBJ_STORE *store, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_SIZE = 256;

  OBJ *slots = alloc_state_mem_obj_array(mem_pool, INIT_SIZE);
  for (uint32 i=0 ; i < INIT_SIZE ; i++)
    slots[i] = make_store_blank(i + 1);
  store->slots = slots;

  quasi_map_hcode_surr_init(&store->hashtable, mem_pool);

  store->capacity = INIT_SIZE;
  store->count = 0;
  store->first_free_surr = 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value, uint32 hashcode) {
  return quasi_map_hcode_surr_find(&store->hashtable, hashcode, store->slots, value);
}

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value) {
  uint32 hashcode = compute_hashcode(value);
  return obj_store_value_to_surr(store, value, hashcode);
}

OBJ obj_store_surr_to_value(OBJ_STORE *store, uint32 surr) {
  assert(surr < store->capacity);
  assert(!is_store_blank(store->slots[surr]));

  return store->slots[surr];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void obj_store_clear(OBJ_STORE *store, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = store->capacity;

  OBJ *slots = store->slots;
  for (uint32 i=0 ; i < capacity ; i++)
    //## BUG BUG BUG: AND WHEN IS THE MEMORY FREED?
    slots[i] = make_store_blank(i + 1);

  quasi_map_hcode_surr_clear(&store->hashtable);

  store->count = 0;
  store->first_free_surr = 0;
}

void obj_store_remove(OBJ_STORE *store, uint32 surr, STATE_MEM_POOL *mem_pool) {
  assert(surr < store->capacity && !is_store_blank(store->slots[surr]));

  OBJ *slot_ptr = store->slots + surr;
  OBJ value = *slot_ptr;

  uint32 hashcode = compute_hashcode(value); //## BAD BAD BAD: SHOULD THE HASHCODE BE STORED, INSTEAD OF BEING RECOMPUTED?
  quasi_map_hcode_surr_delete(&store->hashtable, hashcode, surr);

  remove_from_pool(mem_pool, value);

  *slot_ptr = make_store_blank(store->first_free_surr);
  store->first_free_surr = surr;
  store->count--;
}

////////////////////////////////////////////////////////////////////////////////

void obj_store_insert(OBJ_STORE *store, OBJ value, uint32 hashcode, uint32 surr, STATE_MEM_POOL *mem_pool) {
  assert(store->first_free_surr == surr);
  assert(surr < store->capacity);
  assert(is_store_blank(store->slots[surr]));
  assert(hashcode == compute_hashcode(value));

  store->count++;
  OBJ *slot_ptr = store->slots + surr;
  store->first_free_surr = index_next(slot_ptr);
  *slot_ptr = copy_to_pool(mem_pool, value);

  quasi_map_hcode_surr_insert(&store->hashtable, hashcode, surr, mem_pool);
}

static uint32 obj_store_insert(OBJ_STORE *store, OBJ value, uint32 hashcode, STATE_MEM_POOL *mem_pool) {
  assert(obj_store_value_to_surr(store, value) == 0xFFFFFFFF);
  assert(compute_hashcode(value) == hashcode);

  uint32 capacity = store->capacity;
  uint32 count = store->count;
  assert(count <= capacity);
  if (count == capacity)
    obj_store_resize(store, count + 1, mem_pool);
  uint32 surr = store->first_free_surr;
  obj_store_insert(store, value, hashcode, surr, mem_pool);
  return surr;
}

uint32 obj_store_lookup_or_insert(OBJ_STORE *store, OBJ value, STATE_MEM_POOL *mem_pool) {
  uint32 hashcode = compute_hashcode(value);
  uint32 surr = obj_store_value_to_surr(store, value, hashcode);
  if (surr == 0xFFFFFFFF)
    surr = obj_store_insert(store, value, hashcode, mem_pool);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 obj_store_next_free_surr(OBJ_STORE *store, uint32 last_surr) {
  assert(last_surr == 0xFFFFFFFF || last_surr >= store->capacity || is_store_blank(store->slots[last_surr]));

  if (last_surr == 0xFFFFFFFF)
    return store->first_free_surr;

  if (last_surr >= store->capacity)
    return last_surr + 1;

  return index_next(store->slots + last_surr);
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
    assert(surr < store->capacity && !is_store_blank(store->slots[surr]));
    obj_store_remove(store, surr, mem_pool);
  }
  else
    obj_store_clear(store, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// #ifndef NDEBUG
// void obj_store_print_collisions_histogram(OBJ_STORE *store) {
//   unordered_map<uint32, uint32> counters;

//   for (uint32 i=0 ; i < store->capacity ; i++) {
//     OBJ value = store->slots[i];
//     if (!is_store_blank(value)) {
//       uint32 hashcode = compute_hashcode(value);
//       counters[hashcode]++;
//     }
//   }

//   uint32 histogram[9];
//   for (uint32 i=0 ; i < 9 ; i++)
//     histogram[i] = 0;

//   for (auto it=counters.begin() ; it != counters.end() ; it++) {
//     uint32 count = it->second;
//     assert(count > 0);
//     uint32 index = count - 1;
//     if (index > 8)
//       index = 8;
//     histogram[index]++;
//   }

//   printf("  ");
//   for (uint32 i=0 ; i < 9 ; i++)
//     printf("%8d", histogram[i]);
//   puts("");
// }
// #endif
