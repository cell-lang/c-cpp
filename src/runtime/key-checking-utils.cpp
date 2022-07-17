#include "lib.h"


void col_update_bit_map_init(COL_UPDATE_BIT_MAP *bit_map) {
  bit_map->bits = NULL;
  bit_map->more_dirty = NULL;
  bit_map->num_bits_words = 0;
  bit_map->num_dirty = 0;
}

void col_update_bit_map_clear(COL_UPDATE_BIT_MAP *bit_map) {
  const uint32 inline_capacity = lengthof(bit_map->inline_dirty);

  uint32 num_dirty = bit_map->num_dirty;
  assert((num_dirty > inline_capacity) == (bit_map->more_dirty != NULL));

  //## IF THE NUMBER OF DIRTY WORDS BECOMES TOO LARGE, JUST STOP RECORDING THEM AND DO A memset()
  //## BUT WHAT IF THE ARRAY IS RESIZED IN THE MEANTIME?

  if (num_dirty != 0) {
    uint64 *bits = bit_map->bits;

    uint32 count = min_u32(num_dirty, inline_capacity);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 dirty_word_idx = bit_map->inline_dirty[i];
      bits[dirty_word_idx] = 0;
    }

    if (num_dirty > inline_capacity) {
      count = num_dirty - inline_capacity;
      uint32 *more_dirty = bit_map->more_dirty;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 dirty_word_idx = more_dirty[i];
        bits[dirty_word_idx] = 0;
      }

      bit_map->more_dirty = NULL;
    }

    bit_map->num_dirty = 0;
  }
}

bool col_update_bit_map_check_and_set(COL_UPDATE_BIT_MAP *bit_map, uint32 index, STATE_MEM_POOL *mem_pool) {
  uint32 num_bits_words = bit_map->num_bits_words;
  if (index >= 64 * num_bits_words) {
    uint32 new_num_bits_words = pow_2_ceiling(index + 1);
    if (num_bits_words == 0) {
      num_bits_words = max_u32(32, new_num_bits_words);
      bit_map->num_bits_words = num_bits_words;
      bit_map->bits = alloc_state_mem_zeroed_uint64_array(mem_pool, num_bits_words);
    }
    else {
      bit_map->num_bits_words = new_num_bits_words;
      bit_map->bits = extend_state_mem_zeroed_uint64_array(mem_pool, bit_map->bits, num_bits_words, new_num_bits_words);
      num_bits_words = new_num_bits_words;
    }
  }

  uint32 word_idx = index / 64;
  uint32 bit_idx = index % 64;
  uint64 mask = 1ULL << bit_idx;

  uint64 *word_ptr = bit_map->bits + word_idx;
  uint64 word = *word_ptr;
  if ((word & mask) != 0)
    return true;
  *word_ptr = word | mask;

  if (word == 0) {
    const uint32 inline_capacity = lengthof(bit_map->inline_dirty);

    uint32 num_dirty = bit_map->num_dirty;
    if (num_dirty < inline_capacity) {
      bit_map->inline_dirty[num_dirty] = word_idx;
    }
    else if (num_dirty == inline_capacity) {
      assert(bit_map->more_dirty == NULL);
      uint32 *more_dirty = new_uint32_array(inline_capacity);
      more_dirty[0] = word_idx;
      bit_map->more_dirty = more_dirty;
    }
    else {
      uint32 num_more_dirty = num_dirty - inline_capacity;
      uint32 *more_dirty = bit_map->more_dirty;

      if (num_more_dirty >= inline_capacity & is_pow_2(num_more_dirty)) {
        uint32 *new_more_dirty = new_uint32_array(2 * num_more_dirty);
        memcpy(new_more_dirty, more_dirty, num_more_dirty * sizeof(uint32));
        more_dirty = new_more_dirty;
        bit_map->more_dirty = more_dirty;
      }

      more_dirty[num_more_dirty] = word_idx;
    }

    bit_map->num_dirty = num_dirty + 1;
  }

  return false;
}

void col_update_bit_map_set(COL_UPDATE_BIT_MAP *bit_map, uint32 index, STATE_MEM_POOL *mem_pool) {
  col_update_bit_map_check_and_set(bit_map, index, mem_pool);
}

bool col_update_bit_map_is_set(COL_UPDATE_BIT_MAP *bit_map, uint32 index) {
  uint32 num_bits_words = bit_map->num_bits_words;
  if (index >= 64 * num_bits_words)
    return false;
  uint32 word_idx = index / 64;
  uint32 bit_idx = index % 64;
  uint64 word = bit_map->bits[word_idx];
  return ((word >> bit_idx) & 1) != 0;
}

bool col_update_bit_map_is_dirty(COL_UPDATE_BIT_MAP *bit_map) {
  return bit_map->num_dirty > 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// 00 - untouched
// 01 - deleted
// 10 - inserted
// 11 - updated or deleted and reinserted

void col_update_status_map_init(COL_UPDATE_STATUS_MAP *status_map) {
  col_update_bit_map_init(&status_map->bit_map);
}

void col_update_status_map_clear(COL_UPDATE_STATUS_MAP *status_map) {
  col_update_bit_map_clear(&status_map->bit_map);
}

bool col_update_status_map_is_dirty(COL_UPDATE_STATUS_MAP *status_map) {
  return status_map->bit_map.num_dirty > 0;
}

//## REMOVE THIS ONE, IT'S REDUNDANT
void col_update_status_map_mark_deletion(COL_UPDATE_STATUS_MAP *status_map, uint32 index, STATE_MEM_POOL *mem_pool) {
  col_update_bit_map_set(&status_map->bit_map, 2 * index, mem_pool);
}

bool col_update_status_map_check_and_mark_deletion(COL_UPDATE_STATUS_MAP *status_map, uint32 index, STATE_MEM_POOL *mem_pool) {
  return col_update_bit_map_check_and_set(&status_map->bit_map, 2 * index, mem_pool);
}

bool col_update_status_map_check_and_mark_insertion(COL_UPDATE_STATUS_MAP *status_map, uint32 index, STATE_MEM_POOL *mem_pool) {
  return col_update_bit_map_check_and_set(&status_map->bit_map, 2 * index + 1, mem_pool);
}

bool col_update_status_map_deleted_flag_is_set(COL_UPDATE_STATUS_MAP *status_map, uint32 index) {
  return col_update_bit_map_is_set(&status_map->bit_map, 2 * index);
}

bool col_update_status_map_inserted_flag_is_set(COL_UPDATE_STATUS_MAP *status_map, uint32 index) {
  return col_update_bit_map_is_set(&status_map->bit_map, 2 * index + 1);
}
