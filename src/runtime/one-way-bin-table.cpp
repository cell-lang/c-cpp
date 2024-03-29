#include "lib.h"
#include "one-way-bin-table.h"


// Valid slot states:
//   - Value + payload: 32 bit payload - 3 zeros   - 29 bit value
//   - Index + count:   32 bit count   - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:           32 zeros - ArraySliceAllocator.EMPTY_MARKER == 0xFFFFFFFF
//     This type of slot can only be stored in a block, but cannot be passed in or out


const uint32 ONE_WAY_BIN_TABLE_MIN_CAPACITY = 256;

//////////////////////////////////////////////////////////////////////////////

static bool one_way_bin_table_slot_is_empty(uint64 slot) {
  return slot == EMPTY_SLOT;
}

static bool one_way_bin_table_slot_is_index(uint64 slot) {
  return slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT;
}

//////////////////////////////////////////////////////////////////////////////

static void one_way_bin_table_resize(ONE_WAY_BIN_TABLE *table, uint32 index, STATE_MEM_POOL *mem_pool) {
  assert(table->capacity >= ONE_WAY_BIN_TABLE_MIN_CAPACITY);
  uint32 capacity = table->capacity;
  uint32 new_capacity = 2 * capacity;
  while (index >= new_capacity)
    new_capacity *= 2;
  uint64 *new_slots = extend_state_mem_uint64_array(mem_pool, table->column, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    new_slots[i] = EMPTY_SLOT;
  table->capacity = new_capacity;
  table->column = new_slots;
}

//////////////////////////////////////////////////////////////////////////////

void one_way_bin_table_init(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  array_mem_pool_init(&table->array_pool, false, mem_pool);
  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, ONE_WAY_BIN_TABLE_MIN_CAPACITY);
  for (uint32 i=0 ; i < ONE_WAY_BIN_TABLE_MIN_CAPACITY ; i++)
    slots[i] = EMPTY_SLOT;
  table->column = slots;
  table->capacity = ONE_WAY_BIN_TABLE_MIN_CAPACITY;
  table->count = 0;
}

void one_way_bin_table_release(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  impl_fail(NULL); //## IS THIS AT ALL NEEDED?
}

void one_way_bin_table_clear(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  array_mem_pool_clear(&table->array_pool, mem_pool);
  uint32 capacity = table->capacity;
  uint64 *slots = table->column;
  if (capacity > ONE_WAY_BIN_TABLE_MIN_CAPACITY) { //## WHAT WOULD BE A GOOD VALUE FOR THE REALLOCATION THRESHOLD?
    release_state_mem_uint64_array(mem_pool, slots, capacity);
    slots = alloc_state_mem_uint64_array(mem_pool, ONE_WAY_BIN_TABLE_MIN_CAPACITY);
    table->column = slots;
    capacity = ONE_WAY_BIN_TABLE_MIN_CAPACITY;
    table->capacity = capacity;
  }
  for (uint32 i=0 ; i < ONE_WAY_BIN_TABLE_MIN_CAPACITY ; i++)
    slots[i] = EMPTY_SLOT;
  table->count = 0;
}

//////////////////////////////////////////////////////////////////////////////

bool one_way_bin_table_contains(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 >= table->capacity)
    return false;

  uint64 slot = table->column[surr1];

  if (one_way_bin_table_slot_is_empty(slot))
    return false;

  if (one_way_bin_table_slot_is_index(slot))
    return overflow_table_contains(&table->array_pool, slot, surr2);

  if (get_low_32(slot) == surr2)
    return true;

  return get_high_32(slot) == surr2;
}

uint32 one_way_bin_table_lookup_unstable_surr(ONE_WAY_BIN_TABLE *table, uint32 key, uint32 value) {
  uint32 capacity = table->capacity;

  if (key >= table->capacity)
    return 0xFFFFFFFF;

  uint64 slot = table->column[key];

  if (one_way_bin_table_slot_is_empty(slot))
    return 0xFFFFFFFF;

  if (one_way_bin_table_slot_is_index(slot))
    return 2 * capacity + overflow_table_value_offset(&table->array_pool, slot, value);

  if (get_low_32(slot) == value)
    return 2 * key;

  if (get_high_32(slot) == value)
    return 2 * key + 1;

  return 0xFFFFFFFF;
}

bool one_way_bin_table_contains_key(ONE_WAY_BIN_TABLE *table, uint32 surr1) {
  return surr1 < table->capacity && !one_way_bin_table_slot_is_empty(table->column[surr1]);
}

// uint32[] one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr) {
//   if (surr >= table->capacity)
//     return Array.emptyIntArray;

//   uint64 slot = table->column[surr];

//   if (one_way_bin_table_slot_is_empty(slot))
//     return Array.emptyIntArray;

