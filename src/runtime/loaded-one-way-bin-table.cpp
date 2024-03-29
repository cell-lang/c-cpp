#include "lib.h"
#include "one-way-bin-table.h"


// Valid slot states:
//   - Value + payload: 32 bit payload - 3 zeros   - 29 bit value
//   - Index + count:   32 bit count   - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:           32 zeros - ArraySliceAllocator.EMPTY_MARKER == 0xFFFFFFFF
//     This type of slot can only be stored in a block, but cannot be passed in or out


const uint32 LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY = 256;

//////////////////////////////////////////////////////////////////////////////

static bool loaded_one_way_bin_table_slot_is_empty(uint64 slot) {
  return slot == EMPTY_SLOT;
}

static bool loaded_one_way_bin_table_slot_is_index(uint64 slot) {
  return slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT;
}

//////////////////////////////////////////////////////////////////////////////

static void loaded_one_way_bin_table_resize(ONE_WAY_BIN_TABLE *table, uint32 index, STATE_MEM_POOL *mem_pool) {
  assert(table->capacity >= LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY);
  uint32 capacity = table->capacity;
  uint32 new_capacity = 2 * capacity;
  while (index >= new_capacity)
    new_capacity *= 2;
  uint64 *new_slots = alloc_state_mem_uint64_array(mem_pool, 2 * new_capacity);
  uint64 *slots = table->column;
  memcpy(new_slots, slots, capacity * sizeof(uint64));
  memcpy(new_slots + new_capacity, slots + capacity, capacity * sizeof(uint64));
  release_state_mem_uint64_array(mem_pool, slots, 2 * capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    new_slots[i] = EMPTY_SLOT;
  table->capacity = new_capacity;
  table->column = new_slots;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void loaded_one_way_bin_table_init(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  array_mem_pool_init(&table->array_pool, true, mem_pool);
  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, 2 * LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY);
  for (uint32 i=0 ; i < LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY ; i++)
    slots[i] = EMPTY_SLOT;
  table->column = slots;
  table->capacity = LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY;
  table->count = 0;
}

//////////////////////////////////////////////////////////////////////////////

void loaded_one_way_bin_table_release(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  impl_fail(NULL); //## IS THIS AT ALL NEEDED?
}

void loaded_one_way_bin_table_clear(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  array_mem_pool_clear(&table->array_pool, mem_pool);
  uint32 capacity = table->capacity;
  uint64 *slots = table->column;
  if (capacity > LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY) { //## WHAT WOULD BE A GOOD VALUE FOR THE REALLOCATION THRESHOLD?
    release_state_mem_uint64_array(mem_pool, slots, 2 * capacity);
    slots = alloc_state_mem_uint64_array(mem_pool, 2 * LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY);
    table->column = slots;
    capacity = LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY;
    table->capacity = capacity;
  }
  for (uint32 i=0 ; i < LOADED_ONE_WAY_BIN_TABLE_MIN_CAPACITY ; i++)
    slots[i] = EMPTY_SLOT;
  table->count = 0;
}

//////////////////////////////////////////////////////////////////////////////

uint32 loaded_one_way_bin_table_payload(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  uint32 capacity = table->capacity;

  if (surr1 < capacity) {
    uint64 *slot_ptr = table->column + surr1;
    uint64 slot = *slot_ptr;

    if (!loaded_one_way_bin_table_slot_is_empty(slot)) {
      if (loaded_one_way_bin_table_slot_is_index(slot))
        return loaded_overflow_table_lookup(&table->array_pool, slot, surr2);

      if (get_low_32(slot) == surr2 | get_high_32(slot) == surr2) {
        uint64 data_slot = *(slot_ptr + capacity);
        return get_low_32(slot) == surr2 ? get_low_32(data_slot) : get_high_32(data_slot);
      }
    }
  }

  return 0xFFFFFFFF;
}

uint32 loaded_one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr, uint32 *values, uint32 *data) {
  assert(values != NULL && data != NULL);

  uint32 capacity = table->capacity;

  if (surr >= capacity)
    return 0;

  uint64 *slot_ptr = table->column + surr;
  uint64 slot = *slot_ptr;

  if (loaded_one_way_bin_table_slot_is_empty(slot))
    return 0;

  if (loaded_one_way_bin_table_slot_is_index(slot)) {
    loaded_overflow_table_copy(&table->array_pool, slot, values, data, 0);
    return get_count(slot);
  }

  uint64 data_slot = *(slot_ptr + capacity);

  values[0] = get_low_32(slot);
  data[0] = get_low_32(data_slot);

  uint32 high = get_high_32(slot);
  if (high == EMPTY_MARKER)
    return 1;

  values[1] = high;
  data[1] = get_high_32(data_slot);
  return 2;
}

UINT32_ARRAY loaded_one_way_bin_table_range_restrict(ONE_WAY_BIN_TABLE *table, uint32 key, uint32 first, uint32 *output, uint32 output_capacity) {
  assert(output_capacity == 64);

  UINT32_ARRAY result;

  uint32 table_capacity = table->capacity;

  if (key < table_capacity) {
    uint64 *slot_ptr = table->column + key;
    uint64 slot = *slot_ptr;

    if (!loaded_one_way_bin_table_slot_is_empty(slot)) {
      if (loaded_one_way_bin_table_slot_is_index(slot)) {
        result = loaded_overflow_table_range_copy(&table->array_pool, slot, first, output, output_capacity);
      }
      else {
        uint64 data_slot = *(slot_ptr + table_capacity);

        output[0] = get_low_32(slot);
        output[2] = get_low_32(data_slot);

        uint32 high = get_high_32(slot);
        if (high != EMPTY_MARKER) {
          output[1] = high;
          output[3] = get_high_32(data_slot);
          result.size = 2;
        }
        else
          result.size = 1;

        result.offset = 2;
        result.array = output;
      }
    }
    else
      result.size = 0;
  }
  else
    result.size = 0;

  return result;
}

