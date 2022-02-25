#include "lib.h"
#include "one-way-bin-table.h"


// A slot can be in any of the following states:
//   - Single value:        32 ones                  - 3 zeros   - 29 bit value 1
//   - Two values:          3 zeros - 29 bit value 2 - 3 zeros   - 29 bit value 1
//   - Index + count:       32 bit count             - 3 bit tag - 29 bit index
//     This type of slot can only be stored in a hashed block or passed in and out
//   - Empty:               32 zeros                 - 32 ones
//     This type of slot can only be stored in a block, but cannot be passed in or out

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

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *, uint32, uint32, uint32, uint32, STATE_MEM_POOL *);

static uint64 insert_unique_with_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  assert(get_tag(get_low_32(handle)) >= SIZE_2_BLOCK & get_tag(get_low_32(handle)) <= SIZE_16_BLOCK);

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);
  uint32 count = get_count(handle);
  uint32 capacity = get_capacity(tag);

  // Inserting the new value if there's still room here
  if (count < capacity) {
    uint64 *slots = array_pool->slots;
    uint32 distance = array_pool->size;

    uint64 *slot_ptr = slots + block_idx + count / 2;
    uint64 *data_slot_ptr = slot_ptr + distance;

    if (is_even(count)) {
      *slot_ptr = pack(value, EMPTY_MARKER);
      *data_slot_ptr = pack(data, 0);
    }
    else {
      set_high_32(slot_ptr, value);
      set_high_32(data_slot_ptr, data);
    }
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

    // These must be read only after the above memory allocation
    uint64 *slots = array_pool->slots;
    uint32 distance = array_pool->size;

    // Initializing the new block

    uint32 word_count = count / 2;
    uint64 *src_slots = slots + block_idx;
    uint64 *tgt_slots = slots + new_block_idx;

    // Copying the values
    memcpy(tgt_slots, src_slots, word_count * sizeof(uint64));
    tgt_slots[word_count] = pack(value, EMPTY_MARKER);
    for (uint32 i=word_count+1 ; i < count ; i++)
      tgt_slots[i] = EMPTY_SLOT;

    // Copying the attached data
    src_slots += distance;
    tgt_slots += distance;
    memcpy(tgt_slots, src_slots, word_count * sizeof(uint64));
    tgt_slots[word_count] = pack(data, EMPTY_MARKER);

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

  // These must be read only after the above memory allocation
  uint64 *slots = array_pool->slots;
  uint32 distance = array_pool->size;

  for (uint32 i=0 ; i < 16 ; i++)
    slots[hashed_block_idx + i] = EMPTY_SLOT;

  // Transferring the existing values
  for (uint32 i=0 ; i < 16 ; i++) {
    uint64 *src_slots = array_pool->slots + block_idx; // Reading the slot pointer from the source, the local copy may be stale
    uint64 slot = src_slots[i];
    uint64 data_slot = src_slots[distance + i];
    uint64 tmp_handle = insert_unique_into_hashed_block(array_pool, hashed_block_idx, 2 * i, get_low_32(slot), get_low_32(data_slot), mem_pool);
    assert(get_count(tmp_handle) == 2 * i + 1);
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
    tmp_handle = insert_unique_into_hashed_block(array_pool, hashed_block_idx, 2 * i + 1, get_high_32(slot), get_high_32(data_slot), mem_pool);
    assert(get_count(tmp_handle) == 2 * (i + 1));
    assert(get_payload(get_low_32(tmp_handle)) == hashed_block_idx);
  }

  // Releasing the old block
  array_mem_pool_release_16_block(array_pool, block_idx);

  // Adding the new value
  return insert_unique_into_hashed_block(array_pool, hashed_block_idx, 32, value, data, mem_pool);
}

