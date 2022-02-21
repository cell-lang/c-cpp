#include "lib.h"
#include "one-way-bin-table.h"


// Valid slot states:
//   - Value + payload: 32 bit payload - 3 zeros   - 29 bit value
//   - Index + count:   32 bit count   - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:           32 zeros - ArraySliceAllocator.EMPTY_MARKER == 0xFFFFFFFF
//     This type of slot can only be stored in a block, but cannot be passed in or out

//////////////////////////////////////////////////////////////////////////////

//## SAME AS THE ONE FOR VANILLA ONE-WAY BINARY TABLES
inline bool is_empty(uint64 slot) {
  return slot == EMPTY_SLOT;
}

//## SAME AS THE ONE FOR VANILLA ONE-WAY BINARY TABLES
inline bool is_index(uint64 slot) {
  return slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT;
}

//////////////////////////////////////////////////////////////////////////////

//## SAME AS THE ONE FOR VANILLA ONE-WAY BINARY TABLES
static void resize(ONE_WAY_BIN_TABLE *table, uint32 index, STATE_MEM_POOL *mem_pool) {
  assert(table->capacity > 0);
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

//## SAME AS THE ONE FOR VANILLA ONE-WAY BINARY TABLES
void loaded_one_way_bin_table_init(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_init(table, mem_pool);
}

//////////////////////////////////////////////////////////////////////////////

bool loaded_one_way_bin_table_contains(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  return payload(surr1, surr2) != 0xFFFFFFFF;
}

bool loaded_one_way_bin_table_contains_key(ONE_WAY_BIN_TABLE *table, uint32 surr1) {
  return surr1 < column.length && !is_empty(column[surr1]);
}

uint32 loaded_one_way_bin_table_payload(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 < column.length) {
    uint64 slot = column[surr1];
    if (!is_empty(slot)) {
      if (is_index(slot))
        return loaded_overflow_table_lookup(&table->array_pool, slot, surr2);
      else if (low(slot) == surr2)
        return high(slot);
    }
  }
  return 0xFFFFFFFF;
}

uint32[] loaded_one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= column.length)
    return Array.emptyIntArray;

  uint64 slot = column[surr];

  if (is_empty(slot))
    return Array.emptyIntArray;

  if (is_index(slot)) {
    uint32 count = count(slot);
    uint32[] surrs_idxs = new uint32[count];
    loaded_overflow_table_copy(&table->array_pool, slot, surrs_idxs, null);
    return surrs_idxs;
  }

  return new uint32[] {low(slot)};
}

uint32 loaded_one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr, uint32[] output) {
  if (surr >= column.length)
    return 0;

  uint64 slot = column[surr];

  if (is_empty(slot))
    return 0;

  if (is_index(slot)) {
    loaded_overflow_table_copy(&table->array_pool, slot, output, null);
    return count(slot);
  }

  output[0] = low(slot);
  return 1;
}

uint32 loaded_one_way_bin_table_lookup(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= column.length)
    return -1;
  uint64 slot = column[surr];
  if (is_empty(slot))
    return -1;
  if (is_index(slot))
    internal_fail(); //## internal_fail() OR soft_fail()?
  assert(tag(low(slot)) == INLINE_SLOT);
  return low(slot);
}

uint32 loaded_one_way_bin_table_count(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= column.length)
    return 0;
  uint64 slot = column[surr];
  if (is_empty(slot))
    return 0;
  if (is_index(slot))
    return count(slot);
  return 1;
}

//////////////////////////////////////////////////////////////////////////////

void loaded_one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 size = column.length;
  if (surr1 >= size)
    resize(surr1);

  uint64 slot = column[surr1];

  if (is_empty(slot)) {
    set(surr1, surr2, data);
    count++;
    return;
  }

  assert(low(slot) != surr2);

  uint64 updated_slot = loaded_overflow_table_insert_unique(&table->array_pool, slot, surr2, data, mem_pool);
  assert(updated_slot != slot);

  set(surr1, updated_slot);
  count++;
}

uint32 loaded_one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 >= column.length)
    return 0xFFFFFFFF;

  uint64 slot = column[surr1];

  if (is_empty(slot))
    return 0xFFFFFFFF;

  if (is_index(slot)) {
    uint64 updated_slot = loaded_overflow_table_delete(&table->array_pool, slot, surr2, _data);
    if (updated_slot != slot) {
      set(surr1, updated_slot);
      count--;
      return _data[0];
    }
    else
      return 0xFFFFFFFF;
  }

  assert(tag(low(slot)) == INLINE_SLOT);

  if (low(slot) == surr2) {
    uint32 data = high(slot);
    set(surr1, EMPTY_SLOT);
    count--;
    return data;
  }
  else
    return 0xFFFFFFFF;
}

void loaded_one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32[] surrs2, uint32[] data) {
  if (surr1 >= column.length)
    return;

  uint64 slot = column[surr1];

  if (is_empty(slot))
    return;

  set(surr1, EMPTY_SLOT);

  if (is_index(slot)) {
    uint32 slot_count = count(slot);
    loaded_overflow_table_copy(&table->array_pool, slot, surrs2, data);
    loaded_overflow_table_delete(&table->array_pool, slot);
    count -= slot_count;
  }
  else {
    assert(tag(low(slot)) == INLINE_SLOT);
    surrs2[0] = low(slot);
    data[0] = high(slot);
    count--;
  }
}

bool loaded_one_way_bin_table_is_map(ONE_WAY_BIN_TABLE *table, ) {
  for (uint32 i=0 ; i < column.length ; i++)
    if (is_index(column[i]))
      return false;
  return true;
}

uint32[] loaded_one_way_bin_table_copy(ONE_WAY_BIN_TABLE *table, ) {
  uint32[] data = new uint32[2 * count];
  uint32 next = 0;
  for (uint32 i=0 ; i < column.length ; i++) {
    uint64 slot = column[i];
    if (!is_empty(slot)) {
      if (is_index(slot)) {
        uint32 slot_count = count(slot);
        for (uint32 j=0 ; j < slot_count ; j++)
          data[next+2*j] = i;
        loaded_overflow_table_copy(&table->array_pool, slot, data, null, next + 1, 2);
        next += 2 * slot_count;
      }
      else {
        data[next++] = i;
        data[next++] = low(slot);
      }
    }
  }
  assert(next == 2 * count);
  return data;
}
