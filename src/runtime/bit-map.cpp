#include "lib.h"


void bit_map_init(BIT_MAP *bit_map) {
  bit_map->words = NULL;
  bit_map->num_words = 0;
}

void bit_map_clear(BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  uint32 num_words = bit_map->num_words;
  if (num_words != 0) {
    release_state_mem_uint64_array(mem_pool, bit_map->words, num_words);
    bit_map->words = NULL;
    bit_map->num_words = 0;
  }
}

bool bit_map_set(BIT_MAP *bit_map, uint32 index, STATE_MEM_POOL *mem_pool) {
  uint32 word_idx = index / 64;
  uint32 num_words = bit_map->num_words;
  if (word_idx >= num_words) {
    uint32 new_num_words = pow_2_ceiling(word_idx + 1);
    if (num_words == 0) {
      new_num_words = max_u32(32, new_num_words);
      bit_map->words = alloc_state_mem_zeroed_uint64_array(mem_pool, new_num_words);
    }
    else
      bit_map->words = extend_state_mem_zeroed_uint64_array(mem_pool, bit_map->words, num_words, new_num_words);
    bit_map->num_words = new_num_words;
  }
}

bool bit_map_clear(BIT_MAP *bit_map, uint32 index) {
  uint32 word_idx = index / 64;
  uint32 num_words = bit_map->num_words;
  if (word_idx < num_words) {
    uint64 *word_ptr = bit_map->words + word_idx;
    uint64 word = *word_ptr;
    uint32 bit_idx = index % 64;
    uint64 mask = 1ULL << bit_idx;
    if (word & mask != 0) {
      word = word & ~mask;
      *word_ptr = word;
      return true;
    }
  }
  return false;
}

bool bit_map_is_set(BIT_MAP *bit_map, uint32 index) {
  uint32 word_idx = index / 64;
  uint32 num_words = bit_map->num_words;
  if (word_idx >= num_words)
    return false;
  uint64 word = bit_map->words[word_idx];
  uint32 bit_idx = index % 64;
  uint64 mask = 1ULL << bit_idx;
  return word & mask != 0;
}
