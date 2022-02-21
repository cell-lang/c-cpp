#include "lib.h"
#include "one-way-bin-table.h"


// A slot can be in any of the following states:
//   - Value + data:        32 bit data              - 3 zeros   - 29 bit value
//   - Index + count:       32 bit count             - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:               32 zeros                 - 32 ones
//     This type of slot can only be stored in a block, but cannot be passed in or out


const uint32 INLINE_SLOT    = 0;
const uint32 SIZE_2_BLOCK   = 1;
const uint32 SIZE_4_BLOCK   = 2;
const uint32 SIZE_8_BLOCK   = 3;
const uint32 SIZE_16_BLOCK  = 4;
const uint32 HASHED_BLOCK   = 5;

const uint32 SIZE_2_BLOCK_MIN_COUNT   = 2;
const uint32 SIZE_4_BLOCK_MIN_COUNT   = 2;
const uint32 SIZE_8_BLOCK_MIN_COUNT   = 3;
const uint32 SIZE_16_BLOCK_MIN_COUNT  = 7;
const uint32 HASHED_BLOCK_MIN_COUNT   = 7;

const uint64 EMPTY_SLOT = 0xFFFFFFFFL;

////////////////////////////////////////////////////////////////////////////////

static uint64 insert_unique_2_block(uint64 handle, uint32 value, uint32 data) {
  assert(low(handle) != value);
  assert(tag(low(handle)) == INLINE_SLOT);

  uint32 block_idx = array_mem_pool_alloc_2_block(array_pool, mem_pool);
  slots[block_idx] = handle;
  slots[block_idx + 1] = pack(value, data);
  return size_2_block_handle(block_idx);
}

////////////////////////////////////////////////////////////////////////////

static uint64 insert_unique_with_linear_block(uint64 handle, uint32 value, uint32 data) {
  assert(tag(low(handle)) >= SIZE_2_BLOCK & tag(low(handle)) <= SIZE_16_BLOCK);

  uint32 low = low(handle);
  uint32 tag = tag(low);
  uint32 block_idx = payload(low);
  uint32 count = count(handle);
  uint32 capacity = capacity(tag);

  // Inserting the new value if there's still room here
  if (count < capacity) {
    uint32 slot_idx = block_idx + count;
    slots[slot_idx] = pack(value, data);
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

    // Initializing the new block
    for (uint32 i=0 ; i < count ; i++)
      slots[new_block_idx + i] = slots[block_idx + i];
    slots[new_block_idx + count] = pack(value, data);
    for (uint32 i=count+1 ; i < 2 * count ; i++)
      slots[new_block_idx + i] = EMPTY_SLOT;

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
  for (uint32 i=0 ; i < 16 ; i++)
    slots[hashed_block_idx + i] = EMPTY_SLOT;

  // Transferring the existing values
  for (uint32 i=0 ; i < 16 ; i++) {
    uint64 slot = slot(block_idx + i);
    uint64 tmp_handle = insert_unique_into_hashed_block(array_pool, hashed_block_idx, i, low(slot), high(slot), mem_pool);
    assert(count(tmp_handle) == i + 1);
    assert(payload(low(tmp_handle)) == hashed_block_idx);
  }

  // Releasing the old block
  array_mem_pool_release_16_block(array_pool, block_idx);

  // Adding the new value
  return insert_unique_into_hashed_block(array_pool, hashed_block_idx, 16, value, data, mem_pool);
}

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 slot_idx = block_idx + index(value);
  uint64 slot = slot(slot_idx);

  // Checking for empty slots
  if (slot == EMPTY_SLOT) {
    slots[slot_idx] = pack(value, data);
    return hashed_block_handle(block_idx, count + 1);
  }

  uint32 low = low(slot);
  uint32 tag = tag(low);

  // Checking for inline slots
  if (tag == INLINE_SLOT) {
    assert(value != low);
    uint64 handle = insert_unique_2_block(pack(clipped(low), high(slot)), clipped(value), data);
    assert(count(handle) == 2);
    slots[slot_idx] = handle;
    return hashed_block_handle(block_idx, count + 1);
  }

  // The slot is not an inline one. Inserting the clipped value into the subblock

  uint64 handle;
  if (tag == HASHED_BLOCK)
    handle = insert_unique_into_hashed_block(array_pool, payload(low), count(slot), clipped(value), data, mem_pool);
  else
    handle = insert_unique_with_linear_block(slot, clipped(value), data);

  assert(count(handle) == count(slot) + 1);
  slots[slot_idx] = handle;
  return hashed_block_handle(block_idx, count + 1);
}

