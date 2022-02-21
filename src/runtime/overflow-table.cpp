#include "lib.h"
#include "one-way-bin-table.h"


// A slot can be in any of the following states:
//   - Single value:        32 ones                  - 3 zeros   - 29 bit value 1
//   - Two values:          3 zeros - 29 bit value 2 - 3 zeros   - 29 bit value 1
//   - Index + count:       32 bit count             - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:               32 zeros                 - 32 ones
//     This type of slot can only be stored in a block, but cannot be passed in or out


// const uint32 INLINE_SLOT    = 0; // Moved to header file
const uint32 SIZE_2_BLOCK   = 1;
const uint32 SIZE_4_BLOCK   = 2;
const uint32 SIZE_8_BLOCK   = 3;
const uint32 SIZE_16_BLOCK  = 4;
const uint32 HASHED_BLOCK   = 5;

const uint32 SIZE_2_BLOCK_MIN_COUNT   = 3;
const uint32 SIZE_4_BLOCK_MIN_COUNT   = 4;
const uint32 SIZE_8_BLOCK_MIN_COUNT   = 7;
const uint32 SIZE_16_BLOCK_MIN_COUNT  = 13;
const uint32 HASHED_BLOCK_MIN_COUNT   = 13;

////////////////////////////////////////////////////////////////////////////

static uint32 get_capacity(uint32 tag) {
  assert(tag >= SIZE_2_BLOCK & tag <= SIZE_16_BLOCK);
  assert(SIZE_2_BLOCK == 1 | SIZE_16_BLOCK == 4);
  return 2 << tag;
}

//## CANNOT BE STATIC BECAUSE IT'S NEDEED IN one-way-bin-table.cpp
// static uint32 get_count(uint64 slot) {
uint32 get_count(uint64 slot) {
  assert(get_tag(get_low_32(slot)) >= SIZE_2_BLOCK & get_tag(get_low_32(slot)) <= HASHED_BLOCK);
  // assert(get_high_32(slot) > 2); // Not true when initializing a hashed block
  return get_high_32(slot);
}

static uint64 linear_block_handle(uint32 tag, uint32 index, uint32 count) {
  return pack(pack_tag_payload(tag, index), count);
}

static uint64 size_2_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_2_BLOCK_MIN_COUNT & count <= 4);
  return pack(pack_tag_payload(SIZE_2_BLOCK, index), count);
}

static uint64 size_4_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_4_BLOCK_MIN_COUNT & count <= 8);
  return pack(pack_tag_payload(SIZE_4_BLOCK, index), count);
}

static uint64 size_8_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_8_BLOCK_MIN_COUNT & count <= 16);
  return pack(pack_tag_payload(SIZE_8_BLOCK, index), count);
}

static uint64 size_16_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_16_BLOCK_MIN_COUNT & count <= 32);
  return pack(pack_tag_payload(SIZE_16_BLOCK, index), count);
}

static uint64 hashed_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  // assert(count >= 7); // Not true when initializing a hashed block
  uint64 handle = pack(pack_tag_payload(HASHED_BLOCK, index), count);
  assert(get_tag(get_low_32(handle)) == HASHED_BLOCK);
  assert(get_payload(get_low_32(handle)) == index);
  assert(get_count(handle) == count);
  return handle;
}

static uint32 get_index(uint32 value) {
  assert(get_tag(value) == INLINE_SLOT);
  return value & 0xF;
}

static uint32 clipped(uint32 value) {
  return value >> 4;
}

static uint32 unclipped(uint32 value, uint32 index) {
  assert(get_tag(value) == 0);
  assert(get_tag(value << 4) == 0);
  assert(index >= 0 & index < 16);
  return (value << 4) | index;
}