static uint64 insert_unique_into_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint64 *slots = array_pool->slots;

  uint32 slot_idx = block_idx + get_index(value);
  uint32 data_slot_idx = slot_idx + array_pool->size;
  uint64 slot = slots[slot_idx];
  uint32 low = get_low_32(slot);

  // Checking for empty slots
  if (low == EMPTY_MARKER) {
    slots[slot_idx] = pack(value, EMPTY_MARKER);
    slots[data_slot_idx] = pack(data, 0);
    return hashed_block_handle(block_idx, count + 1);
  }

  uint32 tag = get_tag(low);

  // Checking for inline slots
  if (tag == INLINE_SLOT) {
    assert(value != low);
    uint32 high = get_high_32(slot);
    if (high == EMPTY_MARKER) {
      slots[slot_idx] = pack(low, value);
      set_high_32(slots + data_slot_idx, data);
      return hashed_block_handle(block_idx, count + 1);
    }
    assert(get_tag(high) == INLINE_SLOT);
    assert(value != high);
    uint64 data_slot = slots[data_slot_idx];
    uint64 handle = loaded_overflow_table_create_new_block(array_pool, pack(clipped(low), clipped(high)), data_slot, clipped(value), data, mem_pool);
    assert(get_count(handle) == 3);
    array_pool->slots[slot_idx] = handle; // Using the uncached version of the pointer
    return hashed_block_handle(block_idx, count + 1);
  }

  // The slot is not an inline one. Inserting the clipped value into the subblock

  uint64 handle;
  if (tag == HASHED_BLOCK)
    handle = insert_unique_into_hashed_block(array_pool, get_payload(low), get_count(slot), clipped(value), data, mem_pool);
  else
    handle = insert_unique_with_linear_block(array_pool, slot, clipped(value), data, mem_pool);

  assert(get_count(handle) == get_count(slot) + 1);
  array_pool->slots[slot_idx] = handle; // Using the uncached version of the pointer
  return hashed_block_handle(block_idx, count + 1);
}

////////////////////////////////////////////////////////////////////////////////

