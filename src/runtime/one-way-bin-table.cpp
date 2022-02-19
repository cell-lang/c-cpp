#include "lib.h"
#include "one-way-bin-table.h"


// Valid slot states:
//   - Value + payload: 32 bit payload - 3 zeros   - 29 bit value
//   - Index + count:   32 bit count   - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:           32 zeros - ArraySliceAllocator.EMPTY_MARKER == 0xFFFFFFFF
//     This type of slot can only be stored in a block, but cannot be passed in or out


const uint32 MIN_CAPACITY = 16;

//////////////////////////////////////////////////////////////////////////////

static bool is_empty(uint64 slot) {
  return slot == EMPTY_SLOT;
}

static bool is_index(uint64 slot) {
  return slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT;
}

//////////////////////////////////////////////////////////////////////////////

static void resize(ONE_WAY_BIN_TABLE *table, uint32 index, STATE_MEM_POOL *mem_pool) {
  uint32 capacity = table->capacity;
  uint32 new_capacity = capacity == 0 ? MIN_CAPACITY : 2 * capacity;
  while (index >= new_capacity)
    new_capacity *= 2;
  uint64 *new_slots = extend_state_mem_uint64_array(mem_pool, table->column, capacity, new_capacity);
  for (uint32 i=capacity ; i < new_capacity ; i++)
    new_slots[i] = EMPTY_SLOT;
  table->capacity = new_capacity;
  table->column = new_slots;
}

//////////////////////////////////////////////////////////////////////////////

bool one_way_bin_table_contains(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2) {
  uint32 capacity = table->capacity;

  if (surr1 >= capacity)
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

bool one_way_bin_table_contains_key(ONE_WAY_BIN_TABLE *table, uint32 surr1) {
  return surr1 < table->capacity && !is_empty(table->column[surr1]);
}

// uint32[] one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr) {
//   if (surr >= table->capacity)
//     return Array.emptyIntArray;

//   uint64 slot = table->column[surr];

//   if (is_empty(slot))
//     return Array.emptyIntArray;

//   if (is_index(slot)) {
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

uint32 one_way_bin_table_lookup(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= table->capacity)
    return -1;
  uint64 slot = table->column[surr];
  if (is_empty(slot))
    return -1;
  if (is_index(slot) | get_high_32(slot) != EMPTY_MARKER)
    internal_fail();
  assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
  return get_low_32(slot);
}

uint32 one_way_bin_table_get_count(ONE_WAY_BIN_TABLE *table, uint32 surr) {
  if (surr >= table->capacity)
    return 0;
  uint64 slot = table->column[surr];
  if (is_empty(slot))
    return 0;
  if (is_index(slot))
    return get_count(slot);
  return get_high_32(slot) == EMPTY_MARKER ? 1 : 2;
}