static uint32 min_count(uint32 tag) {
  if (tag == SIZE_2_BLOCK)
    return SIZE_2_BLOCK_MIN_COUNT;

  if (tag == SIZE_4_BLOCK)
    return SIZE_4_BLOCK_MIN_COUNT;

  if (tag == SIZE_8_BLOCK)
    return SIZE_8_BLOCK_MIN_COUNT;

  assert(tag == SIZE_16_BLOCK | tag == HASHED_BLOCK);
  assert(SIZE_16_BLOCK_MIN_COUNT == HASHED_BLOCK_MIN_COUNT);

  return SIZE_16_BLOCK_MIN_COUNT; // Same as HASHED_BLOCK_MIN_COUNT
}

static bool is_even(uint32 value) {
  return (value % 2) == 0;
}

////////////////////////////////////////////////////////////////////////////

static uint64 insert_2_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool) {
  uint32 low = get_low_32(handle);
  uint32 high = get_high_32(handle);

  assert(get_tag(low) == 0 & get_tag(high) == 0);

  // Checking for duplicates
  if (low == value | high == value)
    return handle;

  uint32 block_idx = array_mem_pool_alloc_2_block(array_pool, mem_pool);
  uint64 *slots = array_pool->slots;
  slots[block_idx]     = pack(low,   high);
  slots[block_idx + 1] = pack(value, EMPTY_MARKER);
  return size_2_block_handle(block_idx, 3);
}

static uint64 insert_into_hashed_block(ARRAY_MEM_POOL *, uint32, uint32, uint32, STATE_MEM_POOL *);

static uint64 insert_with_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool) {
  assert(get_tag(get_low_32(handle)) >= SIZE_2_BLOCK & get_tag(get_low_32(handle)) <= SIZE_16_BLOCK);

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);
  uint32 count = get_count(handle);
  uint32 end = (count + 1) / 2;

  uint64 *slots = array_pool->slots;

  // Checking for duplicates and inserting if the next free block is a high one
  for (uint32 i=0 ; i < end ; i++) {
    uint64 slot = slots[block_idx + i];
    uint32 slot_low = get_low_32(slot);
    uint32 slot_high = get_high_32(slot);
    if (value == slot_low | value == slot_high)
      return handle;
    if (slot_high == EMPTY_MARKER) {
      slots[block_idx + i] = pack(slot_low, value);
      return linear_block_handle(tag, block_idx, count + 1);
    }
  }

  uint32 capacity = get_capacity(tag);

  // Inserting the new value if there's still room here
  // It can only be in a low slot
  if (count < capacity) {
    slots[block_idx + end] = pack(value, EMPTY_MARKER);
    return linear_block_handle(tag, block_idx, count + 1);
  }

  if (tag != SIZE_16_BLOCK) {
    // Allocating the new block
    uint32 new_block_idx;
    if (tag == SIZE_2_BLOCK)
      new_block_idx = array_mem_pool_alloc_4_block(array_pool, mem_pool);
    else if (tag == SIZE_4_BLOCK)
      new_block_idx = array_mem_pool_alloc_8_block(array_pool, mem_pool);
    else
      new_block_idx = array_mem_pool_alloc_16_block(array_pool, mem_pool);

    // Reloading the pointer to the slot array, which may have changed with the previous allocation
    slots = array_pool->slots;

    // Initializing the new block
    uint32 idx = count / 2;
    for (uint32 i=0 ; i < idx ; i++)
      slots[new_block_idx + i] = slots[block_idx + i];
    slots[new_block_idx + idx] = pack(value, EMPTY_MARKER);
    for (uint32 i=idx+1 ; i < count ; i++)
      slots[new_block_idx + i] = pack(EMPTY_MARKER, 0);

    // Releasing the old block
    if (tag == SIZE_2_BLOCK)
      array_mem_pool_release_2_block(array_pool, block_idx);
    else if (tag == SIZE_4_BLOCK)
      array_mem_pool_release_4_block(array_pool, block_idx);
    else
      array_mem_pool_release_8_block(array_pool, block_idx);

    return linear_block_handle(tag + 1, new_block_idx, count + 1);
  }

  // Allocating and initializing the hashed block
  uint32 hashed_block_idx = array_mem_pool_alloc_16_block(array_pool, mem_pool);

  // Reloading the pointer to the slot array, which may have changed with the previous allocation
  slots = array_pool->slots;

  for (uint32 i=0 ; i < 16 ; i++)
    slots[hashed_block_idx + i] = pack(EMPTY_MARKER, 0);

  // Transferring the existing values
  for (uint32 i=0 ; i < 16 ; i++) {
    slots = array_pool->slots; // Refreshing the local variable
    uint64 slot = slots[block_idx + i];
    uint64 tmp_handle = insert_into_hashed_block(array_pool, hashed_block_idx, 2 * i, get_low_32(slot), mem_pool);
    assert(get_count(tmp_handle) == 2 * i + 1);
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
    tmp_handle = insert_into_hashed_block(array_pool, hashed_block_idx, 2 * i + 1, get_high_32(slot), mem_pool);
    assert(get_count(tmp_handle) == 2 * (i + 1));
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
  }

  // Releasing the old block
  array_mem_pool_release_16_block(array_pool, block_idx);

  // Adding the new value
  return insert_into_hashed_block(array_pool, hashed_block_idx, 32, value, mem_pool);
}