static uint64 shrink_linear_block(ARRAY_MEM_POOL *array_pool, uint32 tag, uint32 block_idx, uint32 count, uint64 *target_size_2_data_slot_ptr) {
  uint64 *slots = array_pool->slots;
  uint32 distance = array_pool->size;

  if (tag == SIZE_2_BLOCK) {
    assert(count == 2);
    uint64 *slot_ptr = slots + block_idx;
    uint64 slot = *slot_ptr;
    *target_size_2_data_slot_ptr = *(slot_ptr + distance);
    //## BUG BUG BUG: SHOULDN'T THE BLOCK BE RELEASED?
    return slots[block_idx];
  }

  if (tag == SIZE_4_BLOCK) {
    assert(count == 3);

    // Reading the slots that are still in use
    uint64 *src_slots = slots + block_idx;
    uint64 slot_0 = src_slots[0];
    uint64 slot_1 = src_slots[1];

    // Reading their attached data
    src_slots += distance;
    uint64 data_slot_0 = src_slots[0];
    uint64 data_slot_1 = src_slots[1];

    // Reallocating the block
    array_mem_pool_release_4_block(array_pool, block_idx);
    uint32 size_2_block_idx = array_mem_pool_alloc_2_block(array_pool, NULL); // We've just released a block of size 4, no need to allocate new memory
    assert(slots == array_pool->slots && distance == array_pool->size); //## THIS ONE SHOULD BE FINE

    // Copying back the primary data
    uint64 *tgt_slots = slots + size_2_block_idx;
    tgt_slots[0] = slot_0;
    tgt_slots[1] = slot_1;

    // Writing the attached data
    tgt_slots += distance;
    tgt_slots[0] = data_slot_0;
    tgt_slots[1] = data_slot_1;

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

static void copy_and_release_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 data_slot, uint32 least_bits, uint32 &write_idx, uint32 &pending_value, uint32 &pending_data) {
  if (handle == EMPTY_SLOT)
    return;

  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  uint64 *slots = array_pool->slots;
  uint64 *data_slots = slots + array_pool->size;

  if (tag == INLINE_SLOT) {
    uint32 high = get_high_32(handle);

    if (pending_value != EMPTY_MARKER) {
      slots[write_idx] = pack(pending_value, low);
      data_slots[write_idx++] = pack(pending_data, get_low_32(data_slot));
      pending_value = EMPTY_MARKER;
    }
    else {
      pending_value = low;
      pending_data = get_low_32(data_slot);
    }

    if (high != EMPTY_MARKER)
      if (pending_value != EMPTY_MARKER) {
        slots[write_idx] = pack(pending_value, high);
        data_slots[write_idx++] = pack(pending_data, get_high_32(data_slot));
        pending_value = EMPTY_MARKER;
      }
      else {
        pending_value = high;
        pending_data = get_high_32(data_slot);
      }
  }
  else {
    uint32 block_idx = get_payload(low);
    uint32 end = (get_count(handle) + 1) / 2;

    for (uint32 i=0 ; i < end ; i++) {
      uint64 slot = slots[block_idx + i];
      uint64 data_slot = data_slots[block_idx + i];

      assert(slot != EMPTY_SLOT);

      uint32 slot_low = get_low_32(slot);
      uint32 slot_high = get_high_32(slot);

      if (pending_value != EMPTY_MARKER) {
        slots[write_idx] = pack(pending_value, unclipped(slot_low, least_bits));
        data_slots[write_idx++] = pack(pending_data, get_low_32(data_slot));
        pending_value = EMPTY_MARKER;
      }
      else {
        pending_value = unclipped(slot_low, least_bits);
        pending_data = get_low_32(data_slot);
      }

      if (slot_high != EMPTY_MARKER) {
        if (pending_value != EMPTY_MARKER) {
          slots[write_idx] = pack(pending_value, unclipped(slot_high, least_bits));
          data_slots[write_idx++] = pack(pending_data, get_high_32(data_slot));
          pending_value = EMPTY_MARKER;
        }
        else {
          pending_value = unclipped(slot_high, least_bits);
          pending_data = get_high_32(data_slot);
        }
      }
    }

    if (tag == SIZE_2_BLOCK) {
      array_mem_pool_release_2_block(array_pool, block_idx);
    }
    else if (tag == SIZE_4_BLOCK) {
      array_mem_pool_release_4_block(array_pool, block_idx);
    }
    else {
      // Both 16-slot and hashed blocks contain at least 13 elements, so they cannot appear
      // here, as the parent hashed block being shrunk has only 12 elements left
      assert(tag == SIZE_8_BLOCK);
      array_mem_pool_release_8_block(array_pool, block_idx);
    }
  }
}

static uint64 shrink_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert(HASHED_BLOCK_MIN_COUNT == 13);

  uint64 *target_slots = array_pool->slots + block_idx;
  uint64 *target_data_slots = target_slots + array_pool->size;

  //## THIS APPROACH DOESN'T MAKE ANY SENSE. JUST ALLOC A NEW BLOCK, IT'S VERY LIKELY FASTER AND IT LEAVES THE MEMORY LESS FRAGMENTED

  // Here we've exactly 12 elements left, therefore we need the save the first 6 slots
  uint64 saved_slots[6], saved_data_slots[6];
  for (uint32 i=0 ; i < 6 ; i++) {
    saved_slots[i] = target_slots[i];
    saved_data_slots[i] = target_slots[i];
  }

  uint32 write_idx = block_idx;
  uint32 pending_value = EMPTY_MARKER;
  uint32 pending_data;

  for (uint32 i=0 ; i < 6 ; i++)
    copy_and_release_block(array_pool, saved_slots[i], saved_data_slots[i], i, write_idx, pending_value, pending_data);

  uint32 end_idx = block_idx + 6;
  for (uint32 i=6 ; write_idx < end_idx ; i++)
    copy_and_release_block(array_pool, target_slots[i], target_data_slots[i], i, write_idx, pending_value, pending_data);

  assert(write_idx == end_idx && pending_value == EMPTY_MARKER);

  target_slots[6] = EMPTY_SLOT;
  target_slots[7] = EMPTY_SLOT;

  array_mem_pool_release_16_block_upper_half(array_pool, block_idx);
  return size_8_block_handle(block_idx, 12);
}