////////////////////////////////////////////////////////////////////////////

static uint64 delete_from_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 *data) {
  uint32 tag = tag(low(handle));
  uint32 block_idx = payload(low(handle));
  uint32 count = count(handle);

  uint32 last_slot_idx = block_idx + count - 1;
  uint64 last_slot = slot(last_slot_idx);

  assert(last_slot != EMPTY_SLOT);
  assert(count == capacity(tag) || slot(block_idx + count) == EMPTY_SLOT);

  uint32 last_low = low(last_slot);

  // Checking the last slot first
  if (value == last_low) {
    // Removing the value
    slots[last_slot_idx] = EMPTY_SLOT;

    if (data != null)
      data[0] = high(last_slot);

    // Shrinking the block if need be
    if (count == min_count(tag))
      return shrink_linear_block(array_pool, tag, block_idx, count - 1);
    else
      return linear_block_handle(tag, block_idx, count - 1);
  }

  // The last slot didn't contain the searched value, looking in the rest of the array
  for (uint32 i = last_slot_idx - 1 ; i >= block_idx ; i--) {
    uint64 slot = slot(i);
    uint32 low = low(slot);

    assert(slot != EMPTY_SLOT && tag(low) == INLINE_SLOT);

    if (value == low) {
      // Replacing the value to be deleted with the last one
      slots[i] = last_slot;

      // Clearing the last slot whose value has been stored in the delete slot
      slots[last_slot_idx] = EMPTY_SLOT;

      if (data != null)
        data[0] = high(slot);

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

static uint64 shrink_linear_block(ARRAY_MEM_POOL *array_pool, uint32 tag, uint32 block_idx, uint32 count) {
  if (tag == SIZE_2_BLOCK | tag == SIZE_4_BLOCK) {
    assert(count == 1);
    return slot(block_idx);
  }

  if (tag == SIZE_8_BLOCK) {
    assert(count == 2);
    uint32 block_2_idx = array_mem_pool_alloc_2_block(array_pool, mem_pool);
    slots[block_2_idx] = slot(block_idx);
    slots[block_2_idx + 1] = slot(block_idx + 1);
    array_mem_pool_release_8_block(array_pool, block_idx);
    return size_2_block_handle(block_2_idx);

    // array_mem_pool_release_8_block_upper_half(array_pool, block_idx);
    // return size_4_block_handle(block_idx, count);
  }

  assert(tag == SIZE_16_BLOCK);
  assert(count == 6);
  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, count);
}

////////////////////////////////////////////////////////////////////////////

static uint64 delete_from_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, uint32 *data) {
  uint32 index = index(value);
  uint32 slot_idx = block_idx + index;
  uint64 slot = slot(slot_idx);

  // If the slot is empty there's nothing to do
  if (slot == EMPTY_SLOT)
    return hashed_block_handle(block_idx, count);

  uint32 low = low(slot);

  // If the slot is not inline, we recursively call loaded_overflow_table_delete(..) with a clipped value
  if (tag(low) != INLINE_SLOT) {
    uint64 handle = loaded_overflow_table_delete(array_pool, slot, clipped(value), data);
    if (handle == slot)
      return hashed_block_handle(block_idx, count);
    uint32 handle_low = low(handle);
    if (tag(handle_low) == INLINE_SLOT)
      handle = pack(unclipped(handle_low, index), high(handle));
    slots[slot_idx] = handle;
  }
  else if (low == value) {
    slots[slot_idx] = EMPTY_SLOT;
    if (data != null)
      data[0] = high(slot);
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

static uint64 shrink_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert(HASHED_BLOCK_MIN_COUNT == 7);

  // Here we've exactly 6 elements left, therefore we need the save the first 6 slots
  uint64 slot0  = slot(block_idx);
  uint64 slot1  = slot(block_idx + 1);
  uint64 slot2  = slot(block_idx + 2);
  uint64 slot3  = slot(block_idx + 3);
  uint64 slot4  = slot(block_idx + 4);
  uint64 slot5  = slot(block_idx + 5);

  uint32 next_idx = block_idx;
  next_idx = copy_and_release_block(array_pool, slot0, next_idx, 0);
  next_idx = copy_and_release_block(array_pool, slot1, next_idx, 1);
  next_idx = copy_and_release_block(array_pool, slot2, next_idx, 2);
  next_idx = copy_and_release_block(array_pool, slot3, next_idx, 3);
  next_idx = copy_and_release_block(array_pool, slot4, next_idx, 4);
  next_idx = copy_and_release_block(array_pool, slot5, next_idx, 5);

  uint32 end_idx = block_idx + 6;
  for (uint32 i=6 ; next_idx < end_idx ; i++)
    next_idx = copy_and_release_block(array_pool, slot(block_idx + i), next_idx, i);

  slots[block_idx + 6] = EMPTY_SLOT;
  slots[block_idx + 7] = EMPTY_SLOT;

  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, 6);
}

static uint32 copy_and_release_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 next_idx, uint32 least_bits) {
  if (handle == EMPTY_SLOT)
    return next_idx;

  uint32 low = low(handle);
  uint32 tag = tag(low);

  if (tag == INLINE_SLOT) {
    slots[next_idx++] = handle;
    return next_idx;
  }

  // The block the handle is pointing to cannot have more than 6 elements,
  // so the block is a linear one, and at most an 8-block

  uint32 block_idx = payload(low);
  uint32 count = count(handle);

  for (uint32 i=0 ; i < count ; i++) {
    uint64 slot = slot(block_idx + i);
    assert(slot != EMPTY_SLOT & tag(low(slot)) == INLINE_SLOT);
    slots[next_idx++] = pack(unclipped(low(slot), least_bits), high(slot));
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

  return next_idx;
}

////////////////////////////////////////////////////////////////////////////////

public uint64 loaded_overflow_table_insert_unique(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 low = low(handle);
  uint32 tag = tag(low);

  if (tag == 0)
    return insert_unique_2_block(handle, value, data);

  if (tag == HASHED_BLOCK)
    return insert_unique_into_hashed_block(array_pool, payload(low), count(handle), value, data, mem_pool);

  return insert_unique_with_linear_block(handle, value, data);
}

public uint64 loaded_overflow_table_delete(array_pool, ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 *data) {
  uint32 low = low(handle);
  uint32 tag = tag(low);

  assert(tag != INLINE_SLOT);

  if (tag == HASHED_BLOCK)
    return delete_from_hashed_block(array_pool, payload(low), count(handle), value, data);
  else
    return delete_from_linear_block(array_pool, handle, value, data);
}

public void loaded_overflow_table_delete(array_pool, ARRAY_MEM_POOL *array_pool, uint64 handle) {
  uint32 low = low(handle);
  uint32 tag = tag(low);
  uint32 block_idx = payload(low);

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
    for (uint32 i=0 ; i < 16 ; i++) {
      uint64 slot = slot(block_idx + i);
      if (slot != EMPTY_SLOT && tag(low(slot)) != INLINE_SLOT)
        loaded_overflow_table_delete(array_pool, slot);
    }
  }
}