static uint64 insert_into_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, STATE_MEM_POOL *mem_pool) {
  uint64 *slots = array_pool->slots;

  uint32 slot_idx = block_idx + get_index(value);
  uint64 slot = slots[slot_idx];
  uint32 low = get_low_32(slot);

  // Checking for empty slots
  if (low == EMPTY_MARKER) {
    slots[slot_idx] = pack(value, EMPTY_MARKER);
    return hashed_block_handle(block_idx, count + 1);
  }

  uint32 tag = get_tag(low);

  // Checking for inline slots
  if (tag == INLINE_SLOT) {
    if (value == low)
      return hashed_block_handle(block_idx, count);
    uint32 high = get_high_32(slot);
    if (high == EMPTY_MARKER) {
      slots[slot_idx] = pack(low, value);
      return hashed_block_handle(block_idx, count + 1);
    }
    assert(get_tag(high) == INLINE_SLOT);
    if (value == high)
      return hashed_block_handle(block_idx, count);
    uint64 handle = insert_2_block(array_pool, pack(clipped(low), clipped(high)), clipped(value), mem_pool);
    assert(get_count(handle) == 3);
    slots = array_pool->slots; // Refreshing local copy
    slots[slot_idx] = handle;
    return hashed_block_handle(block_idx, count + 1);
  }

  // The slot is not an inline one. Inserting the clipped value into the subblock

  uint64 handle;
  if (tag == HASHED_BLOCK)
    handle = insert_into_hashed_block(array_pool, get_payload(low), get_count(slot), clipped(value), mem_pool);
  else
    handle = insert_with_linear_block(array_pool, slot, clipped(value), mem_pool);

  if (handle == slot)
    return hashed_block_handle(block_idx, count);

  assert(get_count(handle) == get_count(slot) + 1);
  slots = array_pool->slots; // Refreshing local copy
  slots[slot_idx] = handle;
  return hashed_block_handle(block_idx, count + 1);
}

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *, uint32, uint32, uint32, STATE_MEM_POOL *);

