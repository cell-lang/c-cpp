const uint32 EMPTY_MARKER   = 0xFFFFFFFF;
const uint32 PAYLOAD_MASK   = 0x1FFFFFFF;
const uint32 INLINE_SLOT    = 0;

const uint64 EMPTY_SLOT     = 0xFFFFFFFFULL;

////////////////////////////////////////////////////////////////////////////////

inline uint32 get_tag(uint32 word) {
  return word >> 29;
}

inline uint32 get_payload(uint32 word) {
  return word & PAYLOAD_MASK;
}

inline uint32 pack_tag_payload(uint32 tag, uint32 payload) {
  assert(get_tag(payload) == 0);
  return (tag << 29) | payload;
}

////////////////////////////////////////////////////////////////////////////////

uint32 get_count(uint64 slot);

// static uint32 get_count(uint64 slot) {
//   assert(get_tag(get_low_32(slot)) >= SIZE_2_BLOCK & get_tag(get_low_32(slot)) <= HASHED_BLOCK);
//   // assert(get_high_32(slot) > 2); // Not true when initializing a hashed block
//   return get_high_32(slot);
// }

////////////////////////////////////////////////////////////////////////////////

inline uint64 set_low_32(uint64 slot, uint32 low32) {
  uint64 updated_slot = pack(low32, get_high_32(slot));
  assert(get_low_32(updated_slot) == low32);
  assert(get_high_32(updated_slot) == get_high_32(slot));
  return updated_slot;
}

inline uint64 set_high_32(uint64 slot, uint32 high32) {
  uint64 updated_slot = pack(get_low_32(slot), high32);
  assert(get_low_32(updated_slot) == get_low_32(slot));
  assert(get_high_32(updated_slot) == high32);
  return updated_slot;
}

////////////////////////////////////////////////////////////////////////////////

uint64 set_low_32(uint64 slot, uint32 low32);
uint64 set_high_32(uint64 slot, uint32 high32);

void array_mem_pool_init(ARRAY_MEM_POOL *array, STATE_MEM_POOL *mem_pool);

uint32 array_mem_pool_alloc_16_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool);
void array_mem_pool_release_16_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx);
void array_mem_pool_release_16_block_upper_half(ARRAY_MEM_POOL *array_pool, uint32 block_idx);

uint32 array_mem_pool_alloc_8_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool);
void array_mem_pool_release_8_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx);
void array_mem_pool_release_8_block_upper_half(ARRAY_MEM_POOL *array_pool, uint32 block_idx);

uint32 array_mem_pool_alloc_4_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool);
void array_mem_pool_release_4_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx);

uint32 array_mem_pool_alloc_2_block(ARRAY_MEM_POOL *array_pool, STATE_MEM_POOL *mem_pool);
void array_mem_pool_release_2_block(ARRAY_MEM_POOL *array_pool, uint32 block_idx);

////////////////////////////////////////////////////////////////////////////////

uint64 overflow_table_insert(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool);
uint64 overflow_table_insert_unique(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value, STATE_MEM_POOL *mem_pool);
uint64 overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value);
void overflow_table_delete(ARRAY_MEM_POOL *array_pool, uint64 handle);
bool overflow_table_contains(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 value);
void overflow_table_copy(ARRAY_MEM_POOL *array_pool, uint64 handle, uint32 *dest, uint32 offset);

////////////////////////////////////////////////////////////////////////////////

bool one_way_bin_table_contains(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2);
bool one_way_bin_table_contains_key(ONE_WAY_BIN_TABLE *table, uint32 surr1);
uint32 one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *table, uint32 surr, uint32 *dest);
uint32 one_way_bin_table_lookup(ONE_WAY_BIN_TABLE *table, uint32 surr);
uint32 one_way_bin_table_get_count(ONE_WAY_BIN_TABLE *table, uint32 surr);

bool one_way_bin_table_insert(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool);
void one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool);

uint32 one_way_bin_table_update(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2, STATE_MEM_POOL *mem_pool);

bool one_way_bin_table_delete(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 surr2);
void one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *table, uint32 surr1, uint32 *surrs2);

bool one_way_bin_table_is_map(ONE_WAY_BIN_TABLE *table);
void one_way_bin_table_copy(ONE_WAY_BIN_TABLE *table, uint32 *dest);