////////////////////////////////////////////////////////////////////////////////

static uint64 delete_from_linear_block(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint64 *target_size_2_data_slot_ptr) {
  uint64 *slots = array_pool->slots;
  uint32 distance = array_pool->size;

  uint32 tag = get_tag(get_low_32(handle));
  uint32 block_idx = get_payload(get_low_32(handle));
  uint32 count = get_count(handle);

  uint32 last_slot_idx = (count + 1) / 2 - 1;
  uint64 *last_slot_ptr = slots + block_idx + last_slot_idx;
  uint64 *last_slot_data_ptr = last_slot_ptr + distance;
  uint64 last_slot = *last_slot_ptr;
  uint64 last_slot_data = *last_slot_data_ptr;

  uint32 last_low = get_low_32(last_slot);
  uint32 last_high = get_high_32(last_slot);

  assert(last_low != EMPTY_MARKER && get_tag(last_low) == INLINE_SLOT);
  assert((last_high != EMPTY_MARKER && get_tag(last_high) == INLINE_SLOT) || (last_high == EMPTY_MARKER && !is_even(count)));

  // Checking the last slot first
  if (value == last_low | value == last_high) {
    // Removing the value
    if (value == last_low) {
      *last_slot_ptr = pack(last_high, EMPTY_MARKER);
      *last_slot_data_ptr = pack(get_high_32(last_slot_data), 0);
    }
    else {
      *last_slot_ptr = pack(last_low, EMPTY_MARKER);
      *last_slot_data_ptr = pack(get_low_32(last_slot_data), 0);
    }

    // Shrinking the block if need be
    if (count == min_count(tag))
      return shrink_linear_block(array_pool, tag, block_idx, count - 1, target_size_2_data_slot_ptr);
    else
      return linear_block_handle(tag, block_idx, count - 1);
  }

  // The last slot didn't contain the searched value, looking in the rest of the array
  for (uint32 i = last_slot_idx - 1 ; i >= 0 ; i--) {
    uint64 *slot_ptr = slots + block_idx + i;
    uint64 slot = *slot_ptr;
    uint32 low = get_low_32(slot);
    uint32 high = get_high_32(slot);

    assert(low != EMPTY_MARKER && get_tag(low) == INLINE_SLOT);
    assert(high != EMPTY_MARKER && get_tag(high) == INLINE_SLOT);

    if (value == low | value == high) {
      uint64 *slot_data_ptr = slot_ptr + distance;
      uint64 slot_data = *slot_data_ptr;

      // Removing the last value to replace the one being deleted
      uint32 last, last_data;
      if (is_even(count)) {
        last = last_high;
        last_data = get_high_32(slot_data);
        *last_slot_ptr = pack(last_low, EMPTY_MARKER);
        *last_slot_data_ptr = pack(get_low_32(last_slot_data), 0);
      }
      else {
        last = last_low;
        last_data = get_low_32(slot_data);
        *last_slot_ptr = EMPTY_SLOT;
      }

      // Replacing the value to be deleted with the last one
      if (value == low) {
        *slot_ptr = pack(last, high);
        *slot_data_ptr = pack(last_data, get_high_32(slot_data));
      }
      else {
        *slot_ptr = pack(low, last);
        *slot_data_ptr = pack(get_low_32(slot_data), last_data);
      }

      // Shrinking the block if need be
      if (count == min_count(tag))
        return shrink_linear_block(array_pool, tag, block_idx, count - 1, target_size_2_data_slot_ptr);
      else
        return linear_block_handle(tag, block_idx, count - 1);
    }
  }

  // Value not found
  return handle;
}

