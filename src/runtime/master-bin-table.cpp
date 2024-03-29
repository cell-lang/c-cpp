#include "lib.h"
#include "one-way-bin-table.h"


bool master_bin_table_slot_is_empty(uint64 slot) {
  return get_high_32(slot) == 0xFFFFFFFF;
}

inline uint64 empty_slot(uint32 next) {
  uint64 slot = pack(next, 0xFFFFFFFF);
  assert(master_bin_table_slot_is_empty(slot));
  return slot;
}

inline uint32 get_next_free(uint64 slot) {
  assert(get_high_32(slot) == 0xFFFFFFFF);
  assert(get_low_32(slot) <= 16 * 1024 * 1024);
  return get_low_32(slot);
}

bool is_locked(uint64 slot) {
  assert(!master_bin_table_slot_is_empty(slot));
  return (slot >> 63) == 1;
}

inline uint64 lock_slot(uint64 slot) {
  assert(!master_bin_table_slot_is_empty(slot) && !is_locked(slot));
  uint64 locked_slot = slot | (1ULL << 63);
  assert(is_locked(locked_slot));
  return locked_slot;
}

inline uint64 unlock_slot(uint64 slot) {
  assert(!master_bin_table_slot_is_empty(slot) && is_locked(slot));
  uint64 unlocked_slot = slot & ~(1ULL << 63);
  assert(!is_locked(unlocked_slot));
  return unlocked_slot;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_alloc_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;
  uint32 first_free = table->first_free;
  uint64 *slots = table->slots;

  if (first_free >= capacity) {
    uint32 new_capacity = 2 * capacity;
    slots = extend_state_mem_uint64_array(mem_pool, slots, capacity, new_capacity);
    for (uint32 i=capacity ; i < new_capacity ; i++)
      slots[i] = empty_slot(i + 1);
    table->slots = slots;
    table->capacity = new_capacity;
  }

  table->first_free = get_next_free(slots[first_free]);
  uint64 args = pack_args(arg1, arg2);
  slots[first_free] = args;
  return first_free;
}

void master_bin_table_claim_reserved_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool) {
  //## MAYBE HERE WE COULD CHECK THAT THE SURROGATE HAS ALREADY BEEN REMOVED FROM THE AVAILABLE POOL

  uint32 capacity = table->capacity;
  uint64 *slots = table->slots;

  if (surr >= capacity) {
    uint32 new_capacity = 2 * capacity;
    while (surr >= new_capacity)
      new_capacity *= 2;
    slots = extend_state_mem_uint64_array(mem_pool, slots, capacity, new_capacity);
    for (uint32 i=capacity ; i < new_capacity ; i++)
      slots[i] = empty_slot(i + 1);
    table->slots = slots;
    table->capacity = new_capacity;
  }

  slots[surr] = pack_args(arg1, arg2);
}

void master_bin_table_release_surr(MASTER_BIN_TABLE *table, uint32 surr) {
  uint64 *slot_ptr = table->slots + surr;
  uint64 slot = *slot_ptr;
  if (!is_locked(slot)) {
    *slot_ptr = empty_slot(table->first_free);
    table->first_free = surr;
  }
}

bool master_bin_table_lock_surr(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity && !master_bin_table_slot_is_empty(table->slots[surr]));
  uint64 *slot_ptr = table->slots + surr;
  uint64 slot = *slot_ptr;
  if (!is_locked(slot)) {
    *slot_ptr = lock_slot(slot);
    return true;
  }
  else
    return false;
}

bool master_bin_table_unlock_surr(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity && !master_bin_table_slot_is_empty(table->slots[surr]));
  uint64 *slot_ptr = table->slots + surr;
  uint64 slot = *slot_ptr;
  if (is_locked(slot)) {
    *slot_ptr = unlock_slot(slot);
    return true;
  }
  else
    return false;
}

bool master_bin_table_slot_is_locked(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity && !master_bin_table_slot_is_empty(table->slots[surr]));
  return is_locked(table->slots[surr]);
}

uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *table, uint32 last_surr) {
  if (last_surr == 0xFFFFFFFF)
    return table->first_free;
  if (last_surr >= table->capacity)
    return last_surr + 1;
  return get_next_free(table->slots[last_surr]);
}

