#include "lib.h"
#include "one-way-bin-table.h"


// Valid slot states:
//   - Value + payload: 32 bit payload - 3 zeros   - 29 bit value
//   - Index + count:   32 bit count   - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:           32 zeros - ArraySliceAllocator.EMPTY_MARKER == 0xFFFFFFFF
//     This type of slot can only be stored in a block, but cannot be passed in or out


const uint32 MIN_CAPACITY = 256;

//////////////////////////////////////////////////////////////////////////////

static bool is_empty(uint64 slot) {
  return slot == EMPTY_SLOT;
}

static bool is_index(uint64 slot) {
  return slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT;
}

//////////////////////////////////////////////////////////////////////////////

static void resize(ONE_WAY_BIN_TABLE *table, uint32 index, STATE_MEM_POOL *mem_pool) {
  assert(table->capacity >= MIN_CAPACITY);
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

void loaded_one_way_bin_table_init(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  array_mem_pool_init(&table->array_pool, true, mem_pool);
  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, 2 * MIN_CAPACITY);
  for (uint32 i=0 ; i < MIN_CAPACITY ; i++)
    slots[i] = EMPTY_SLOT;
  table->column = slots;
  table->capacity = MIN_CAPACITY;
  table->count = 0;
}

//////////////////////////////////////////////////////////////////////////////

uint32 loaded_one_way_bin_table_payload(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  uint32 capacity = table->capacity;

  if (surr1 < capacity) {
    uint64 *slot_ptr = table->column + surr1;
    uint64 slot = *slot_ptr;

    if (!is_empty(slot)) {
      if (is_index(slot))
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
  uint32 capacity = table->capacity;

  if (surr >= capacity)
    return 0;

  uint64 *slot_ptr = table->column + surr;
  uint64 slot = *slot_ptr;

  if (is_empty(slot))
    return 0;

  if (is_index(slot)) {
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

void loaded_one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;

  if (surr1 >= capacity)
    resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  uint64 *data_slot_ptr = slot_ptr + capacity;

  table->count++;

  if (is_empty(slot)) {
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

bool loaded_one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  uint32 capacity = table->capacity;

  if (surr1 >= capacity)
    return false;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot))
    return false;

  uint64 *data_slot_ptr = slot_ptr + capacity;

  if (is_index(slot)) {
    uint64 updated_slot = loaded_overflow_table_delete(&table->array_pool, slot, surr2, data_slot_ptr);
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
    if (high == EMPTY_MARKER) {
      *slot_ptr = EMPTY_SLOT;
    }
    else {
      *slot_ptr = pack(high, EMPTY_MARKER);
      uint64 data_slot = *data_slot_ptr;
      *data_slot_ptr = pack(get_high_32(data_slot), 0);
    }
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

void loaded_one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2, uint32 *data) {
  uint32 capacity = table->capacity;

  if (surr1 >= capacity)
    return;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot))
    return;

  *slot_ptr = EMPTY_SLOT;

  if (is_index(slot)) {
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

void loaded_one_way_bin_table_clear(ONE_WAY_BIN_TABLE *table) {
  impl_fail(NULL); //## IMPLEMENT IMPLEMENT IMPLEMENT
}
