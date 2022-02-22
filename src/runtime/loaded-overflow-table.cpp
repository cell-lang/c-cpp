#include "lib.h"
#include "one-way-bin-table.h"


// A slot can be in any of the following states:
//   - Value + data:        32 bit data              - 3 zeros   - 29 bit value
//   - Index + count:       32 bit count             - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:               32 zeros                 - 32 ones
//     This type of slot can only be stored in a block, but cannot be passed in or out


const uint32 SIZE_2_BLOCK_MIN_COUNT   = 2;
const uint32 SIZE_4_BLOCK_MIN_COUNT   = 2;
const uint32 SIZE_8_BLOCK_MIN_COUNT   = 3;
const uint32 SIZE_16_BLOCK_MIN_COUNT  = 7;
const uint32 HASHED_BLOCK_MIN_COUNT   = 7;

////////////////////////////////////////////////////////////////////////////

static uint32 get_capacity(uint32 tag) {
  assert(tag >= SIZE_2_BLOCK & tag <= SIZE_16_BLOCK);
  assert(SIZE_2_BLOCK == 1 | SIZE_16_BLOCK == 4);
  return 1 << tag;
}

static uint64 linear_block_handle(uint32 tag, uint32 index, uint32 count) {
  return pack(pack_tag_payload(tag, index), count);
}

static uint64 size_2_block_handle(uint32 index) {
  assert(get_tag(index) == 0);
  return pack(pack_tag_payload(SIZE_2_BLOCK, index), 2);
}

static uint64 size_4_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_4_BLOCK_MIN_COUNT & count <= 4);
  return pack(pack_tag_payload(SIZE_4_BLOCK, index), count);
}

static uint64 size_8_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_8_BLOCK_MIN_COUNT & count <= 8);
  return pack(pack_tag_payload(SIZE_8_BLOCK, index), count);
}

static uint64 size_16_block_handle(uint32 index, uint32 count) {
  assert(get_tag(index) == 0);
  assert(count >= SIZE_16_BLOCK_MIN_COUNT & count <= 16);
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

////////////////////////////////////////////////////////////////////////////////

static uint64 insert_unique_2_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  assert(get_low_32(handle) != value);
  assert(get_tag(get_low_32(handle)) == INLINE_SLOT);

  uint32 block_idx = array_mem_pool_alloc_2_block(array_pool, mem_pool);
  uint64 *target_slots = array_pool->slots + block_idx;
  target_slots[0] = handle;
  target_slots[1] = pack(value, data);
  return size_2_block_handle(block_idx);
}

////////////////////////////////////////////////////////////////////////////

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *, uint32 block_idx, uint32 count, uint32 value, uint32 data, STATE_MEM_POOL *);

static uint64 insert_unique_with_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  assert(get_tag(get_low_32(handle)) >= SIZE_2_BLOCK & get_tag(get_low_32(handle)) <= SIZE_16_BLOCK);

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);
  uint32 count = get_count(handle);
  uint32 capacity = get_capacity(tag);

  // Inserting the new value if there's still room here
  if (count < capacity) {
    uint32 slot_idx = block_idx + count;
    array_pool->slots[slot_idx] = pack(value, data);
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
    uint64 *slots = array_pool->slots;
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
  uint64 *target_slots = array_pool->slots + hashed_block_idx;
  for (uint32 i=0 ; i < 16 ; i++)
    target_slots[i] = EMPTY_SLOT;

  // Transferring the existing values
  for (uint32 i=0 ; i < 16 ; i++) {
    uint64 slot = array_pool->slots[block_idx + i];
    uint64 tmp_handle = insert_unique_into_hashed_block(array_pool, hashed_block_idx, i, get_low_32(slot), get_high_32(slot), mem_pool);
    assert(get_count(tmp_handle) == i + 1);
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
  }

  // Releasing the old block
  array_mem_pool_release_16_block(array_pool, block_idx);

  // Adding the new value
  return insert_unique_into_hashed_block(array_pool, hashed_block_idx, 16, value, data, mem_pool);
}

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 slot_idx = block_idx + get_index(value);
  uint64 *slot_ptr = array_pool->slots + slot_idx;
  uint64 slot = *slot_ptr;

  // Checking for empty slots
  if (slot == EMPTY_SLOT) {
    *slot_ptr = pack(value, data);
    return hashed_block_handle(block_idx, count + 1);
  }

  uint32 low = get_low_32(slot);
  uint32 tag = get_tag(low);

  // Checking for inline slots
  if (tag == INLINE_SLOT) {
    assert(value != low);
    uint64 handle = insert_unique_2_block(array_pool, pack(clipped(low), get_high_32(slot)), clipped(value), data, mem_pool);
    assert(get_count(handle) == 2);
    array_pool->slots[slot_idx] = handle;
    return hashed_block_handle(block_idx, count + 1);
  }

  // The slot is not an inline one. Inserting the clipped value into the subblock

  uint64 handle;
  if (tag == HASHED_BLOCK)
    handle = insert_unique_into_hashed_block(array_pool, get_payload(low), get_count(slot), clipped(value), data, mem_pool);
  else
    handle = insert_unique_with_linear_block(array_pool, slot, clipped(value), data, mem_pool);

  assert(get_count(handle) == get_count(slot) + 1);
  array_pool->slots[slot_idx] = handle;
  return hashed_block_handle(block_idx, count + 1);
}

