#include "lib.h"


void quasi_map_u32_u32_init(QUASI_MAP_U32_U32 *table, STATE_MEM_POOL *mem_pool) {

}

void quasi_map_u32_u32_release(QUASI_MAP_U32_U32 *table, STATE_MEM_POOL *mem_pool) {

}

void quasi_map_u32_u32_resize(QUASI_MAP_U32_U32 *table, uint32 new_size, STATE_MEM_POOL *mem_pool) {

}

void quasi_map_u32_u32_insert(QUASI_MAP_U32_U32 *table, uint32 hashcode, uint32 index, STATE_MEM_POOL *mem_pool) {
  uint32 &index_ref = table->main_hashtable[hashcode];
  if (index_ref == 0) {
    index_ref = index + 1;
  }
  else {
    index_ref |= 1u << 31;
    table->collisions[hashcode].push_back(index);
  }

  // auto it = table->main_hashtable.find(hashcode);
  // if (it == table->main_hashtable.end()) {
  //   table->main_hashtable[hashcode] = index;
  // }
  // else {
  //   it->second |= 1u << 31;
  //   table->collisions[hashcode].push_back(index);
  // }
}

void quasi_map_u32_u32_delete(QUASI_MAP_U32_U32 *table, uint32 hashcode, uint32 index) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

uint32 quasi_map_u32_u32_find(QUASI_MAP_U32_U32 *table, uint32 hashcode, OBJ *slots, OBJ value) {
  auto main_it = table->main_hashtable.find(hashcode);
  if (main_it == table->main_hashtable.end())
    return 0xFFFFFFFF;

  uint32 idx = main_it->second - 1;
  if (idx >> 31 == 0) {
    // No collisions
    if (are_eq(slots[idx], value))
      return idx;
    else
      return 0xFFFFFFFF;
  }
  else {
    // There are collisions

    idx &= 0x7FFFFFFF;

    if (are_eq(slots[idx], value))
      return idx;

    auto colls_it = table->collisions.find(hashcode);
    if (colls_it == table->collisions.end())
      throw 0;
      // return 0xFFFFFFFF;

    vector<uint32> &colls = colls_it->second;
    for (uint32 i=0 ; i < colls.size() ; i++) {
      idx = colls[i];
      if (are_eq(slots[idx], value))
        return idx;
    }

    return 0xFFFFFFFF;
  }
}