void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *table, uint32 next_free) {
  table->first_free = next_free;
}

////////////////////////////////////////////////////////////////////////////////

// void master_bin_table_self_check(MASTER_BIN_TABLE *table) {
//   uint32 capacity = table->capacity;
//   uint64 *slots = table->slots;
//   for (uint32 i=0 ; i < capacity ; i++) {
//     uint64 slot = slots[i];
//     if (!master_bin_table_slot_is_empty(slot)) {
//       uint32 arg1 = unpack_arg1(slot);
//       uint32 arg2 = unpack_arg2(slot);
//       assert(one_way_bin_table_contains(&table->table.forward, arg1, arg2));
//       assert(one_way_bin_table_contains(&table->table.backward, arg2, arg1));
//       uint32 surr = loaded_one_way_bin_table_payload(&table->table.forward, arg1, arg2);
//       assert(surr == i);
//     }
//   }
// }

// void master_bin_table_partial_self_check(MASTER_BIN_TABLE *table) {
//   uint32 capacity = table->capacity;
//   uint64 *slots = table->slots;
//   for (uint32 i=0 ; i < capacity ; i++) {
//     uint64 slot = slots[i];
//     if (!master_bin_table_slot_is_empty(slot)) {
//       uint32 arg1 = unpack_arg1(slot);
//       uint32 arg2 = unpack_arg2(slot);
//       assert(one_way_bin_table_contains(&table->table.forward, arg1, arg2));
//       // assert(one_way_bin_table_contains(&table->table.backward, arg2, arg1));
//       uint32 surr = loaded_one_way_bin_table_payload(&table->table.forward, arg1, arg2);
//       assert(surr == i);
//     }
//   }
// }

////////////////////////////////////////////////////////////////////////////////

const uint32 MASTER_BIN_TABLE_INIT_SIZE = 256;

void master_bin_table_init(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  void bin_table_loaded_init(BIN_TABLE *, STATE_MEM_POOL *);
  bin_table_loaded_init(&table->table, mem_pool);

  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, MASTER_BIN_TABLE_INIT_SIZE);
  for (uint32 i=0 ; i < MASTER_BIN_TABLE_INIT_SIZE ; i++)
    slots[i] = empty_slot(i + 1);

  table->slots = slots;
  table->capacity = MASTER_BIN_TABLE_INIT_SIZE;
  table->first_free = 0;
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_size(MASTER_BIN_TABLE *table) {
  return table->table.forward.count;
}

uint32 master_bin_table_count_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_get_count(&table->table.forward, arg1);
}

uint32 master_bin_table_count_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_get_count(&table->table.backward, arg2);
}

bool master_bin_table_contains(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return one_way_bin_table_contains(&table->table.forward, arg1, arg2);
}

bool master_bin_table_contains_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_contains_key(&table->table.forward, arg1);
}

bool master_bin_table_contains_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_contains_key(&table->table.backward, arg2);
}