////////////////////////////////////////////////////////////////////////////

static uint64 shrink_linear_block(ARRAY_MEM_POOL *array_pool, uint32 tag, uint32 block_idx, uint32 count) {
  if (tag == SIZE_2_BLOCK | tag == SIZE_4_BLOCK) {
    assert(count == 1);
    return array_pool->slots[block_idx];
  }

  if (tag == SIZE_8_BLOCK) {
    assert(count == 2);
    uint64 *src_slots = array_pool->slots + block_idx;
    uint64 slot_0 = src_slots[0];
    uint64 slot_1 = src_slots[1];
    array_mem_pool_release_8_block(array_pool, block_idx);
    uint32 block_2_idx = array_mem_pool_alloc_2_block(array_pool, NULL); // We've just released a block of size 8, no need to allocate new memory
    uint64 *target_slots = array_pool->slots + block_2_idx;
    target_slots[0] = slot_0;
    target_slots[1] = slot_1;
    return size_2_block_handle(block_2_idx);

    // array_mem_pool_release_8_block_upper_half(array_pool, block_idx);
    // return size_4_block_handle(block_idx, count);
  }

  assert(tag == SIZE_16_BLOCK);
  assert(count == 6);
  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, count);
}

static uint32 copy_and_release_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 next_idx, uint32 least_bits) {
  if (handle == EMPTY_SLOT)
    return next_idx;

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  uint64 *slots = array_pool->slots;

  if (tag == INLINE_SLOT) {
    array_pool->slots[next_idx++] = handle;
    return next_idx;
  }

  // The block the handle is pointing to cannot have more than 6 elements,
  // so the block is a linear one, and at most an 8-block

  uint32 block_idx = get_payload(low);
  uint32 count = get_count(handle);

  for (uint32 i=0 ; i < count ; i++) {
    uint64 slot = slots[block_idx + i];
    assert(slot != EMPTY_SLOT & get_tag(get_low_32(slot)) == INLINE_SLOT);
    slots[next_idx++] = pack(unclipped(get_low_32(slot), least_bits), get_high_32(slot));
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

static uint64 shrink_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert(HASHED_BLOCK_MIN_COUNT == 7);

  uint64 *target_slots = array_pool->slots + block_idx;

  // Here we've exactly 6 elements left, therefore we need the save the first 6 slots
  uint64 saved_slots[6];
  for (uint32 i=0 ; i < 6 ; i++)
    saved_slots[i] = target_slots[i];

  uint32 next_idx = block_idx;
  for (uint32 i=0 ; i < 6 ; i++)
    next_idx = copy_and_release_block(array_pool, saved_slots[i], next_idx, i);

  uint32 end_idx = block_idx + 6;
  for (uint32 i=6 ; next_idx < end_idx ; i++)
    next_idx = copy_and_release_block(array_pool, target_slots[i], next_idx, i);

  target_slots[6] = EMPTY_SLOT;
  target_slots[7] = EMPTY_SLOT;

  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, 6);
}