static uint64 delete_from_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 count, uint32 value) {
  uint64 *slots = array_pool->slots;

  uint32 index = get_index(value);
  uint64 *slot_ptr = slots + block_idx + index;
  uint64 *data_slot_ptr = slots + array_pool->size;
  uint64 slot = *slot_ptr;

  // If the slot is empty there's nothing to do
  if (slot == EMPTY_SLOT)
    return hashed_block_handle(block_idx, count);

  uint32 low = get_low_32(slot);
  uint32 high = get_high_32(slot);

  // If the slot is not inline, we recursively call loaded_overflow_table_delete(..) with a clipped value
  if (get_tag(low) != INLINE_SLOT) {
    uint64 handle = loaded_overflow_table_delete(array_pool, slot, clipped(value), data_slot_ptr); //## I'D LIKE TO REMOVE THIS DEPENDENCY FROM A PUBLIC FUNCTION
    if (handle == slot)
      return hashed_block_handle(block_idx, count);
    uint32 handle_low = get_low_32(handle);
    if (get_tag(handle_low) == INLINE_SLOT)
      handle = pack(unclipped(handle_low, index), unclipped(get_high_32(handle), index));
    *slot_ptr = handle;
  }
  else if (low == value) {
    if (high != EMPTY_MARKER) {
      uint64 data_slot = *data_slot_ptr;
      *slot_ptr = pack(high, EMPTY_MARKER);
      *data_slot_ptr = pack(get_high_32(data_slot), 0);
    }
    else
      *slot_ptr = EMPTY_SLOT;
  }
  else if (high == value) {
    assert(high != EMPTY_MARKER);
    *slot_ptr = pack(low, EMPTY_MARKER);
    // Upper part of the slot was erased, not moved, so no need to do anything to the corresponding data slot
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
  uint64 *slots = array_pool->slots + block_idx;
  uint32 end = (count + 1) / 2;
  for (uint32 i=0 ; i < end ; i++) {
    uint64 slot = slots[i];
    if (value == get_low_32(slot)| value == get_high_32(slot))  {
      uint64 data_slot = slots[i + array_pool->size];
      return value == get_low_32(slot) ? get_low_32(data_slot) : get_high_32(data_slot);
    }
  }
  return 0xFFFFFFFF;
}

static uint32 hashed_block_lookup(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 value) {
  uint64 *slot_ptr = array_pool->slots + block_idx + get_index(value);
  uint64 slot = *slot_ptr;

  if (slot == EMPTY_SLOT)
    return 0xFFFFFFFF;

  uint32 low = get_low_32(slot);
  uint32 tag = get_tag(low);

  if (tag == INLINE_SLOT & (value == low | value == get_high_32(slot))) {
    uint64 data_slot = *(slot_ptr + array_pool->size);
    return value == low ? get_low_32(data_slot) : get_high_32(data_slot);
  }

  if (tag == HASHED_BLOCK)
    return hashed_block_lookup(array_pool, get_payload(low), clipped(value));

  return loaded_overflow_table_lookup(array_pool, slot, clipped(value)); //## I'D LIKE TO REMOVE THIS DEPENDENCY FROM A PUBLIC FUNCTION
}

////////////////////////////////////////////////////////////////////////////////

static uint32 copy_hashed_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx, uint32 *values, uint32 *data, uint32 offset, uint32 shift, uint32 least_bits) {
  uint64 *slots = array_pool->slots;
  uint32 distance = array_pool->size;

  uint32 subshift = shift + 4;
  uint32 target_idx = offset;

  for (uint32 i=0 ; i < 16 ; i++) {
    uint32 slot_least_bits = (i << shift) + least_bits;
    uint64 *slot_ptr = slots + block_idx + i;
    uint64 slot = *slot_ptr;
    uint32 low = get_low_32(slot);

    if (low != EMPTY_MARKER) {
      uint32 tag = get_tag(low);
      if (tag == INLINE_SLOT) {
        uint64 data_slot = *(slot_ptr + distance);

        values[target_idx] = (get_payload(low) << shift) + least_bits;
        data[target_idx++] = get_low_32(data_slot);

        uint32 high = get_high_32(slot);
        if (high != EMPTY_MARKER) {
          values[target_idx] = (get_payload(high) << shift) + least_bits;
          data[target_idx++] = get_high_32(data_slot);
        }
      }
      else if (tag == HASHED_BLOCK) {
        target_idx = copy_hashed_block(array_pool, get_payload(low), values, data, target_idx, subshift, slot_least_bits);
      }
      else {
        uint32 subblock_idx = get_payload(low);
        uint32 count = get_count(slot);
        uint32 end = (count + 1) / 2;

        for (uint32 j=0 ; j < end ; j++) {
          uint64 *subslot_ptr = slots + subblock_idx + j;
          uint64 subslot = *subslot_ptr;
          uint32 sublow = get_low_32(subslot);
          uint32 subhigh = get_high_32(subslot);

          assert(sublow != EMPTY_MARKER & get_tag(sublow) == 0);

          uint64 data_subslot = *(subslot_ptr + distance);

          values[target_idx] = (sublow << subshift) + slot_least_bits;
          data[target_idx++] = get_low_32(data_subslot);

          if (subhigh != EMPTY_MARKER) {
            values[target_idx] = (subhigh << subshift) + slot_least_bits;
            data[target_idx++] = get_high_32(data_subslot);
          }
        }
      }
    }
  }

  return target_idx;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint64 loaded_overflow_table_create_new_block(ARRAY_MEM_POOL *array_pool, uint64 values, uint64 datas, uint32 new_value, uint32 new_data, STATE_MEM_POOL *mem_pool) {
  assert(get_tag(get_low_32(values)) == 0 && get_tag(get_high_32(values)) == 0 && get_tag(new_value) == 0);

  uint32 block_idx = array_mem_pool_alloc_2_block(array_pool, mem_pool);
  uint64 *slots = array_pool->slots;
  uint64 *value_slots = slots + block_idx;
  value_slots[0] = values;
  value_slots[1] = pack(new_value, EMPTY_MARKER);
  uint64 *data_slots = value_slots + array_pool->size;
  data_slots[0] = datas;
  data_slots[1] = pack(new_data, 0);
  return size_2_block_handle(block_idx, 3);
}

uint64 loaded_overflow_table_insert_unique(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *mem_pool) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  assert(tag != INLINE_SLOT);

  if (tag == HASHED_BLOCK)
    return insert_unique_into_hashed_block(array_pool, get_payload(low), get_count(handle), value, data, mem_pool);

  return insert_unique_with_linear_block(array_pool, handle, value, data, mem_pool);
}