bool one_way_bin_table_insert(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool) {
  if (surr1 >= table->capacity)
    resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot)) {
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

// Assuming there's at most one entry whose first argument is surr1
uint32 one_way_bin_table_update(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool) {
  if (surr1 >= table->capacity)
    resize(table, surr1, mem_pool);

  uint64 *slot_ptr = table->column + surr1;
  uint64 slot = *slot_ptr;

  if (is_empty(slot)) {
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

void one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2) {
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

bool one_way_bin_table_is_map(ONE_WAY_BIN_TABLE *table) {
  uint32 capacity = table->capacity;
  uint64 *slots = table->column;
  for (uint32 i=0 ; i < capacity ; i++) {
    uint64 slot = slots[i];
    if (!is_empty(slot) & (get_tag(get_low_32(slot)) != INLINE_SLOT | get_high_32(slot) != EMPTY_MARKER))
      return false;
  }
  return true;
}

void one_way_bin_table_copy(ONE_WAY_BIN_TABLE *table, uint32 *dest) {
  uint32 capacity = table->capacity;
  uint64 *slots = table->column;

  uint32 inline_buffer[1024];
  uint32 *buffer = inline_buffer;
  uint32 buffer_size = 1024;

  uint32 next = 0;
  for (uint32 i=0 ; i < capacity ; i++) {
    uint64 slot = slots[i];
    if (!is_empty(slot)) {
      if (is_index(slot)) {
        uint32 slot_count = get_count(slot);
        if (slot_count > buffer_size) {
          do
            buffer_size *= 2;
          while (slot_count > buffer_size);
          buffer = new_uint32_array(buffer_size);
        }
        overflow_table_copy(&table->array_pool, slot, buffer, 0);
        for (uint32 j=0 ; j < slot_count ; j++) {
          uint32 idx = next + 2 * j;
          dest[idx] = i;
          dest[idx + 1] = buffer[j];
        }
        next += 2 * slot_count;
      }
      else {
        dest[next++] = i;
        dest[next++] = get_low_32(slot);
        uint32 high = get_high_32(slot);
        if (high != EMPTY_MARKER) {
          dest[next++] = i;
          dest[next++] = high;
        }
      }
    }
  }
  assert(next == 2 * table->count);
}

// uint32[] one_way_bin_table_copySym(ONE_WAY_BIN_TABLE *table, uint32 eqCount) {
//   uint32[] data = new uint32[count+eqCount];

//   uint32[] buffer = new uint32[32];

//   uint32 next = 0;
//   for (uint32 surr1 = 0 ; surr1 < column.length ; surr1++) {
//     uint64 slot = column[surr1];
//     if (!is_empty(slot)) {
//       if (is_index(slot)) {
//         uint32 slot_count = get_count(slot);
//         if (slot_count > buffer.length)
//           buffer = new uint32[Array.capacity(buffer.length, slot_count)];
//         uint32 _count = restrict(surr1, buffer);
//         assert(_count == slot_count);
//         for (uint32 i=0 ; i < slot_count ; i++) {
//           uint32 surr2 = buffer[i];
//           if (surr1 <= surr2) {
//             data[next++] = surr1;
//             data[next++] = surr2;
//           }
//         }
//       }
//       else {
//         uint32 low = get_low_32(slot);
//         uint32 high = get_high_32(slot);
//         if (surr1 <= low) {
//           data[next++] = surr1;
//           data[next++] = get_low_32(slot);
//         }
//         if (high != EMPTY_MARKER & surr1 <= high) {
//           data[next++] = surr1;
//           data[next++] = high;
//         }
//       }
//     }
//   }
//   assert(next == count + eqCount);
//   return data;
// }

//////////////////////////////////////////////////////////////////////////////

// void one_way_bin_table_initReverse(ONE_WAY_BIN_TABLE *table, OneWayBinTable source) {
//   assert(count == 0);

//   uint32 len = source.column.length;
//   for (uint32 i=0 ; i < len ; i++) {
//     uint32[] surrs = source.restrict(i);
//     for (uint32 j=0 ; j < surrs.length ; j++)
//       insert(surrs[j], i);
//   }
// }

// void one_way_bin_table_initReverse(ONE_WAY_BIN_TABLE *table, LoadedOneWayBinTable source) {
//   assert(count == 0);

//   uint32 len = source.column.length;
//   for (uint32 i=0 ; i < len ; i++) {
//     uint32[] surrs = source.restrict(i);
//     for (uint32 j=0 ; j < surrs.length ; j++)
//       insert(surrs[j], i);
//   }
// }

//////////////////////////////////////////////////////////////////////////////


// public void check() {
//   overflowTable.check(column, count);
// }

// public void dump() {
//   System.out.println("count = " + Integer.toString(count));
//   System.out.print("column = [");
//   for (uint32 i=0 ; i < column.length ; i++)
//     System.out.printf("%s%X", i > 0 ? " " : "", column[i]);
//   System.out.println("]");
//   overflowTable.dump();
// }