////////////////////////////////////////////////////////////////////////////

static uint64 delete_from_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 *data) {
  uint64 *slots = array_pool->slots;

  uint32 tag = get_tag(get_low_32(handle));
  uint32 block_idx = get_payload(get_low_32(handle));
  uint32 count = get_count(handle);

  uint32 last_slot_idx = block_idx + count - 1;
  uint64 last_slot = slots[last_slot_idx];

  assert(last_slot != EMPTY_SLOT);
  assert(count == get_capacity(tag) || slots[block_idx + count] == EMPTY_SLOT);

  uint32 last_low = get_low_32(last_slot);

  // Checking the last slot first
  if (value == last_low) {
    // Removing the value
    slots[last_slot_idx] = EMPTY_SLOT;

    if (data != NULL)
      *data = get_high_32(last_slot);

    // Shrinking the block if need be
    if (count == min_count(tag))
      return shrink_linear_block(array_pool, tag, block_idx, count - 1);
    else
      return linear_block_handle(tag, block_idx, count - 1);
  }

  // The last slot didn't contain the searched value, looking in the rest of the array
  for (uint32 i = last_slot_idx - 1 ; i >= block_idx ; i--) {
    uint64 slot = slots[i];
    uint32 low = get_low_32(slot);

    assert(slot != EMPTY_SLOT && get_tag(low) == INLINE_SLOT);

    if (value == low) {
      // Replacing the value to be deleted with the last one
      slots[i] = last_slot;

      // Clearing the last slot whose value has been stored in the delete slot
      slots[last_slot_idx] = EMPTY_SLOT;

      if (data != NULL)
        *data = get_high_32(slot);

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

uint64 loaded_overflow_table_delete(ARRAY_MEM_POOL *, uint64 handle, uint32 value, uint32 *data);

static uint64 delete_from_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, uint32 *data) {
  uint64 *slots = array_pool->slots;

  uint32 index = get_index(value);
  uint32 slot_idx = block_idx + index;
  uint64 slot = slots[slot_idx];

  // If the slot is empty there's nothing to do
  if (slot == EMPTY_SLOT)
    return hashed_block_handle(block_idx, count);

  uint32 low = get_low_32(slot);

  // If the slot is not inline, we recursively call loaded_overflow_table_delete(..) with a clipped value
  if (get_tag(low) != INLINE_SLOT) {
    uint64 handle = loaded_overflow_table_delete(array_pool, slot, clipped(value), data);
    if (handle == slot)
      return hashed_block_handle(block_idx, count);
    uint32 handle_low = get_low_32(handle);
    if (get_tag(handle_low) == INLINE_SLOT)
      handle = pack(unclipped(handle_low, index), get_high_32(handle));
    slots[slot_idx] = handle;
  }
  else if (low == value) {
    slots[slot_idx] = EMPTY_SLOT;
    if (data != NULL)
      *data = get_high_32(slot);
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

static uint32 linear_block_lookup(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value) {
  uint64 *target_slots = array_pool->slots + block_idx;
  for (uint32 i=0 ; i < count ; i++) {
    uint64 slot = target_slots[i];
    assert(get_tag(get_low_32(slot)) == INLINE_SLOT);
    if (value == get_low_32(slot))
      return get_high_32(slot);
  }
  return 0xFFFFFFFF;
}

static uint32 hashed_block_lookup(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 value) {
  uint64 *slots = array_pool->slots;

  uint32 slot_idx = block_idx + get_index(value);
  uint64 slot = slots[slot_idx];

  if (slot == EMPTY_SLOT)
    return 0xFFFFFFFF;

  uint32 low = get_low_32(slot);
  uint32 tag = get_tag(low);

  if (tag == INLINE_SLOT)
    return value == low ? get_high_32(slot) : 0xFFFFFFFF;

  uint32 subblock_idx = get_payload(low);
  uint32 clipped_value = clipped(value);

  if (tag == HASHED_BLOCK)
    return hashed_block_lookup(array_pool, subblock_idx, clipped_value);
  else
    return linear_block_lookup(array_pool, subblock_idx, get_count(slot), clipped_value);
}

////////////////////////////////////////////////////////////////////////////////

static uint32 copy_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 *surrs2, uint32 *data, uint32 offset, uint32 shift, uint32 least_bits) {
  uint64 *slots = array_pool->slots;

  uint32 subshift = shift + 4;
  uint32 target_idx = offset;

  for (uint32 i=0 ; i < 16 ; i++) {
    uint32 slot_least_bits = (i << shift) + least_bits;
    uint64 slot = slots[block_idx + i];

    if (slot != EMPTY_SLOT) {
      uint32 low = get_low_32(slot);
      uint32 tag = get_tag(low);

      if (tag == INLINE_SLOT) {
        if (surrs2 != NULL)
          surrs2[target_idx] = (get_payload(low) << shift) + least_bits;
        if (data != NULL)
          data[target_idx] = get_high_32(slot);
        target_idx++;
      }
      else if (tag == HASHED_BLOCK) {
        target_idx = copy_hashed_block(array_pool, get_payload(low), surrs2, data, target_idx, subshift, slot_least_bits);
      }
      else {
        uint32 subblock_idx = get_payload(low);
        uint32 count = get_count(slot);

        for (uint32 j=0 ; j < count ; j++) {
          uint64 subslot = slots[subblock_idx + j];

          assert(subslot != EMPTY_SLOT & get_tag(get_low_32(subslot)) == INLINE_SLOT);

          if (surrs2 != NULL)
            surrs2[target_idx] = (get_low_32(subslot) << subshift) + slot_least_bits;
          if (data != NULL)
            data[target_idx] = get_high_32(subslot);
          target_idx++;
        }
      }
    }
  }

  return target_idx;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint64 loaded_overflow_table_insert_unique(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  if (tag == 0)
    return insert_unique_2_block(array_pool, handle, value, data, mem_pool);

  if (tag == HASHED_BLOCK)
    return insert_unique_into_hashed_block(array_pool, get_payload(low), get_count(handle), value, data, mem_pool);

  return insert_unique_with_linear_block(array_pool, handle, value, data, mem_pool);
}

uint64 loaded_overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 *data) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  assert(tag != INLINE_SLOT);

  if (tag == HASHED_BLOCK)
    return delete_from_hashed_block(array_pool, get_payload(low), get_count(handle), value, data);
  else
    return delete_from_linear_block(array_pool, handle, value, data);
}

void loaded_overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle) {
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
    uint64 *target_slots = array_pool->slots + block_idx;
    for (uint32 i=0 ; i < 16 ; i++) {
      uint64 slot = target_slots[i];
      if (slot != EMPTY_SLOT && get_tag(get_low_32(slot)) != INLINE_SLOT)
        loaded_overflow_table_delete(array_pool, slot);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

uint32 loaded_overflow_table_lookup(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);

  assert(tag != INLINE_SLOT);
  assert(pack_tag_payload(tag, block_idx) == get_low_32(handle));

  if (tag != HASHED_BLOCK)
    return linear_block_lookup(array_pool, block_idx, get_count(handle), value);
  else
    return hashed_block_lookup(array_pool, block_idx, value);
}

void loaded_overflow_table_copy(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 *surrs2, uint32 *data, uint32 offset) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);

  assert(tag != INLINE_SLOT);
  assert(pack_tag_payload(tag, block_idx) == get_low_32(handle));

  if (tag != HASHED_BLOCK) {
    uint32 count = get_count(handle);
    uint32 target_idx = offset;

    uint64 *src_slots = array_pool->slots + block_idx;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 slot = src_slots[i];

      assert(slot != EMPTY_SLOT & get_tag(get_low_32(slot)) == INLINE_SLOT);

      if (surrs2 != NULL)
        surrs2[target_idx] = get_low_32(slot);
      if (data != NULL)
        data[target_idx] = get_high_32(slot);

      target_idx++;
    }
  }
  else
    copy_hashed_block(array_pool, block_idx, surrs2, data, offset, 0, 0);
}