void loaded_one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;

  if (surr1 >= capacity) {
    loaded_one_way_bin_table_resize(table, surr1, mem_pool);
    capacity = table->capacity;
  }

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  uint64 *data_slot_ptr = slot_ptr + capacity;

  table->count++;

  if (loaded_one_way_bin_table_slot_is_empty(slot)) {
    *slot_ptr = pack(surr2, EMPTY_MARKER);
    *data_slot_ptr = pack(data, 0);
    return;
  }

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  uint64 updated_slot;

  if (get_tag(low) == INLINE_SLOT) {
    if (high == EMPTY_MARKER) {
      assert(surr2 != low);
      *slot_ptr = pack(low, surr2);
      set_high_32(data_slot_ptr, data);
      return;
    }
    else {
      assert(get_tag(low) == 0 && get_tag(high) == 0);
      uint64 data_slot = *(slot_ptr + capacity);
      updated_slot = loaded_overflow_table_create_new_block(&table->array_pool, slot, data_slot, surr2, data, mem_pool);
    }
  }
  else
    updated_slot = loaded_overflow_table_insert_unique(&table->array_pool, slot, surr2, data, mem_pool);

  assert(updated_slot != slot);
  *slot_ptr = updated_slot;
}

uint32 loaded_one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  uint32 capacity = table->capacity;

  if (surr1 < capacity) {
    uint64 *slot_ptr = table->column + surr1;
    uint64 slot = *slot_ptr;

    if (!loaded_one_way_bin_table_slot_is_empty(slot)) {
      uint64 *data_slot_ptr = slot_ptr + capacity;

      if (loaded_one_way_bin_table_slot_is_index(slot)) {
        uint32 data = loaded_overflow_table_lookup(&table->array_pool, slot, surr2);
        if (data != 0xFFFFFFFF) {
          uint64 updated_slot = loaded_overflow_table_delete(&table->array_pool, slot, surr2, data_slot_ptr);
          assert(updated_slot != slot);
          *slot_ptr = updated_slot;
          table->count--;
          return data;
        }
      }
      else {
        assert(get_tag(get_low_32(slot)) == INLINE_SLOT);

        uint32 low = get_low_32(slot);
        uint32 high = get_high_32(slot);

        if (surr2 == low) {
          uint64 data_slot = *data_slot_ptr;
          if (high == EMPTY_MARKER) {
            *slot_ptr = EMPTY_SLOT;
          }
          else {
            *slot_ptr = pack(high, EMPTY_MARKER);
            *data_slot_ptr = pack(get_high_32(data_slot), 0);
          }
          table->count--;
          return get_low_32(data_slot);
        }

        if (surr2 == high) {
          *slot_ptr = pack(low, EMPTY_MARKER);
          table->count--;
          return get_high_32(*data_slot_ptr);
        }
      }
    }
  }

  return 0xFFFFFFFF;
}

void loaded_one_way_bin_table_delete_existing(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  assert(one_way_bin_table_contains(table, surr1, surr2));
  assert(surr1 < table->capacity);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  assert(!loaded_one_way_bin_table_slot_is_empty(slot));

  uint32 capacity = table->capacity;
  uint64 *data_slot_ptr = slot_ptr + capacity;

  if (loaded_one_way_bin_table_slot_is_index(slot)) {
    uint64 updated_slot = loaded_overflow_table_delete(&table->array_pool, slot, surr2, data_slot_ptr);
    assert(updated_slot != slot);
    *slot_ptr = updated_slot;
    table->count--;
  }
  else {
    assert(get_tag(get_low_32(slot)) == INLINE_SLOT);

    uint32 low = get_low_32(slot);
    uint32 high = get_high_32(slot);

    if (surr2 == low) {
      uint64 data_slot = *data_slot_ptr;
      if (high == EMPTY_MARKER) {
        *slot_ptr = EMPTY_SLOT;
      }
      else {
        *slot_ptr = pack(high, EMPTY_MARKER);
        *data_slot_ptr = pack(get_high_32(data_slot), 0);
      }
      table->count--;
    }
    else {
      assert(surr2 == high);
      *slot_ptr = pack(low, EMPTY_MARKER);
      table->count--;
    }
  }
}

void loaded_one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2, uint32 *data) {
  uint32 capacity = table->capacity;

  if (surr1 >= capacity)
    return;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (loaded_one_way_bin_table_slot_is_empty(slot))
    return;

  *slot_ptr = EMPTY_SLOT;

  if (loaded_one_way_bin_table_slot_is_index(slot)) {
    uint32 slot_count = get_count(slot);
    loaded_overflow_table_copy(&table->array_pool, slot, surrs2, data, 0);
    loaded_overflow_table_delete(&table->array_pool, slot);
    table->count -= slot_count;
  }
  else {
    uint64 data_slot;
    assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
    surrs2[0] = get_low_32(slot);
    if (data != NULL) {
      data_slot = *(slot_ptr + capacity);
      data[0] = get_low_32(data_slot);
    }
    uint32 high = get_high_32(slot);
    if (high != EMPTY_MARKER) {
      surrs2[1] = high;
      if (data != NULL)
        data[1] = get_high_32(data_slot);
      table->count -= 2;
    }
    else
      table->count--;
  }
}