bool master_bin_table_contains_surr(MASTER_BIN_TABLE *table, uint32 surr) {
  return surr < table->capacity && !master_bin_table_slot_is_empty(table->slots[surr]);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_restrict_1(MASTER_BIN_TABLE *table, uint32 arg1, uint32 *arg2s) {
  return one_way_bin_table_restrict(&table->table.forward, arg1, arg2s);
}

uint32 master_bin_table_restrict_1(MASTER_BIN_TABLE *table, uint32 arg1, uint32 *arg2s, uint32 *surrs) {
  return loaded_one_way_bin_table_restrict(&table->table.forward, arg1, arg2s, surrs);
}

uint32 master_bin_table_restrict_2(MASTER_BIN_TABLE *table, uint32 arg2, uint32 *arg1s) {
  return one_way_bin_table_restrict(&table->table.backward, arg2, arg1s);
}

uint32 master_bin_table_restrict_2(MASTER_BIN_TABLE *table, uint32 arg2, uint32 *arg1s, uint32 *surrs) {
  return loaded_one_way_bin_table_restrict(&table->table.backward, arg2, arg1s, surrs);
}

UINT32_ARRAY master_bin_table_range_restrict_1(MASTER_BIN_TABLE *table, uint32 arg1, uint32 first, uint32 *arg2s, uint32 capacity) {
  return one_way_bin_table_range_restrict(&table->table.forward, arg1, first, arg2s, capacity);
}

UINT32_ARRAY master_bin_table_range_restrict_2(MASTER_BIN_TABLE *table, uint32 arg2, uint32 first, uint32 *arg1s, uint32 capacity) {
  return one_way_bin_table_range_restrict(&table->table.backward, arg2, first, arg1s, capacity);
}

UINT32_ARRAY master_bin_table_range_restrict_1_with_surrs(MASTER_BIN_TABLE *table, uint32 arg1, uint32 first, uint32 *arg2s_surrs, uint32 capacity) {
  return loaded_one_way_bin_table_range_restrict(&table->table.forward, arg1, first, arg2s_surrs, capacity);
}

UINT32_ARRAY master_bin_table_range_restrict_2_with_surrs(MASTER_BIN_TABLE *table, uint32 arg2, uint32 first, uint32 *arg1s_surrs, uint32 capacity) {
  return loaded_one_way_bin_table_range_restrict(&table->table.backward, arg2, first, arg1s_surrs, capacity);
}

uint32 master_bin_table_lookup_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_lookup(&table->table.forward, arg1);
}

uint32 master_bin_table_lookup_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_lookup(&table->table.backward, arg2);
}

////////////////////////////////////////////////////////////////////////////////

uint32 master_bin_table_lookup_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  uint32 surr = loaded_one_way_bin_table_payload(&table->table.forward, arg1, arg2);
#ifndef NDEBUG
  if (surr != 0xFFFFFFFF) {
    assert(surr < table->capacity);
    uint64 slot = table->slots[surr];
    assert(unpack_arg1(slot) == arg1);
    assert(unpack_arg2(slot) == arg2);
  }
  assert(surr == loaded_one_way_bin_table_payload(&table->table.backward, arg2, arg1));
#endif
  return surr;
}

// uint32 master_bin_table_lookup_possibly_locked_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
//   uint32 surr = loaded_one_way_bin_table_payload(&table->table.forward, arg1, arg2);
// #ifndef NDEBUG
//   if (surr != 0xFFFFFFFF) {
//     assert(surr < table->capacity);
//     uint64 slot = table->slots[surr];
//     if (is_locked(slot))
//       slot = unlock_slot(slot);
//     assert(unpack_arg1(slot) == arg1);
//     assert(unpack_arg2(slot) == arg2);
//   }
// #endif
//   return surr;
// }

uint64 *master_bin_table_slots(MASTER_BIN_TABLE *table) {
  return table->slots;
}

uint32 master_bin_table_get_arg_1(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity);
  uint64 slot = table->slots[surr];
  assert(master_bin_table_lookup_surr(table, unpack_arg1(slot), unpack_arg2(slot)) == surr);
  return unpack_arg1(slot);
}

uint32 master_bin_table_get_arg_2(MASTER_BIN_TABLE *table, uint32 surr) {
  assert(surr < table->capacity);
  uint64 slot = table->slots[surr];
  assert(master_bin_table_lookup_surr(table, unpack_arg1(slot), unpack_arg2(slot)) == surr);
  return unpack_arg2(slot);
}

////////////////////////////////////////////////////////////////////////////////

// Code to recover the surrogate:
//   int32 code = master_bin_table_insert_ex(table, arg1, arg2, mem_pool);
//   uint32 surr12 = code >= 0 ? code : -code - 1;
//   bool was_new = code >= 0;
int32 master_bin_table_insert_ex(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  if (master_bin_table_contains(table, arg1, arg2)) {
    uint32 surr = master_bin_table_lookup_surr(table, arg1, arg2);
    assert(surr != 0xFFFFFFFF);
    return -((int32) surr) - 1;
  }

  assert(!one_way_bin_table_contains(&table->table.forward, arg1, arg2));
  assert(!one_way_bin_table_contains(&table->table.backward, arg2, arg1));

  uint32 surr = master_bin_table_alloc_surr(table, arg1, arg2, mem_pool);
  loaded_one_way_bin_table_insert_unique(&table->table.forward, arg1, arg2, surr, mem_pool);
  loaded_one_way_bin_table_insert_unique(&table->table.backward, arg2, arg1, surr, mem_pool);
  return surr;
}

