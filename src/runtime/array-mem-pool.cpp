#include "lib.h"
#include "one-way-bin-table.h"


const uint32 MIN_SIZE = 32;

const uint32 END_LOWER_MARKER     = 0xFFFFFFFF;
const uint32 END_UPPER_MARKER_2   = 0x3FFFFFFF;
const uint32 END_UPPER_MARKER_4   = 0x5FFFFFFF;
const uint32 END_UPPER_MARKER_8   = 0x7FFFFFFF;
const uint32 END_UPPER_MARKER_16  = 0x9FFFFFFF;

// const uint32 BLOCK_1    = 0;
const uint32 BLOCK_2    = 1;
const uint32 BLOCK_4    = 2;
const uint32 BLOCK_8    = 3;
const uint32 BLOCK_16   = 4;
// const uint32 BLOCK_32   = 5;
// const uint32 BLOCK_64   = 6;
const uint32 AVAILABLE  = 7;

////////////////////////////////////////////////////////////////////////////////

// inline uint32 get_tag(uint32 word) {
//   return word >> 29;
// }

// inline uint32 get_payload(uint32 word) {
//   return word & PAYLOAD_MASK;
// }

// inline uint32 pack_tag_payload(uint32 tag, uint32 payload) {
//   assert(get_tag(payload) == 0);
//   return (tag << 29) | payload;
// }

////////////////////////////////////////////////////////////////////////////////

void array_mem_pool_init(ARRAY_MEM_POOL *array, bool alloc_double_space, STATE_MEM_POOL *mem_pool) {
  uint64 *slots = alloc_state_mem_uint64_array(mem_pool, alloc_double_space ? 2 * MIN_SIZE : MIN_SIZE);

  slots[0] = pack(END_LOWER_MARKER, pack_tag_payload(BLOCK_16, 16));
  for (uint32 i=16 ; i < MIN_SIZE - 16 ; i += 16)
    slots[i] = pack(pack_tag_payload(AVAILABLE, i - 16), pack_tag_payload(BLOCK_16, i + 16));
  slots[MIN_SIZE - 16] = pack(pack_tag_payload(AVAILABLE, MIN_SIZE - 32), END_UPPER_MARKER_16);

  array->slots = slots;
  array->size = MIN_SIZE;
  array->head2 = EMPTY_MARKER;
  array->head4 = EMPTY_MARKER;
  array->head8 = EMPTY_MARKER;
  array->head16 = 0;
  array->alloc_double_space = alloc_double_space;
}

void array_mem_pool_release(ARRAY_MEM_POOL *array, STATE_MEM_POOL *mem_pool) {
  uint32 alloc_size = array->size;
  if (array->alloc_double_space)
    alloc_size *= 2;
  uint64 *slots = array->slots;
  release_state_mem_uint64_array(mem_pool, slots, alloc_size);

}

void array_mem_pool_clear(ARRAY_MEM_POOL *array, STATE_MEM_POOL *mem_pool) {
  uint32 size = array->size;
  uint64 *slots = array->slots;

  if (size > MIN_SIZE) { //## WHAT WOULD BE A GOOD VALUE FOR THE REALLOCATION THRESHOLD?
    release_state_mem_uint64_array(mem_pool, slots, array->alloc_double_space ? 2 * size : size);
    slots = alloc_state_mem_uint64_array(mem_pool, array->alloc_double_space ? 2 * MIN_SIZE : MIN_SIZE);
    array->slots = slots;
    size = MIN_SIZE;
    array->size = size;
  }

  slots[0] = pack(END_LOWER_MARKER, pack_tag_payload(BLOCK_16, 16));
  for (uint32 i=16 ; i < MIN_SIZE - 16 ; i += 16)
    slots[i] = pack(pack_tag_payload(AVAILABLE, i - 16), pack_tag_payload(BLOCK_16, i + 16));
  slots[MIN_SIZE - 16] = pack(pack_tag_payload(AVAILABLE, MIN_SIZE - 32), END_UPPER_MARKER_16);

  array->head2 = EMPTY_MARKER;
  array->head4 = EMPTY_MARKER;
  array->head8 = EMPTY_MARKER;
  array->head16 = 0;
}