//   if (one_way_bin_table_slot_is_index(slot)) {
//     uint32 count = get_count(slot);
//     uint32[] surrs = new uint32[count];
//     overflow_table_copy(&table->array_pool, slot, surrs);
//     return surrs;
//   }

//   uint32 low = get_low_32(slot);
//   uint32 high = get_high_32(slot);
//   return high == EMPTY_MARKER ? new uint32[] {low} : new uint32[] {low, high};
// }

uint32 one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr, uint32 *dest) {
  if (surr >= table->capacity)
    return 0;

  uint64 slot = table->column[surr];

  if (one_way_bin_table_slot_is_empty(slot))
    return 0;

  if (one_way_bin_table_slot_is_index(slot)) {
    overflow_table_copy(&table->array_pool, slot, dest, 0);
    return get_count(slot);
  }

  dest[0] = get_low_32(slot);
  uint32 high = get_high_32(slot);
  if (high == EMPTY_MARKER)
    return 1;
  dest[1] = high;
  return 2;
}

UINT32_ARRAY one_way_bin_table_range_restrict(ONE_WAY_BIN_TABLE *table, uint32 key, uint32 first, uint32 *dest, uint32 capacity) {
  assert(capacity == 64);

  UINT32_ARRAY result;

  if (key < table->capacity) {
    uint64 slot = table->column[key];

    if (!one_way_bin_table_slot_is_empty(slot)) {
      if (one_way_bin_table_slot_is_index(slot)) {
        result = overflow_table_range_copy(&table->array_pool, slot, first, dest, capacity);
      }
      else {
        assert(first == 0);
        dest[0] = get_low_32(slot);
        uint32 high = get_high_32(slot);
        if (high != EMPTY_MARKER) {
          dest[1] = high;
          result.size = 2;
        }
        else
          result.size = 1;
        result.array = dest;
      }
    }
    else
      result.size = 0;
  }
  else
    result.size = 0;

  return result;
}

uint32 one_way_bin_table_lookup(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= table->capacity)
    return -1;
  uint64 slot = table->column[surr];
  if (one_way_bin_table_slot_is_empty(slot))
    return -1;
  if (one_way_bin_table_slot_is_index(slot) | get_high_32(slot) != EMPTY_MARKER)
    internal_fail();
  assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
  return get_low_32(slot);
}

uint32 one_way_bin_table_get_count(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= table->capacity)
    return 0;
  uint64 slot = table->column[surr];
  if (one_way_bin_table_slot_is_empty(slot))
    return 0;
  if (one_way_bin_table_slot_is_index(slot))
    return get_count(slot);
  return get_high_32(slot) == EMPTY_MARKER ? 1 : 2;
}

bool one_way_bin_table_insert(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool) {
  if (surr1 >= table->capacity)
    one_way_bin_table_resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (one_way_bin_table_slot_is_empty(slot)) {
    *slot_ptr = pack(surr2, EMPTY_MARKER);
    table->count++;
    return true;
  }

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  if (get_tag(low) == INLINE_SLOT & high == EMPTY_MARKER) {
    if (surr2 == low)
      return false;
    *slot_ptr = pack(low, surr2);
    table->count++;
    return true;
  }

  uint64 updated_slot = overflow_table_insert(&table->array_pool, slot, surr2, mem_pool);
  if (updated_slot == slot)
    return false;

  *slot_ptr = updated_slot;
  table->count++;
  return true;
}

void one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool) {
  if (surr1 >= table->capacity)
    one_way_bin_table_resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (one_way_bin_table_slot_is_empty(slot)) {
    *slot_ptr = pack(surr2, EMPTY_MARKER);
    table->count++;
    return;
  }

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  if (get_tag(low) == INLINE_SLOT & high == EMPTY_MARKER) {
    assert(surr2 != low);
    *slot_ptr = pack(low, surr2);
    table->count++;
    return;
  }

  uint64 updated_slot = overflow_table_insert_unique(&table->array_pool, slot, surr2, mem_pool);
  assert(updated_slot != slot);

  *slot_ptr = updated_slot;
  table->count++;
}

// Assuming there's at most one entry whose first argument is surr1
uint32 one_way_bin_table_update(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool) {
  if (surr1 >= table->capacity)
    one_way_bin_table_resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (one_way_bin_table_slot_is_empty(slot)) {
    *slot_ptr = pack(surr2, EMPTY_MARKER);
    table->count++;
    return -1;
  }

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  if (get_tag(low) == INLINE_SLOT & high == EMPTY_MARKER) {
    *slot_ptr = pack(surr2, EMPTY_MARKER);
    return low;
  }

  internal_fail();
}