static uint64 insert_unique_with_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool) {
  assert(get_tag(get_low_32(handle)) >= SIZE_2_BLOCK & get_tag(get_low_32(handle)) <= SIZE_16_BLOCK);

  uint64 *slots = array_pool->slots;

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);
  uint32 count = get_count(handle);
  uint32 capacity = get_capacity(tag);

  // Inserting the new value if there's still room here
  if (count < capacity) {
    uint32 slot_idx = block_idx + count / 2;
    if (is_even(count))
      slots[slot_idx] = pack(value, EMPTY_MARKER);
    else
      slots[slot_idx] = set_high_32(slots[slot_idx], value);
    return linear_block_handle(tag, block_idx, count + 1);
  }

  if (tag != SIZE_16_BLOCK) {
    // Allocating the new block
    uint32 new_block_idx;
    if (tag == SIZE_2_BLOCK)
      new_block_idx = array_mem_pool_alloc_4_block(array_pool, mem_pool);
    else if (tag == SIZE_4_BLOCK)
      new_block_idx = array_mem_pool_alloc_8_block(array_pool, mem_pool);
    else
      new_block_idx = array_mem_pool_alloc_16_block(array_pool, mem_pool);

    // Reloading the pointer to the slot array, which may have changed with the previous allocation
    slots = array_pool->slots;

    // Initializing the new block
    uint32 idx = count / 2;
    for (uint32 i=0 ; i < idx ; i++)
      slots[new_block_idx + i] = slots[block_idx + i];
    slots[new_block_idx + idx] = pack(value, EMPTY_MARKER);
    for (uint32 i=idx+1 ; i < count ; i++)
      slots[new_block_idx + i] = pack(EMPTY_MARKER, 0);

    // Releasing the old block
    if (tag == SIZE_2_BLOCK)
      array_mem_pool_release_2_block(array_pool, block_idx);
    else if (tag == SIZE_4_BLOCK)
      array_mem_pool_release_4_block(array_pool, block_idx);
    else
      array_mem_pool_release_8_block(array_pool, block_idx);

    return linear_block_handle(tag + 1, new_block_idx, count + 1);
  }

  // Allocating and initializing the hashed block
  uint32 hashed_block_idx = array_mem_pool_alloc_16_block(array_pool, mem_pool);

  // Reloading the pointer to the slot array, which may have changed with the previous allocation
  slots = array_pool->slots;

  for (uint32 i=0 ; i < 16 ; i++)
    slots[hashed_block_idx + i] = pack(EMPTY_MARKER, 0);

  // Transferring the existing values
  for (uint32 i=0 ; i < 16 ; i++) {
    slots = array_pool->slots; // Refreshing the local copy
    uint64 slot = slots[block_idx + i];
    uint64 tmp_handle = insert_unique_into_hashed_block(array_pool, hashed_block_idx, 2 * i, get_low_32(slot), mem_pool);
    assert(get_count(tmp_handle) == 2 * i + 1);
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
    tmp_handle = insert_unique_into_hashed_block(array_pool, hashed_block_idx, 2 * i + 1, get_high_32(slot), mem_pool);
    assert(get_count(tmp_handle) == 2 * (i + 1));
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
  }

  // Releasing the old block
  array_mem_pool_release_16_block(array_pool, block_idx);

  // Adding the new value
  return insert_unique_into_hashed_block(array_pool, hashed_block_idx, 32, value, mem_pool);
}

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, STATE_MEM_POOL *mem_pool) {
  uint64 *slots = array_pool->slots;

  uint32 slot_idx = block_idx + get_index(value);
  uint64 slot = slots[slot_idx];
  uint32 low = get_low_32(slot);

  // Checking for empty slots
  if (low == EMPTY_MARKER) {
    slots[slot_idx] = pack(value, EMPTY_MARKER);
    return hashed_block_handle(block_idx, count + 1);
  }

  uint32 tag = get_tag(low);

  // Checking for inline slots
  if (tag == INLINE_SLOT) {
    assert(value != low);
    uint32 high = get_high_32(slot);
    if (high == EMPTY_MARKER) {
      slots[slot_idx] = pack(low, value);
      return hashed_block_handle(block_idx, count + 1);
    }
    assert(get_tag(high) == INLINE_SLOT);
    assert(value != high);
    uint64 handle = insert_2_block(array_pool, pack(clipped(low), clipped(high)), clipped(value), mem_pool);
    assert(get_count(handle) == 3);
    slots = array_pool->slots; // Refreshing the local copy
    slots[slot_idx] = handle;
    return hashed_block_handle(block_idx, count + 1);
  }

  // The slot is not an inline one. Inserting the clipped value into the subblock

  uint64 handle;
  if (tag == HASHED_BLOCK)
    handle = insert_unique_into_hashed_block(array_pool, get_payload(low), get_count(slot), clipped(value), mem_pool);
  else
    handle = insert_unique_with_linear_block(array_pool, slot, clipped(value), mem_pool);

  assert(get_count(handle) == get_count(slot) + 1);
  slots = array_pool->slots; // Refreshing the local copy
  slots[slot_idx] = handle;
  return hashed_block_handle(block_idx, count + 1);
}