////////////////////////////////////////////////////////////////////////////////

static uint32 add_block_to_chain(ARRAY_MEM_POOL *array, uint32 block_idx, uint32 size_tag, uint32 end_upper_marker, uint32 head) {
  uint64 *slots = array->slots;
  if (head != EMPTY_MARKER) {
    // If the list of blocks is not empty, we link the first two blocks
    // The 'previous' field of the newly released block must be cleared
    slots[block_idx] = pack(END_LOWER_MARKER, pack_tag_payload(size_tag, head));
    slots[head] = set_low_32(slots[head], pack_tag_payload(AVAILABLE, block_idx));
  }
  else {
    // Otherwise we just clear then 'next' field of the newly released block
    // The 'previous' field of the newly released block must be cleared
    slots[block_idx] = pack(END_LOWER_MARKER, end_upper_marker);
  }
  // The new block becomes the head one
  return block_idx;
}

static uint32 remove_block_from_chain(ARRAY_MEM_POOL *array, uint32 block_idx, uint64 first_slot, uint32 end_upper_marker, uint32 head) {
  uint32 first_low = get_low_32(first_slot);
  uint32 first_high = get_high_32(first_slot);

  if (first_low != END_LOWER_MARKER) {
    // Not the first block in the chain
    assert(head != block_idx);
    uint32 prev_block_idx = get_payload(first_low);

    uint64 *slots = array->slots;

    if (first_high != end_upper_marker) {
      // The block is in the middle of the chain
      // The previous and next blocks must be repointed to each other
      uint32 next_block_idx = get_payload(first_high);
      slots[prev_block_idx] = set_high_32(slots[prev_block_idx], first_high); // setSlotHigh(prev_block_idx, first_high);
      slots[next_block_idx] = set_low_32(slots[next_block_idx], first_low);   // setSlotLow(next_block_idx, first_low);
    }
    else {
      // Last block in a chain with multiple blocks
      // The 'next' field of the previous block must be cleared
      slots[prev_block_idx] = set_high_32(slots[prev_block_idx], end_upper_marker); // setSlotHigh(prev_block_idx, end_upper_marker);
    }
  }
  else {
    // First slot in the chain, must be the one pointed to by head
    assert(head == block_idx);

    if (first_high != end_upper_marker) {
      uint64 *slots = array->slots;
      // The head must be repointed at the next block,
      // whose 'previous' field must now be cleared
      uint32 next_block_idx = get_payload(first_high);
      head = next_block_idx;
      slots[next_block_idx] = set_low_32(slots[next_block_idx], END_LOWER_MARKER); // setSlotLow(next_block_idx, END_LOWER_MARKER);
    }
    else {
      // No 'previous' nor 'next' slots, it must be the only one
      // Just resetting the head of the 2-slot block chain
      head = EMPTY_MARKER;
    }
  }

  return head;
}

////////////////////////////////////////////////////////////////////////////////

