#include "lib.h"



typedef struct {
  void *ptr;
  OBJ obj;
} VOID_PTR_OBJ_HASHTABLE_ENTRY;

typedef struct {
  VOID_PTR_OBJ_HASHTABLE_ENTRY *entries;
  uint32 capacity;
  uint32 count;
} VOID_PTR_OBJ_HASHTABLE;



inline uint32 ptr_hashcode(void *ptr) {
  uint64 lword = (uint64) ptr;
  return (uint32) (lword ^ (lword >> 32));
}

inline uint32 hashtable_step(uint32 hashcode) {
  // Using an odd step size should guarantee that all slots are eventually visited
  //## I'M STILL NOT ENTIRELY SURE ABOUT THIS. CHECK!
  return (hashcode & 0xF) | 1;
}

VOID_PTR_OBJ_HASHTABLE_ENTRY *void_ptr_obj_hashtable_lookup(VOID_PTR_OBJ_HASHTABLE *hashtable, void *ptr) {
  uint32 hcode = ptr_hashcode(ptr);
  uint32 step = hashtable_step(hcode);

  VOID_PTR_OBJ_HASHTABLE_ENTRY *entries = hashtable->entries;
  uint32 capacity = hashtable->capacity;
  if (capacity == 0)
    return NULL;

  uint32 idx = hcode % capacity;
  for ( ; ; ) {
    VOID_PTR_OBJ_HASHTABLE_ENTRY *entry = entries + idx;
    void *entry_ptr = entry->ptr;
    if (entry_ptr == NULL)
      return NULL;
    if (entry_ptr == ptr)
      return entry;
    idx = (idx + step) % capacity;
  }
}

void void_ptr_obj_hashtable_insert_unsafe(VOID_PTR_OBJ_HASHTABLE *hashtable, void *ptr, OBJ obj) {
  // Assuming the hashtable is not full and the symbol has not been inserted yet
  assert(hashtable->count < hashtable->capacity);
  assert(void_ptr_obj_hashtable_lookup(hashtable, ptr) == NULL);

  uint32 hcode = ptr_hashcode(ptr);
  uint32 step = hashtable_step(hcode);

  VOID_PTR_OBJ_HASHTABLE_ENTRY *entries = hashtable->entries;
  uint32 capacity = hashtable->capacity;
  hashtable->count++;

  uint32 idx = hcode % capacity;
  for ( ; ; ) {
    VOID_PTR_OBJ_HASHTABLE_ENTRY *entry = entries + idx;
    if (entry->ptr == NULL) {
      entry->ptr = ptr;
      entry->obj = obj;
      return;
    }
    idx = (idx + step) % capacity;
  }
}

void void_ptr_obj_hashtable_insert(VOID_PTR_OBJ_HASHTABLE *hashtable, void *ptr, OBJ obj) {
  // Assuming the symbol is not there yet
  assert(void_ptr_obj_hashtable_lookup(hashtable, ptr) == NULL);

  if (hashtable->entries == NULL) {
    uint32 mem_size = 1024 * sizeof(VOID_PTR_OBJ_HASHTABLE_ENTRY);
    hashtable->entries = (VOID_PTR_OBJ_HASHTABLE_ENTRY *) alloc_static_block(mem_size);
    memset(hashtable->entries, 0, mem_size);
    hashtable->capacity = 1024;
    hashtable->count = 0;
  }
  else if (hashtable->capacity < 2 * hashtable->count) {
    VOID_PTR_OBJ_HASHTABLE_ENTRY *entries = hashtable->entries;
    uint32 capacity = hashtable->capacity;

    uint32 mem_size = 2 * capacity * sizeof(VOID_PTR_OBJ_HASHTABLE_ENTRY);
    hashtable->entries = (VOID_PTR_OBJ_HASHTABLE_ENTRY *) alloc_static_block(mem_size);
    memset(hashtable->entries, 0, mem_size);
    hashtable->capacity = 2 * capacity;
    hashtable->count = 0;

    for (int i=0 ; i < capacity ; i++) {
      VOID_PTR_OBJ_HASHTABLE_ENTRY *entry = entries + i;
      if (entry->ptr != NULL) {
        void_ptr_obj_hashtable_insert_unsafe(hashtable, entry->ptr, entry->obj);
        assert(are_shallow_eq(void_ptr_obj_hashtable_lookup(hashtable, entry->ptr)->obj, entry->obj));
      }
    }

    release_static_block(entries, capacity * sizeof(VOID_PTR_OBJ_HASHTABLE_ENTRY));
  }

  void_ptr_obj_hashtable_insert_unsafe(hashtable, ptr, obj);
  assert(are_shallow_eq(void_ptr_obj_hashtable_lookup(hashtable, ptr)->obj, obj));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct ENV_;
typedef struct ENV_ ENV;


static VOID_PTR_OBJ_HASHTABLE attachments;


OBJ attach_F2(OBJ obj_V, OBJ data_V, ENV &env) {
  if (!is_inline_obj(obj_V)) {
    void *ptr = obj_V.core_data.ptr;
    VOID_PTR_OBJ_HASHTABLE_ENTRY *entry = void_ptr_obj_hashtable_lookup(&attachments, ptr);
    assert(entry == NULL || are_eq(entry->obj, data_V));
    if (entry != NULL)
      void_ptr_obj_hashtable_insert(&attachments, ptr, data_V);
  }
  return obj_V;
}

OBJ fetch_F1(OBJ obj_V, ENV &env) {
  if (!is_inline_obj(obj_V)) {
    void *ptr = obj_V.core_data.ptr;
    VOID_PTR_OBJ_HASHTABLE_ENTRY *entry = void_ptr_obj_hashtable_lookup(&attachments, ptr);
    if (entry != NULL)
      return make_tag_obj(symb_id_just, entry->obj);
  }
  return make_symb(symb_id_nothing);
}

////////////////////////////////////////////////////////////////////////////////

OBJ source_file_location_F1(OBJ mtc_V, ENV &env) {
  OBJ source_file_location_F1_(OBJ mtc_V, ENV &env);

  // return source_file_location_F1_(mtc_V, env);

  static VOID_PTR_OBJ_HASHTABLE cache;

  if (is_inline_obj(mtc_V))
    return source_file_location_F1_(mtc_V, env);

  void *ptr = mtc_V.core_data.ptr;
  VOID_PTR_OBJ_HASHTABLE_ENTRY *entry = void_ptr_obj_hashtable_lookup(&cache, ptr);
  if (entry != NULL)
    return entry->obj;

  OBJ value = source_file_location_F1_(mtc_V, env);
  void_ptr_obj_hashtable_insert(&cache, ptr, value);
  return value;
}