////////////////////////////////////////////////////////////////////////////////

static uint64 shrink_linear_block(ARRAY_MEM_POOL *array_pool, uint32 tag, uint32 block_idx, uint32 count) {
  uint64 *slots = array_pool->slots;

  if (tag == SIZE_2_BLOCK) {
    assert(count == 2);
    return slots[block_idx];
  }

  if (tag == SIZE_4_BLOCK) {
    assert(count == 3);
    uint64 slot_0 = slots[block_idx];
    uint64 slot_1 = slots[block_idx + 1];
    array_mem_pool_release_4_block(array_pool, block_idx);
    uint32 size_2_block_idx = array_mem_pool_alloc_2_block(array_pool, NULL); // We've just released a block of size 4, no need to allocate new memory
    assert(slots == array_pool->slots); //## THIS ONE SHOULD BE FINE
    slots[size_2_block_idx] = slot_0;
    slots[size_2_block_idx + 1] = slot_1;
    return size_2_block_handle(size_2_block_idx, count);
  }

  if (tag == SIZE_8_BLOCK) {
    assert(count == 6);
    array_mem_pool_release_8_block_upper_half(array_pool, block_idx);
    return size_4_block_handle(block_idx, count);
  }

  assert(tag == SIZE_16_BLOCK);
  assert(count == 12);
  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, count);
}

uint64 copy_and_release_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint64 state, uint32 least_bits) {
  if (handle == EMPTY_SLOT)
    return state;

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  uint32 next_idx = get_low_32(state);
  uint32 leftover = get_high_32(state);

  uint64 *slots = array_pool->slots;

  if (tag == INLINE_SLOT) {
    uint32 high = get_high_32(handle);

    if (leftover != EMPTY_MARKER) {
      slots[next_idx++] = pack(leftover, low);
      leftover = EMPTY_MARKER;
    }
    else
      leftover = low;

    if (high != EMPTY_MARKER)
      if (leftover != EMPTY_MARKER) {
        slots[next_idx++] = pack(leftover, high);
        leftover = EMPTY_MARKER;
      }
      else
        leftover = high;
  }
  else {
    uint32 block_idx = get_payload(low);
    uint32 end = (get_count(handle) + 1) / 2;

    for (uint32 i=0 ; i < end ; i++) {
      uint64 slot = slots[block_idx + i];

      assert(slot != EMPTY_SLOT);

      uint32 slot_low = get_low_32(slot);
      uint32 slot_high = get_high_32(slot);

      if (leftover != EMPTY_MARKER) {
        slots[next_idx++] = pack(leftover, unclipped(slot_low, least_bits));
        leftover = EMPTY_MARKER;
      }
      else
        leftover = unclipped(slot_low, least_bits);

      if (slot_high != EMPTY_MARKER) {
        if (leftover != EMPTY_MARKER) {
          slots[next_idx++] = pack(leftover, unclipped(slot_high, least_bits));
          leftover = EMPTY_MARKER;
        }
        else
          leftover = unclipped(slot_high, least_bits);
      }
    }

    if (tag == SIZE_2_BLOCK) {
      array_mem_pool_release_2_block(array_pool, block_idx);
    }
    else if (tag == SIZE_4_BLOCK) {
      array_mem_pool_release_4_block(array_pool, block_idx);
    }
    else {
      // Both 16-slot and hashed blocks contain at least 7 elements, so they cannot appear
      // here, as the parent hashed block being shrunk has only six elements left
      assert(tag == SIZE_8_BLOCK);
      array_mem_pool_release_8_block(array_pool, block_idx);
    }
  }

  return pack(next_idx, leftover);
}