bool master_bin_table_insert(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  int32 code = master_bin_table_insert_ex(table, arg1, arg2, mem_pool);
  return code >= 0;
}

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool) {
  if (!master_bin_table_contains(table, arg1, arg2)) {
    master_bin_table_claim_reserved_surr(table, arg1, arg2, surr, mem_pool);
    loaded_one_way_bin_table_insert_unique(&table->table.forward, arg1, arg2, surr, mem_pool);
    loaded_one_way_bin_table_insert_unique(&table->table.backward, arg2, arg1, surr, mem_pool);
    return true;
  }
  else {
    assert(master_bin_table_lookup_surr(table, arg1, arg2) == surr);
    return false;
  }
}

void master_bin_table_insert_using_first_free_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool) {
  if (surr == table->first_free) {
    assert(!master_bin_table_contains(table, arg1, arg2));
    uint32 same_surr = master_bin_table_alloc_surr(table, arg1, arg2, mem_pool);
    assert(same_surr == surr);
    loaded_one_way_bin_table_insert_unique(&table->table.forward, arg1, arg2, surr, mem_pool);
    loaded_one_way_bin_table_insert_unique(&table->table.backward, arg2, arg1, surr, mem_pool);
  }
  else {
    assert(master_bin_table_contains(table, arg1, arg2));
    assert(master_bin_table_lookup_surr(table, arg1, arg2) == surr);
  }
}

void master_bin_table_clear(MASTER_BIN_TABLE *table, uint32 highest_locked_surr, STATE_MEM_POOL *mem_pool) {
  loaded_one_way_bin_table_clear(&table->table.forward, mem_pool);
  loaded_one_way_bin_table_clear(&table->table.backward, mem_pool);

  uint32 capacity = table->capacity;
  uint64 *slots = table->slots;

  if (highest_locked_surr == 0xFFFFFFFF) {
    if (capacity != MASTER_BIN_TABLE_INIT_SIZE) {
      release_state_mem_uint64_array(mem_pool, slots, capacity);

      capacity = MASTER_BIN_TABLE_INIT_SIZE;
      slots = alloc_state_mem_uint64_array(mem_pool, MASTER_BIN_TABLE_INIT_SIZE);

      table->capacity = MASTER_BIN_TABLE_INIT_SIZE;
      table->slots = slots;
    }

    table->first_free = 0;
    for (uint32 i=0 ; i < capacity ; i++)
      slots[i] = empty_slot(i + 1);
  }
  else {
    if (capacity != MASTER_BIN_TABLE_INIT_SIZE) {
      release_state_mem_uint64_array(mem_pool, slots, capacity);

      uint32 new_capacity = pow_2_ceiling(highest_locked_surr + 1, MASTER_BIN_TABLE_INIT_SIZE);
      uint64 *new_slots = alloc_state_mem_uint64_array(mem_pool, new_capacity);

      table->capacity = new_capacity;
      table->slots = new_slots;

      uint32 last_free = new_capacity;
      for (uint32 i=capacity-1 ; i != 0xFFFFFFFF ; i--) {
        uint64 slot = slots[i];
        if (master_bin_table_slot_is_empty(slot) || !is_locked(slot)) {
          new_slots[i] = empty_slot(last_free);
          last_free = i;
        }
        else
          new_slots[i] = slot;
      }
      table->first_free = last_free;
    }
    else {
      uint32 last_free = MASTER_BIN_TABLE_INIT_SIZE;
      for (uint32 i=MASTER_BIN_TABLE_INIT_SIZE-1 ; i != 0xFFFFFFFF ; i--) {
        uint64 *slot_ptr = slots + i;
        uint64 slot = *slot_ptr;
        if (master_bin_table_slot_is_empty(slot) || !is_locked(slot)) {
          *slot_ptr = empty_slot(last_free);
          last_free = i;
        }
      }
    }
  }
}