uint32 array_mem_pool_alloc_16_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool) {
  uint64 *slots = array_pool->slots;
  uint32 head16 = array_pool->head16;

  if (head16 == EMPTY_MARKER) {
    uint32 size = array_pool->size;
    uint32 new_size = 2 * size;
    if (array_pool->alloc_double_space) {
      uint64 *new_slots = alloc_state_mem_uint64_array(mem_pool, 2 * new_size);
      uint32 mem_size = size * sizeof(uint64);
      memcpy(new_slots, slots, mem_size);
      memcpy(new_slots + new_size, slots + size, mem_size);
      release_state_mem_uint64_array(mem_pool, slots, 2 * size);
      slots = new_slots;
    }
    else
      slots = extend_state_mem_uint64_array(mem_pool, slots, size, new_size);
    for (uint32 i=size ; i < new_size ; i += 16)
      slots[i] = pack(pack_tag_payload(AVAILABLE, i - 16), pack_tag_payload(BLOCK_16, i + 16));

    assert(get_high_32(slots[size]) == pack_tag_payload(BLOCK_16, size + 16));
    assert(get_low_32(slots[new_size - 16]) == pack_tag_payload(AVAILABLE, 2 * size - 32));

    slots[size] = pack(END_LOWER_MARKER, pack_tag_payload(BLOCK_16, size + 16));
    slots[new_size - 16] = pack(pack_tag_payload(AVAILABLE, new_size - 32), END_UPPER_MARKER_16);

    head16 = size;

    array_pool->slots = slots;
    array_pool->size = new_size;
    array_pool->head16 = head16;
  }

  assert(get_low_32(slots[head16]) == END_LOWER_MARKER);
  assert(get_high_32(slots[head16]) == END_UPPER_MARKER_16 | get_tag(get_high_32(slots[head16])) == BLOCK_16);

  uint32 block_idx = head16;
  array_pool->head16 = remove_block_from_chain(array_pool, block_idx, slots[block_idx], END_UPPER_MARKER_16, head16);
  return block_idx;
}

void array_mem_pool_release_16_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  array_pool->head16 = add_block_to_chain(array_pool, block_idx, BLOCK_16, END_UPPER_MARKER_16, array_pool->head16);
}

void array_mem_pool_release_16_block_upper_half(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  array_pool->head8 = add_block_to_chain(array_pool, block_idx + 8, BLOCK_8, END_UPPER_MARKER_8, array_pool->head8);
}

////////////////////////////////////////////////////////////////////////////////

uint32 array_mem_pool_alloc_8_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool) {
  uint32 head8 = array_pool->head8;

  if (head8 != EMPTY_MARKER) {
    uint64 *slots = array_pool->slots;

    assert(get_low_32(slots[head8]) == END_LOWER_MARKER);
    assert(get_high_32(slots[head8]) == END_UPPER_MARKER_8 | get_tag(get_high_32(slots[head8])) == BLOCK_8);

    uint32 block_idx = head8;
    array_pool->head8 = remove_block_from_chain(array_pool, block_idx, slots[block_idx], END_UPPER_MARKER_8, head8);
    return block_idx;
  }
  else {
    uint32 block_idx_16 = array_mem_pool_alloc_16_block(array_pool, mem_pool);

    assert(get_low_32(array_pool->slots[block_idx_16]) == END_LOWER_MARKER);
    assert(get_high_32(array_pool->slots[block_idx_16]) == END_UPPER_MARKER_16 | get_tag(get_high_32(array_pool->slots[block_idx_16])) == BLOCK_16);

    array_pool->head8 = add_block_to_chain(array_pool, block_idx_16, BLOCK_8, END_UPPER_MARKER_8, head8);
    return block_idx_16 + 8;
  }
}

void array_mem_pool_release_8_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert((block_idx & 7) == 0);

  uint64 *slots = array_pool->slots;

  bool is_first = (block_idx & 15) == 0;
  uint32 other_block_idx = block_idx + (is_first ? 8 : -8);
  uint64 other_block_slot_0 = slots[other_block_idx];

  if (get_tag(get_low_32(other_block_slot_0)) == AVAILABLE & get_tag(get_high_32(other_block_slot_0)) == BLOCK_8) {
    array_pool->head8 = remove_block_from_chain(array_pool, other_block_idx, other_block_slot_0, END_UPPER_MARKER_8, array_pool->head8);
    array_mem_pool_release_16_block(array_pool, is_first ? block_idx : other_block_idx);
  }
  else
    array_pool->head8 = add_block_to_chain(array_pool, block_idx, BLOCK_8, END_UPPER_MARKER_8, array_pool->head8);
}

void array_mem_pool_release_8_block_upper_half(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  array_pool->head4 = add_block_to_chain(array_pool, block_idx + 4, BLOCK_4, END_UPPER_MARKER_4, array_pool->head4);
}

////////////////////////////////////////////////////////////////////////////////

