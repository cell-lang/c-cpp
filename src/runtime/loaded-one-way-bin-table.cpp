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

void loaded_one_way_bin_table_init(ONE_WAY_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_init(table, mem_pool);
}

//////////////////////////////////////////////////////////////////////////////

bool loaded_one_way_bin_table_contains_key(ONE_WAY_BIN_TABLE *table, uint32 surr1) {
  return surr1 < table->capacity && !is_empty(table->column[surr1]);
}

uint32 loaded_one_way_bin_table_payload(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 < table->capacity) {
    uint64 slot = table->column[surr1];
    if (!is_empty(slot)) {
      if (is_index(slot))
        return loaded_overflow_table_lookup(&table->array_pool, slot, surr2);
      else if (get_low_32(slot) == surr2)
        return get_high_32(slot);
    }
  }
  return 0xFFFFFFFF;
}

bool loaded_one_way_bin_table_contains(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  return loaded_one_way_bin_table_payload(table, surr1, surr2) != 0xFFFFFFFF;
}

uint32 loaded_one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr, uint32 *dest) {
  if (surr >= table->capacity)
    return 0;

  uint64 slot = table->column[surr];

  if (is_empty(slot))
    return 0;

  if (is_index(slot)) {
    loaded_overflow_table_copy(&table->array_pool, slot, dest, NULL, 0);
    return get_count(slot);
  }

  dest[0] = get_low_32(slot);
  return 1;
}

uint32 loaded_one_way_bin_table_lookup(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= table->capacity)
    return -1;
  uint64 slot = table->column[surr];
  if (is_empty(slot))
    return -1;
  if (is_index(slot))
    internal_fail(); //## internal_fail() OR soft_fail()?
  assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
  return get_low_32(slot);
}

uint32 loaded_one_way_bin_table_count(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= table->capacity)
    return 0;
  uint64 slot = table->column[surr];
  if (is_empty(slot))
    return 0;
  if (is_index(slot))
    return get_count(slot);
  return 1;
}

//////////////////////////////////////////////////////////////////////////////

void loaded_one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 size = table->capacity;
  if (surr1 >= size)
    resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = table->column[surr1];

  if (is_empty(slot)) {
    *slot_ptr = pack(surr2, data);
    table->count++;
    return;
  }

  assert(get_low_32(slot) != surr2);

  uint64 updated_slot = loaded_overflow_table_insert_unique(&table->array_pool, slot, surr2, data, mem_pool);
  assert(updated_slot != slot);

  *slot_ptr = updated_slot;
  table->count++;
}

uint32 loaded_one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  if (surr1 >= table->capacity)
    return 0xFFFFFFFF;

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot))
    return 0xFFFFFFFF;

  if (is_index(slot)) {
    uint32 data;
    uint64 updated_slot = loaded_overflow_table_delete(&table->array_pool, slot, surr2, &data);
    if (updated_slot != slot) {
      *slot_ptr = updated_slot;
      table->count--;
      return data;
    }
    else
      return 0xFFFFFFFF;
  }

  assert(get_tag(get_low_32(slot)) == INLINE_SLOT);

  if (get_low_32(slot) == surr2) {
    uint32 data = get_high_32(slot);
    *slot_ptr = EMPTY_SLOT;
    table->count--;
    return data;
  }
  else
    return 0xFFFFFFFF;
}

void loaded_one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2, uint32 *data) {
  if (surr1 >= table->capacity)
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
    assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
    surrs2[0] = get_low_32(slot);
    data[0] = get_high_32(slot);
    table->count--;
  }
}

void loaded_one_way_bin_table_clear(ONE_WAY_BIN_TABLE *table) {
  one_way_bin_table_clear(table); //## NOT SURE HERE
}

bool loaded_one_way_bin_table_is_map(ONE_WAY_BIN_TABLE *table) {
  uint32 capacity = table->capacity;
  uint64 *slots = table->column;
  for (uint32 i=0 ; i < capacity ; i++)
    if (is_index(slots[i]))
      return false;
  return true;
}

//## NOT PORTED
// void loaded_one_way_bin_table_copy(ONE_WAY_BIN_TABLE *table, uint32 *data) {
//   uint32 capacity = table->capacity;
//   uint64 *slots = table->column;
//   uint32 next = 0;
//   for (uint32 i=0 ; i < capacity ; i++) {
//     uint64 slot = slots[i];
//     if (!is_empty(slot)) {
//       if (is_index(slot)) {
//         uint32 slot_count = get_count(slot);
//         for (uint32 j=0 ; j < slot_count ; j++)
//           data[next + 2 * j] = i;
//         loaded_overflow_table_copy(&table->array_pool, slot, data, null, next + 1, 2);
//         next += 2 * slot_count;
//       }
//       else {
//         data[next++] = i;
//         data[next++] = get_low_32(slot);
//       }
//     }
//   }
//   assert(next == 2 * count);
//   return data;
// }
