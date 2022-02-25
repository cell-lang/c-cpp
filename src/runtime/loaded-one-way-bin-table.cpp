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
  uint64 *new_slots = extend_state_mem_uint64_array(mem_pool, table->column, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    new_slots[i] = EMPTY_SLOT;
  table->capacity = new_capacity;
  table->column = new_slots;
}

//////////////////////////////////////////////////////////////////////////////

void loaded_one_way_bin_table_init(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  array_mem_pool_init(&table->array_pool, true, mem_pool);
  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, MIN_CAPACITY);
  for (uint32 i=0 ; i < MIN_CAPACITY ; i++)
    slots[i] = EMPTY_SLOT;
  table->column = slots;
  table->capacity = MIN_CAPACITY;
  table->count = 0;
}

//////////////////////////////////////////////////////////////////////////////

uint32 loaded_one_way_bin_table_payload(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 >= table->capacity)
    return false;

  uint64 slot = table->column[surr1];

  if (is_empty(slot))
    return false;

  if (is_index(slot))
    return overflow_table_contains(&table->array_pool, slot, surr2);

  if (get_low_32(slot) == surr2)
    return true;

  return get_high_32(slot) == surr2;
}

uint32 loaded_one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr, uint32 *dest, uint32 *data) {
  if (surr >= table->capacity)
    return 0;

  uint64 slot = table->column[surr];

  if (is_empty(slot))
    return 0;

  if (is_index(slot)) {
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

void loaded_one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, uint32 data, STATE_MEM_POOL *mem_pool) {
  if (surr1 >= table->capacity)
    resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot)) {
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

// // Assuming there's at most one entry whose first argument is surr1
// uint32 loaded_one_way_bin_table_update(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool) {
//   if (surr1 >= table->capacity)
//     resize(table, surr1, mem_pool);

//   uint64 *slot_ptr = table->column + surr1;
//   uint64 slot = *slot_ptr;

//   if (is_empty(slot)) {
//     *slot_ptr = pack(surr2, EMPTY_MARKER);
//     table->count++;
//     return -1;
//   }

//   uint32 low = get_low_32(slot);
//   uint32 high = get_high_32(slot);

//   if (get_tag(low) == INLINE_SLOT & high == EMPTY_MARKER) {
//     *slot_ptr = pack(surr2, EMPTY_MARKER);
//     return low;
//   }

//   internal_fail();
// }

uint32 loaded_one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 >= table->capacity)
    return false;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot))
    return false;

  if (is_index(slot)) {
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

void loaded_one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2) {
  if (surr1 >= table->capacity)
    return;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot))
    return;

  *slot_ptr = EMPTY_SLOT;

  if (is_index(slot)) {
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

void loaded_one_way_bin_table_clear(ONE_WAY_BIN_TABLE *table) {
  impl_fail(NULL); //## IMPLEMENT IMPLEMENT IMPLEMENT
}