uint32 array_mem_pool_alloc_4_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool) {
  uint32 head4 = array_pool->head4;

  if (head4 != EMPTY_MARKER) {
    uint64 *slots = array_pool->slots;

    assert(get_low_32(slots[head4]) == END_LOWER_MARKER);
    assert(get_high_32(slots[head4]) == END_UPPER_MARKER_4 | get_tag(get_high_32(slots[head4])) == BLOCK_4);

    uint32 block_idx = head4;
    array_pool->head4 = remove_block_from_chain(array_pool, block_idx, slots[block_idx], END_UPPER_MARKER_4, head4);
    return block_idx;
  }
  else {
    uint32 block_idx_8 = array_mem_pool_alloc_8_block(array_pool, mem_pool);
    array_pool->head4 = add_block_to_chain(array_pool, block_idx_8, BLOCK_4, END_UPPER_MARKER_4, head4);
    return block_idx_8 + 4;
  }
}

void array_mem_pool_release_4_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert((block_idx & 3) == 0);

  bool is_first = (block_idx & 7) == 0;
  uint32 other_block_idx = block_idx + (is_first ? 4 : -4);
  uint64 other_block_slot_0 = array_pool->slots[other_block_idx];

  if (get_tag(get_low_32(other_block_slot_0)) == AVAILABLE & get_tag(get_high_32(other_block_slot_0)) == BLOCK_4) {
    array_pool->head4 = remove_block_from_chain(array_pool, other_block_idx, other_block_slot_0, END_UPPER_MARKER_4, array_pool->head4);
    array_mem_pool_release_8_block(array_pool, is_first ? block_idx : other_block_idx);
  }
  else
    array_pool->head4 = add_block_to_chain(array_pool, block_idx, BLOCK_4, END_UPPER_MARKER_4, array_pool->head4);
}

////////////////////////////////////////////////////////////////////////////////

uint32 array_mem_pool_alloc_2_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool) {
  uint32 head2 = array_pool->head2;

  if (head2 != EMPTY_MARKER) {
    uint64 *slots = array_pool->slots;

    assert(get_low_32(array_pool->slots[head2]) == END_LOWER_MARKER);
    assert(get_high_32(array_pool->slots[head2]) == END_UPPER_MARKER_2 || get_tag(get_high_32(array_pool->slots[head2])) == BLOCK_2);

    uint32 block_idx = array_pool->head2;
    array_pool->head2 = remove_block_from_chain(array_pool, block_idx, slots[block_idx], END_UPPER_MARKER_2, head2);
    return block_idx;
  }
  else {
    uint32 block_idx_4 = array_mem_pool_alloc_4_block(array_pool, mem_pool);
    array_pool->head2 = add_block_to_chain(array_pool, block_idx_4, BLOCK_2, END_UPPER_MARKER_2, head2);
    return block_idx_4 + 2;
  }
}

void array_mem_pool_release_2_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx) {
  assert((block_idx & 1) == 0);

  bool is_first = (block_idx & 3) == 0;
  uint32 other_block_idx = block_idx + (is_first ? 2 : -2);
  uint64 other_block_slot_0 = array_pool->slots[other_block_idx];

  if (get_tag(get_low_32(other_block_slot_0)) == AVAILABLE) {
    assert(get_tag(get_high_32(other_block_slot_0)) == BLOCK_2);

    // The matching block is available, so we release both at once as a 4-slot block
    // But first we have to remove the matching block from the 2-slot block chain
    array_pool->head2 = remove_block_from_chain(array_pool, other_block_idx, other_block_slot_0, END_UPPER_MARKER_2, array_pool->head2);
    array_mem_pool_release_4_block(array_pool, is_first ? block_idx : other_block_idx);
  }
  else {
    // The matching block is not available, so we
    // just add the new one to the 2-slot block chain
    array_pool->head2 = add_block_to_chain(array_pool, block_idx, BLOCK_2, END_UPPER_MARKER_2, array_pool->head2);
  }
}