static uint64 shrink_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert(HASHED_BLOCK_MIN_COUNT == 13);

  uint64 *slots = array_pool->slots;

  // Here we've exactly 12 elements left, therefore we need the save the first 6 slots
  uint64 saved_slots[6];
  for (uint32 i=0 ; i < 6 ; i++)
    saved_slots[i] = slots[block_idx + i];

  uint64 state = pack(block_idx, EMPTY_MARKER);
  for (uint32 i=0 ; i < 6 ; i++)
    state = copy_and_release_block(array_pool, saved_slots[i], state, i);

  uint32 end_idx = block_idx + 6;
  for (uint32 i=6 ; get_low_32(state) < end_idx ; i++)
    state = copy_and_release_block(array_pool, slots[block_idx + i], state, i);

  assert(state == pack(block_idx + 6, EMPTY_MARKER));

  slots[block_idx + 6] = pack(EMPTY_MARKER, 0);
  slots[block_idx + 7] = pack(EMPTY_MARKER, 0);

  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, 12);
}

////////////////////////////////////////////////////////////////////////////////

static uint64 delete_from_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value) {
  uint64 *slots = array_pool->slots;

  uint32 tag = get_tag(get_low_32(handle));
  uint32 block_idx = get_payload(get_low_32(handle));
  uint32 count = get_count(handle);

  uint32 last_slot_idx = (count + 1) / 2 - 1;
  uint64 last_slot = slots[block_idx + last_slot_idx];

  uint32 last_low = get_low_32(last_slot);
  uint32 last_high = get_high_32(last_slot);

  assert(last_low != EMPTY_MARKER && get_tag(last_low) == INLINE_SLOT);
  assert((last_high != EMPTY_MARKER && get_tag(last_high) == INLINE_SLOT) || (last_high == EMPTY_MARKER && !is_even(count)));

  // Checking the last slot first
  if (value == last_low | value == last_high) {
    // Removing the value
    if (value == last_low)
      slots[block_idx + last_slot_idx] = pack(last_high, EMPTY_MARKER);
    else
      slots[block_idx + last_slot_idx] = pack(last_low, EMPTY_MARKER);

    // Shrinking the block if need be
    if (count == min_count(tag))
      return shrink_linear_block(array_pool, tag, block_idx, count - 1);
    else
      return linear_block_handle(tag, block_idx, count - 1);
  }

  // The last slot didn't contain the searched value, looking in the rest of the array
  for (uint32 i = last_slot_idx - 1 ; i >= 0 ; i--) {
    uint64 slot = slots[block_idx + i];
    uint32 low = get_low_32(slot);
    uint32 high = get_high_32(slot);

    assert(low != EMPTY_MARKER && get_tag(low) == INLINE_SLOT);
    assert(high != EMPTY_MARKER && get_tag(high) == INLINE_SLOT);

    if (value == low | value == high) {
      // Removing the last value to replace the one being deleted
      uint32 last;
      if (is_even(count)) {
        last = last_high;
        slots[block_idx + last_slot_idx] = pack(last_low, EMPTY_MARKER);
      }
      else {
        last = last_low;
        slots[block_idx + last_slot_idx] = pack(EMPTY_MARKER, 0);
      }

      // Replacing the value to be deleted with the last one
      if (value == low)
        slots[block_idx + i] = pack(last, high);
      else
        slots[block_idx + i] = pack(low, last);

      // Shrinking the block if need be
      if (count == min_count(tag))
        return shrink_linear_block(array_pool, tag, block_idx, count - 1);
      else
        return linear_block_handle(tag, block_idx, count - 1);
    }
  }

  // Value not found
  return handle;
}


uint64 overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value);