bool one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 >= table->capacity)
    return false;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (one_way_bin_table_slot_is_empty(slot))
    return false;

  if (one_way_bin_table_slot_is_index(slot)) {
    uint64 updated_slot = overflow_table_delete(&table->array_pool, slot, surr2);
    if (updated_slot == slot)
      return false;

    *slot_ptr = updated_slot;
    table->count--;
    return true;
  }

  assert(get_tag(get_low_32(slot)) == INLINE_SLOT);

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  if (surr2 == low) {
    if (high == EMPTY_MARKER)
      *slot_ptr = EMPTY_SLOT;
    else
      *slot_ptr = pack(high, EMPTY_MARKER);
    table->count--;
    return true;
  }

  if (surr2 == high) {
    *slot_ptr = pack(low, EMPTY_MARKER);
    table->count--;
    return true;
  }

  return false;
}

void one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1) {
  if (surr1 >= table->capacity)
    return;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (one_way_bin_table_slot_is_empty(slot))
    return;

  *slot_ptr = EMPTY_SLOT;

  if (one_way_bin_table_slot_is_index(slot)) {
    uint32 slot_count = get_count(slot);
    overflow_table_delete(&table->array_pool, slot);
    table->count -= slot_count;
  }
  else {
    assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
    table->count -= get_high_32(slot) != EMPTY_MARKER ? 2 : 1;
  }
}

void one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2) {
  if (surr1 >= table->capacity)
    return;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (one_way_bin_table_slot_is_empty(slot))
    return;

  *slot_ptr = EMPTY_SLOT;

  if (one_way_bin_table_slot_is_index(slot)) {
    uint32 slot_count = get_count(slot);
    overflow_table_copy(&table->array_pool, slot, surrs2, 0);
    overflow_table_delete(&table->array_pool, slot);
    table->count -= slot_count;
  }
  else {
    assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
    surrs2[0] = get_low_32(slot);
    uint32 high = get_high_32(slot);
    if (high != EMPTY_MARKER) {
      surrs2[1] = high;
      table->count -= 2;
    }
    else
      table->count--;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool one_way_bin_table_is_map(ONE_WAY_BIN_TABLE *table) {
  uint32 capacity = table->capacity;
  uint64 *slots = table->column;
  for (uint32 i=0 ; i < capacity ; i++) {
    uint64 slot = slots[i];
    if (!one_way_bin_table_slot_is_empty(slot) & (get_tag(get_low_32(slot)) != INLINE_SLOT | get_high_32(slot) != EMPTY_MARKER))
      return false;
  }
  return true;
}

// void one_way_bin_table_copy(ONE_WAY_BIN_TABLE *table, uint32 *dest) {
//   uint32 capacity = table->capacity;
//   uint64 *slots = table->column;

//   uint32 inline_buffer[1024];
//   uint32 *buffer = inline_buffer;
//   uint32 buffer_size = 1024;

//   uint32 next = 0;
//   for (uint32 i=0 ; i < capacity ; i++) {
//     uint64 slot = slots[i];
//     if (!one_way_bin_table_slot_is_empty(slot)) {
//       if (one_way_bin_table_slot_is_index(slot)) {
//         uint32 slot_count = get_count(slot);
//         if (slot_count > buffer_size) {
//           do
//             buffer_size *= 2;
//           while (slot_count > buffer_size);
//           buffer = new_uint32_array(buffer_size);
//         }
//         overflow_table_copy(&table->array_pool, slot, buffer, 0);
//         for (uint32 j=0 ; j < slot_count ; j++) {
//           uint32 idx = next + 2 * j;
//           dest[idx] = i;
//           dest[idx + 1] = buffer[j];
//         }
//         next += 2 * slot_count;
//       }
//       else {
//         dest[next++] = i;
//         dest[next++] = get_low_32(slot);
//         uint32 high = get_high_32(slot);
//         if (high != EMPTY_MARKER) {
//           dest[next++] = i;
//           dest[next++] = high;
//         }
//       }
//     }
//   }
//   assert(next == 2 * table->count);
// }


void one_way_bin_table_build_reverse(ONE_WAY_BIN_TABLE *table, ONE_WAY_BIN_TABLE *rev_table, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;
  for (uint32 i=0 ; i < capacity ; i++) {
    uint64 slot = table->column[i];
    if (!one_way_bin_table_slot_is_empty(slot)) {
      if (one_way_bin_table_slot_is_index(slot)) {
        overflow_table_insert_reversed(&table->array_pool, i, slot, rev_table, mem_pool);
      }
      else {
        uint32 low = get_low_32(slot);
        uint32 high = get_high_32(slot);
        one_way_bin_table_insert_unique(rev_table, low, i, mem_pool);
        if (high != EMPTY_MARKER)
          one_way_bin_table_insert_unique(rev_table, high, i, mem_pool);
      }
    }
  }
}
