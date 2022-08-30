#include "lib.h"


void map_surr_u32_init(MAP_SURR_U32 *) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void map_surr_u32_clear(MAP_SURR_U32 *) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}
void map_surr_u32_delete(MAP_SURR_U32 *, uint32 key) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void map_surr_u32_set(MAP_SURR_U32 *, uint32 key, uint32 value) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

uint32 map_surr_u32_lookup(MAP_SURR_U32 *, uint32 key, uint32 default_) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

void map_i64_surr_init(MAP_I64_SURR *map) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void map_i64_surr_clear(MAP_I64_SURR *map) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void map_i64_surr_delete(MAP_I64_SURR *map, int64) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void map_i64_surr_insert_new(MAP_I64_SURR *map, int64, uint32) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

uint32 map_i64_surr_lookup(MAP_I64_SURR *map, int64) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

void quasi_map_hcode_surr_init(QUASI_MAP_HCODE_SURR *table, STATE_MEM_POOL *mem_pool) {

}

void quasi_map_hcode_surr_release(QUASI_MAP_HCODE_SURR *table, STATE_MEM_POOL *mem_pool) {

}

void quasi_map_hcode_surr_resize(QUASI_MAP_HCODE_SURR *table, uint32 new_size, STATE_MEM_POOL *mem_pool) {

}

////////////////////////////////////////////////////////////////////////////////

void quasi_map_hcode_surr_insert(QUASI_MAP_HCODE_SURR *table, uint32 hashcode, uint32 index, STATE_MEM_POOL *mem_pool) {
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

////////////////////////////////////////////////////////////////////////////////

void quasi_map_hcode_surr_delete(QUASI_MAP_HCODE_SURR *table, uint32 hashcode, uint32 index) {
  assert(table->main_hashtable.find(hashcode) != table->main_hashtable.end());

  auto main_it = table->main_hashtable.find(hashcode);
  uint32 code = main_it->second;

  if (code >> 31 == 0) {
    // Easy case, no collisions
    assert(code - 1 == index);
    table->main_hashtable.erase(main_it);
    return;
  }

  // There are collisions
  uint32 first_index = (code & 0x7FFFFFFF) - 1;

  auto colls_it = table->collisions.find(hashcode);
  assert(colls_it != table->collisions.end());

  vector<uint32> &colls = colls_it->second;

  if (first_index == index) {
    if (colls.size() == 1) {
      main_it->second = colls.front() + 1;
      table->collisions.erase(colls_it);
    }
    else {
      main_it->second = (colls.back() + 1) | (1u << 31);
      colls.pop_back();
    }
  }
  else {
    if (colls.size() == 1) {
      assert(colls.front() == index);
      main_it->second = code & 0x7FFFFFFF;
      table->collisions.erase(colls_it);
    }
    else {
      assert(std::find(colls.begin(), colls.end(), index) != colls.end());
      for (uint32 i=0 ; i < colls.size() ; i++) {
        if (colls[i] == index) {
          colls[i] = colls.back();
          colls.pop_back();
          break;
        }
      }
    }
  }
}

void quasi_map_hcode_surr_clear(QUASI_MAP_HCODE_SURR *table) {
  table->main_hashtable.clear();
  table->collisions.clear();
}

////////////////////////////////////////////////////////////////////////////////

uint32 quasi_map_hcode_surr_find(QUASI_MAP_HCODE_SURR *table, uint32 hashcode, OBJ *slots, OBJ value) {
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

////////////////////////////////////////////////////////////////////////////////

void trns_map_surr_surr_surr_init(TRNS_MAP_SURR_SURR_SURR *map) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void trns_map_surr_surr_surr_clear(TRNS_MAP_SURR_SURR_SURR *map) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void trns_map_surr_surr_surr_insert_new(TRNS_MAP_SURR_SURR_SURR *map, uint32 surr1, uint32 surr2, uint32 surr3) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

uint32 trns_map_surr_surr_surr_extract(TRNS_MAP_SURR_SURR_SURR *map, uint32 surr1, uint32 surr2) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

uint32 trns_map_surr_surr_surr_lookup(TRNS_MAP_SURR_SURR_SURR *map, uint32 surr1, uint32 surr2) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool trns_map_surr_surr_surr_is_empty(TRNS_MAP_SURR_SURR_SURR *map) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

void trns_map_surr_u32_init(TRNS_MAP_SURR_U32 *, STATE_MEM_POOL *) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void trns_map_surr_u32_set(TRNS_MAP_SURR_U32 *, uint32 key, uint32 value) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

uint32 trns_map_surr_u32_lookup(TRNS_MAP_SURR_U32 *, uint32 key, uint32 default_) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}