uint64 loaded_overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, uint64 *target_size_2_data_slot_ptr) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);

  assert(tag != INLINE_SLOT);

  if (tag == HASHED_BLOCK)
    return delete_from_hashed_block(array_pool, get_payload(low), get_count(handle), value);
  else
    return delete_from_linear_block(array_pool, handle, value, target_size_2_data_slot_ptr);
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

////////////////////////////////////////////////////////////////////////////////

void loaded_overflow_table_copy(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 *values, uint32 *data, uint32 offset) {
  uint32 low = get_low_32(handle);
  uint32 tag = get_tag(low);
  uint32 block_idx = get_payload(low);

  assert(tag != INLINE_SLOT);
  assert(pack_tag_payload(tag, block_idx) == get_low_32(handle));

  if (tag != HASHED_BLOCK) {
    uint32 count = get_count(handle);
    uint32 end = (count + 1) / 2;
    uint32 target_idx = offset;

    uint64 *src_slots = array_pool->slots + block_idx;
    uint32 distance = array_pool->size;

    for (uint32 i=0 ; i < end ; i++) {
      uint64 slot = src_slots[i];
      uint32 slot_low = get_low_32(slot);
      uint32 slot_high = get_high_32(slot);

      assert(slot_low != EMPTY_MARKER & get_tag(slot_low) == INLINE_SLOT);

      uint64 data_slot = src_slots[i + distance];

      values[target_idx] = slot_low;
      data[target_idx++] = get_low_32(data_slot);

      if (slot_high != EMPTY_MARKER) {
        assert(get_tag(slot_high) == INLINE_SLOT);

        values[target_idx] = slot_high;
        data[target_idx++] = get_high_32(data_slot);
      }
    }
  }
  else
    copy_hashed_block(array_pool, block_idx, values, data, offset, 0, 0);
}