static uint64 delete_from_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value) {
  uint64 *slots = array_pool->slots;

  uint32 index = get_index(value);
  uint32 slot_idx = block_idx + index;
  uint64 slot = slots[slot_idx];

  // If the slot is empty there's nothing to do
  if (slot == EMPTY_SLOT)
    return hashed_block_handle(block_idx, count);

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  // If the slot is not inline, we recursively call overflow_table_delete(..) with a clipped value
  if (get_tag(low) != INLINE_SLOT) {
    uint64 handle = overflow_table_delete(array_pool, slot, clipped(value)); //## I'D LIKE TO REMOVE THIS DEPENDENCY FROM A PUBLIC FUNCTION
    if (handle == slot)
      return hashed_block_handle(block_idx, count);
    uint32 handleLow = get_low_32(handle);
    if (get_tag(handleLow) == INLINE_SLOT)
      handle = pack(unclipped(handleLow, index), unclipped(get_high_32(handle), index));
    slots[slot_idx] = handle;
  }
  else if (low == value) {
    if (high == EMPTY_MARKER)
      slots[slot_idx] = pack(EMPTY_MARKER, 0);
    else
      slots[slot_idx] = pack(high, EMPTY_MARKER);
  }
  else if (high == value) {
    assert(high != EMPTY_MARKER);
    slots[slot_idx] = pack(low, EMPTY_MARKER);
  }
  else {
    return hashed_block_handle(block_idx, count);
  }

  assert(count >= HASHED_BLOCK_MIN_COUNT);

  // The value has actually been deleted. Shrinking the block if need be
  if (count > HASHED_BLOCK_MIN_COUNT)
    return hashed_block_handle(block_idx, count - 1);
  else
    return shrink_hashed_block(array_pool, block_idx);
}

////////////////////////////////////////////////////////////////////////////////

bool overflow_table_contains(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value);

static bool linear_block_contains(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value) {
  uint64 *slots = array_pool->slots;
  uint32 end = (count + 1) / 2;
  for (uint32 i=0 ; i < end ; i++) {
    uint64 slot = slots[block_idx + i];
    if (value == get_low_32(slot) | value == get_high_32(slot))
      return true;
  }
  return false;
}

static bool hashed_block_contains(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 value) {
  uint64 *slots = array_pool->slots;

  uint32 slot_idx = block_idx + get_index(value);
  uint64 slot = slots[slot_idx];

  if (slot == EMPTY_SLOT)
    return false;

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);
  uint32 tag = get_tag(low);

  if (tag == 0)
    return value == low | value == high;

  if (tag == HASHED_BLOCK)
    return hashed_block_contains(array_pool, get_payload(low), clipped(value));

  return overflow_table_contains(array_pool, slot, clipped(value)); //## I'D LIKE TO REMOVE THIS DEPENDENCY FROM A PUBLIC FUNCTION
}

////////////////////////////////////////////////////////////////////////////////

uint32 copy_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 *dest, uint32 offset, uint32 shift, uint32 least_bits) {
  uint64 *slots = array_pool->slots;

  uint32 subshift = shift + 4;
  uint32 target_idx = offset;

  for (uint32 i=0 ; i < 16 ; i++) {
    uint32 slot_least_bits = (i << shift) + least_bits;
    uint64 slot = slots[block_idx + i];
    uint32 low = get_low_32(slot);

    if (low != EMPTY_MARKER) {
      uint32 tag = get_tag(low);
      if (tag == INLINE_SLOT) {
        dest[target_idx] = (get_payload(low) << shift) + least_bits;
        target_idx++;

        uint32 high = get_high_32(slot);
        if (high != EMPTY_MARKER) {
          dest[target_idx] = (get_payload(high) << shift) + least_bits;
          target_idx++;
        }
      }
      else if (tag == HASHED_BLOCK) {
        target_idx = copy_hashed_block(array_pool, get_payload(low), dest, target_idx, subshift, slot_least_bits);
      }
      else {
        uint32 subblock_idx = get_payload(low);
        uint32 count = get_count(slot);
        uint32 end = (count + 1) / 2;

        for (uint32 j=0 ; j < end ; j++) {
          uint64 subslot = slots[subblock_idx + j];
          uint32 sublow = get_low_32(subslot);
          uint32 subhigh = get_high_32(subslot);

          assert(sublow != EMPTY_MARKER & get_tag(sublow) == 0);

          dest[target_idx] = (sublow << subshift) + slot_least_bits;
          target_idx++;

          if (subhigh != EMPTY_MARKER) {
            dest[target_idx] = (subhigh << subshift) + slot_least_bits;
            target_idx++;
          }
        }
      }
    }
  }
  return target_idx;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint64 overflow_table_insert(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  if (tag == 0)
    return insert_2_block(array_pool, handle, value, mem_pool);

  if (tag == HASHED_BLOCK)
    return insert_into_hashed_block(array_pool, get_payload(low), get_count(handle), value, mem_pool);

  return insert_with_linear_block(array_pool, handle, value, mem_pool);
}

uint64 overflow_table_insert_unique(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  if (tag == 0)
    return insert_2_block(array_pool, handle, value, mem_pool);

  if (tag == HASHED_BLOCK)
    return insert_unique_into_hashed_block(array_pool, get_payload(low), get_count(handle), value, mem_pool);

  return insert_unique_with_linear_block(array_pool, handle, value, mem_pool);
}

uint64 overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  assert(tag != INLINE_SLOT);

  if (tag == HASHED_BLOCK)
    return delete_from_hashed_block(array_pool, get_payload(low), get_count(handle), value);
  else
    return delete_from_linear_block(array_pool, handle, value);
}

void overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);

  assert(tag != INLINE_SLOT);

  if (tag == SIZE_2_BLOCK)
    array_mem_pool_release_2_block(array_pool, block_idx);
  else if (tag == SIZE_4_BLOCK)
    array_mem_pool_release_4_block(array_pool, block_idx);
  else if (tag == SIZE_8_BLOCK)
    array_mem_pool_release_8_block(array_pool, block_idx);
  else if (tag == SIZE_16_BLOCK)
    array_mem_pool_release_16_block(array_pool, block_idx);
  else {
    assert(tag == HASHED_BLOCK);
    uint64 *slots = array_pool->slots;
    for (uint32 i=0 ; i < 16 ; i++) {
      uint64 slot = slots[block_idx + i];
      if (slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT)
        overflow_table_delete(array_pool, slot);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool overflow_table_contains(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value) {
  uint32 tag = get_tag(get_low_32(handle));
  uint32 block_idx = get_payload(get_low_32(handle));

  assert(tag != INLINE_SLOT);
  assert(pack_tag_payload(tag, block_idx) == get_low_32(handle));

  if (tag != HASHED_BLOCK)
    return linear_block_contains(array_pool, block_idx, get_count(handle), value);
  else
    return hashed_block_contains(array_pool, block_idx, value);
}

void overflow_table_copy(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 *dest, uint32 offset) {
  uint64 *slots = array_pool->slots;

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);

  assert(tag != INLINE_SLOT);
  assert(pack_tag_payload(tag, block_idx) == get_low_32(handle));

  if (tag != HASHED_BLOCK) {
    uint32 count = get_count(handle);
    uint32 end = (count + 1) / 2;
    uint32 target_idx = offset;

    for (uint32 i=0 ; i < end ; i++) {
      uint64 slot = slots[block_idx + i];
      uint32 slot_low = get_low_32(slot);
      uint32 slot_high = get_high_32(slot);

      assert(slot_low != EMPTY_MARKER & get_tag(slot_low) == INLINE_SLOT);

      dest[target_idx] = slot_low;
      target_idx++;

      if (slot_high != EMPTY_MARKER) {
        assert(get_tag(slot_high) == INLINE_SLOT);

        dest[target_idx] = slot_high;
        target_idx++;
      }
    }
  }
  else
    copy_hashed_block(array_pool, block_idx, dest, offset, 0, 0);
}

// static uint32 overflow_table_count(ARRAY_MEM_POOL *array_pool, uint64 slot) {
//   assert(tag(get_low_32(slot)) >= SIZE_2_BLOCK & tag(get_low_32(slot)) <= HASHED_BLOCK);
//   // assert(get_high_32(slot) > 2); // Not true when initializing a hashed block
//   return get_high_32(slot);
// }