bool master_bin_table_delete(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  uint32 surr = loaded_one_way_bin_table_delete(&table->table.forward, arg1, arg2);
  if (surr != 0xFFFFFFFF) {
    uint32 surr_ = loaded_one_way_bin_table_delete(&table->table.backward, arg2, arg1);
    assert(surr_ == surr);
    master_bin_table_release_surr(table, surr);
    return true;
  }
  else
    return false;
}

uint32 master_bin_table_delete_1(MASTER_BIN_TABLE *table, uint32 arg1) {
  uint32 count = one_way_bin_table_get_count(&table->table.forward, arg1);
  if (count > 0) {
    uint32 inline_array[256];
    uint32 *arg2s = count <= 128 ? inline_array : new_uint32_array(2 * count);
    uint32 *surrs = arg2s + count;
    loaded_one_way_bin_table_delete_by_key(&table->table.forward, arg1, arg2s, surrs);

    for (uint32 i=0 ; i < count ; i++) {
      master_bin_table_release_surr(table, surrs[i]);
      loaded_one_way_bin_table_delete_existing(&table->table.backward, arg2s[i], arg1);
    }
  }
  return count;
}

uint32 master_bin_table_delete_2(MASTER_BIN_TABLE *table, uint32 arg2) {
  uint32 count = one_way_bin_table_get_count(&table->table.backward, arg2);
  if (count > 0) {
    uint32 inline_array[256];
    uint32 *arg1s = count <= 128 ? inline_array : new_uint32_array(2 * count);
    uint32 *surrs = arg1s + count;
    loaded_one_way_bin_table_delete_by_key(&table->table.backward, arg2, arg1s, surrs);

    for (uint32 i=0 ; i < count ; i++) {
#ifndef NDEBUG
      uint32 surr = surrs[i];
      assert(surr != 0xFFFFFFFF && surr < table->capacity);
      uint64 slot = table->slots[surr];
      if (is_locked(slot))
        slot = unlock_slot(slot);
      assert(unpack_arg1(slot) == arg1s[i]);
      assert(unpack_arg2(slot) == arg2);
#endif
      master_bin_table_release_surr(table, surrs[i]);
      loaded_one_way_bin_table_delete_existing(&table->table.forward, arg1s[i], arg2);
    }
  }
  return count;
}

bool master_bin_table_delete_by_surr(MASTER_BIN_TABLE *table, uint32 assoc_surr) {
  assert(assoc_surr < table->capacity);
  uint64 slot = table->slots[assoc_surr];
  uint32 arg1 = unpack_arg1(slot);
  uint32 arg2 = unpack_arg2(slot);
  assert(master_bin_table_lookup_surr(table, unpack_arg1(slot), unpack_arg2(slot)) == assoc_surr);
  return master_bin_table_delete(table, arg1, arg2);
}

////////////////////////////////////////////////////////////////////////////////

bool master_bin_table_col_1_is_key(MASTER_BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->table.forward);
}

bool master_bin_table_col_2_is_key(MASTER_BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->table.backward);
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_copy_to(MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  bin_table_copy_to(&table->table, surr_to_obj_1, store_1, surr_to_obj_2, store_2, strm_1, strm_2);
}

void master_bin_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool flipped) {
  bin_table_write(write_state, &table->table, surr_to_obj_1, store_1, surr_to_obj_2, store_2, false, flipped);
}

////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void master_bin_table_self_check(MASTER_BIN_TABLE *table) {
  uint32 errors = 0;
  uint32 left = master_bin_table_size(table);
  for (uint32 i=0 ; left > 0 ; i++) {
    assert(i < table->capacity);
    uint64 args = table->slots[i];
    if (!master_bin_table_slot_is_empty(args)) {
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      uint32 fwd_surr = loaded_one_way_bin_table_payload(&table->table.forward, arg1, arg2);
      uint32 bkwd_surr = loaded_one_way_bin_table_payload(&table->table.backward, arg2, arg1);
      if (fwd_surr != bkwd_surr) {
        errors++;
        printf("%d, %d -> %d != %d\n", arg1, arg2, fwd_surr, bkwd_surr);
      }
      left--;
    }
  }
  if (errors == 0) {
    puts("All is fine");
  }
  else {
    printf("Number of errors = %d\n", errors);
  }
}
#endif