public uint32 lookup(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value) {
  uint32 low = low(handle);
  uint32 tag = tag(low);
  uint32 block_idx = payload(low);

  assert(tag != INLINE_SLOT);
  assert(tag(tag, block_idx) == low(handle));

  if (tag != HASHED_BLOCK)
    return linear_block_lookup(array_pool, block_idx, count(handle), value);
  else
    return hashed_block_lookup(array_pool, block_idx, value);
}

public void copy(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 *surrs2, uint32 *data) {
  copy(handle, surrs2, data, 0, 1);
}

public void copy(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 *surrs2, uint32 *data, uint32 offset, uint32 step) {
  uint32 low = low(handle);
  uint32 tag = tag(low);
  uint32 block_idx = payload(low);

  assert(tag != INLINE_SLOT);
  assert(tag(tag, block_idx) == low(handle));

  if (tag != HASHED_BLOCK) {
    uint32 count = count(handle);
    uint32 target_idx = offset;

    for (uint32 i=0 ; i < count ; i++) {
      uint64 slot = slot(block_idx + i);

      assert(slot != EMPTY_SLOT & tag(low(slot)) == INLINE_SLOT);

      surrs2[target_idx] = low(slot);
      if (data != null)
        data[target_idx] = high(slot);

      target_idx += step;
    }
  }
  else
    copy_hashed_block(array_pool, block_idx, surrs2, data, offset, step, 0, 0);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

public static uint32 count(uint64 slot) {
  assert(tag(low(slot)) >= SIZE_2_BLOCK & tag(low(slot)) <= HASHED_BLOCK);
  // assert(high(slot) > 2); // Not true when initializing a hashed block
  return high(slot);
}

////////////////////////////////////////////////////////////////////////////

static uint32 capacity(uint32 tag) {
  assert(tag >= SIZE_2_BLOCK & tag <= SIZE_16_BLOCK);
  assert(SIZE_2_BLOCK == 1 | SIZE_16_BLOCK == 4);
  return 1 << tag;
}

static uint64 linear_block_handle(uint32 tag, uint32 index, uint32 count) {
  return pack(tag(tag, index), count);
}

static uint64 size_2_block_handle(uint32 index) {
  assert(tag(index) == 0);
  return pack(tag(SIZE_2_BLOCK, index), 2);
}

static uint64 size_4_block_handle(uint32 index, uint32 count) {
  assert(tag(index) == 0);
  assert(count >= SIZE_4_BLOCK_MIN_COUNT & count <= 4);
  return pack(tag(SIZE_4_BLOCK, index), count);
}

static uint64 size_8_block_handle(uint32 index, uint32 count) {
  assert(tag(index) == 0);
  assert(count >= SIZE_8_BLOCK_MIN_COUNT & count <= 8);
  return pack(tag(SIZE_8_BLOCK, index), count);
}

static uint64 size_16_block_handle(uint32 index, uint32 count) {
  assert(tag(index) == 0);
  assert(count >= SIZE_16_BLOCK_MIN_COUNT & count <= 16);
  return pack(tag(SIZE_16_BLOCK, index), count);
}

static uint64 hashed_block_handle(uint32 index, uint32 count) {
  assert(tag(index) == 0);
  // assert(count >= 7); // Not true when initializing a hashed block
  uint64 handle = pack(tag(HASHED_BLOCK, index), count);
  assert(tag(low(handle)) == HASHED_BLOCK);
  assert(payload(low(handle)) == index);
  assert(count(handle) == count);
  return handle;
}

static uint32 index(uint32 value) {
  assert(tag(value) == INLINE_SLOT);
  return value & 0xF;
}

static uint32 clipped(uint32 value) {
  return value >> 4;
}

static uint32 unclipped(uint32 value, uint32 index) {
  assert(tag(value) == 0);
  assert(tag(value << 4) == 0);
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

// static boolean is_even(uint32 value) {
//   return (value % 2) == 0;
// }

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

static uint32 linear_block_lookup(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value) {
  for (uint32 i=0 ; i < count ; i++) {
    uint64 slot = slot(block_idx + i);
    assert(tag(low(slot)) == INLINE_SLOT);
    if (value == low(slot))
      return high(slot);
  }
  return 0xFFFFFFFF;
}

static uint32 hashed_block_lookup(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 value) {
  uint32 slot_idx = block_idx + index(value);
  uint64 slot = slot(slot_idx);

  if (slot == EMPTY_SLOT)
    return 0xFFFFFFFF;

  uint32 low = low(slot);
  uint32 tag = tag(low);

  if (tag == INLINE_SLOT)
    return value == low ? high(slot) : 0xFFFFFFFF;

  uint32 subblock_idx = payload(low);
  uint32 clipped_value = clipped(value);

  if (tag == HASHED_BLOCK)
    return hashed_block_lookup(array_pool, subblock_idx, clipped_value);
  else
    return linear_block_lookup(array_pool, subblock_idx, count(slot), clipped_value);
}

////////////////////////////////////////////////////////////////////////////

static uint32 copy_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 *surrs2, uint32 *data, uint32 offset, uint32 step, uint32 shift, uint32 least_bits) {
  uint32 subshift = shift + 4;
  uint32 target_idx = offset;

  for (uint32 i=0 ; i < 16 ; i++) {
    uint32 slot_least_bits = (i << shift) + least_bits;
    uint64 slot = slot(block_idx + i);

    if (slot != EMPTY_SLOT) {
      uint32 low = low(slot);
      uint32 tag = tag(low);

      if (tag == INLINE_SLOT) {
        surrs2[target_idx] = (payload(low) << shift) + least_bits;
        if (data != null)
          data[target_idx] = high(slot);
        target_idx += step;
      }
      else if (tag == HASHED_BLOCK) {
        target_idx = copy_hashed_block(array_pool, payload(low), surrs2, data, target_idx, step, subshift, slot_least_bits);
      }
      else {
        uint32 subblock_idx = payload(low);
        uint32 count = count(slot);

        for (uint32 j=0 ; j < count ; j++) {
          uint64 subslot = slot(subblock_idx + j);

          assert(subslot != EMPTY_SLOT & tag(low(subslot)) == INLINE_SLOT);

          surrs2[target_idx] = (low(subslot) << subshift) + slot_least_bits;
          if (data != null)
            data[target_idx] = high(subslot);
          target_idx += step;
        }
      }
    }
  }

  return target_idx;
}
