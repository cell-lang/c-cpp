const uint32 INLINE_SLOT    = 0;
const uint32 SIZE_2_BLOCK   = 1;
const uint32 SIZE_4_BLOCK   = 2;
const uint32 SIZE_8_BLOCK   = 3;
const uint32 SIZE_16_BLOCK  = 4;
const uint32 HASHED_BLOCK   = 5;

const uint32 EMPTY_MARKER   = 0xFFFFFFFF;
const uint32 PAYLOAD_MASK   = 0x1FFFFFFF;

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

inline uint32 get_count(uint64 slot) {
  assert(get_tag(get_low_32(slot)) >= SIZE_2_BLOCK & get_tag(get_low_32(slot)) <= HASHED_BLOCK);
  // assert(get_high_32(slot) > 2); // Not true when initializing a hashed block
  return get_high_32(slot);
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

////////////////////////////////////////////////////////////////////////////

//## WARNING: THIS ONLY WORKS FOR LITTLE-ENDIAM ARCHITECTURES
//## TRY THE AGNOSTIC VERSION TO SEE IF IT'S ANY SLOWER
inline void set_high_32(uint64 *ptr, uint32 data) {
#ifndef NDEBUG
  uint64 content = *ptr;
#endif
  uint32 *high_32_ptr = ((uint32 *) ptr) + 1;
  *high_32_ptr = data;
#ifndef NDEBUG
  uint64 updated_content = *ptr;
#endif
  assert(get_low_32(updated_content) == get_low_32(content));
  assert(get_high_32(updated_content) == data);
}

////////////////////////////////////////////////////////////////////////////////

uint64 set_low_32(uint64 slot, uint32 low32);
uint64 set_high_32(uint64 slot, uint32 high32);

void array_mem_pool_init(ARRAY_MEM_POOL *, bool alloc_parallel_space, STATE_MEM_POOL *);
void array_mem_pool_release(ARRAY_MEM_POOL *array, STATE_MEM_POOL *mem_pool);
void array_mem_pool_clear(ARRAY_MEM_POOL *array, STATE_MEM_POOL *mem_pool);

uint32 array_mem_pool_alloc_16_block(ARRAY_MEM_POOL *, STATE_MEM_POOL *);
void array_mem_pool_release_16_block(ARRAY_MEM_POOL *, uint32 block_idx);
void array_mem_pool_release_16_block_upper_half(ARRAY_MEM_POOL *, uint32 block_idx);

uint32 array_mem_pool_alloc_8_block(ARRAY_MEM_POOL *, STATE_MEM_POOL *);
void array_mem_pool_release_8_block(ARRAY_MEM_POOL *, uint32 block_idx);
void array_mem_pool_release_8_block_upper_half(ARRAY_MEM_POOL *, uint32 block_idx);

uint32 array_mem_pool_alloc_4_block(ARRAY_MEM_POOL *, STATE_MEM_POOL *);
void array_mem_pool_release_4_block(ARRAY_MEM_POOL *, uint32 block_idx);

uint32 array_mem_pool_alloc_2_block(ARRAY_MEM_POOL *, STATE_MEM_POOL *);
void array_mem_pool_release_2_block(ARRAY_MEM_POOL *, uint32 block_idx);

////////////////////////////////////////////////////////////////////////////////

uint64 overflow_table_insert(ARRAY_MEM_POOL *, uint64 handle, uint32 value, STATE_MEM_POOL *);
uint64 overflow_table_insert_unique(ARRAY_MEM_POOL *, uint64 handle, uint32 value, STATE_MEM_POOL *);
uint64 overflow_table_delete(ARRAY_MEM_POOL *, uint64 handle, uint32 value);
void overflow_table_delete(ARRAY_MEM_POOL *, uint64 handle);
bool overflow_table_contains(ARRAY_MEM_POOL *, uint64 handle, uint32 value);
void overflow_table_copy(ARRAY_MEM_POOL *, uint64 handle, uint32 *dest, uint32 offset);
UINT32_ARRAY overflow_table_range_copy(ARRAY_MEM_POOL *, uint64 handle, uint32 first, uint32 *dest, uint32 capacity);
void overflow_table_insert_reversed(ARRAY_MEM_POOL *, uint32 key, uint64 handle, ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);

////////////////////////////////////////////////////////////////////////////////

void one_way_bin_table_init(ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);
void one_way_bin_table_release(ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);
void one_way_bin_table_clear(ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);

bool one_way_bin_table_contains(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2);
bool one_way_bin_table_contains_key(ONE_WAY_BIN_TABLE *, uint32 surr1);
uint32 one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *, uint32 surr, uint32 *dest);
UINT32_ARRAY one_way_bin_table_range_restrict(ONE_WAY_BIN_TABLE *, uint32 arg1, uint32 first, uint32 *args2, uint32 capacity);
uint32 one_way_bin_table_lookup(ONE_WAY_BIN_TABLE *, uint32 surr);
uint32 one_way_bin_table_get_count(ONE_WAY_BIN_TABLE *, uint32 surr);

bool one_way_bin_table_insert(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2, STATE_MEM_POOL *);
void one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2, STATE_MEM_POOL *);

uint32 one_way_bin_table_update(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2, STATE_MEM_POOL *);

bool one_way_bin_table_delete(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2);
void one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *, uint32 surr1);
void one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 *surrs2);

bool one_way_bin_table_is_map(ONE_WAY_BIN_TABLE *);
void one_way_bin_table_copy(ONE_WAY_BIN_TABLE *, uint32 *dest);

void one_way_bin_table_build_reverse(ONE_WAY_BIN_TABLE *, ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);

////////////////////////////////////////////////////////////////////////////////

uint32 loaded_overflow_table_lookup(ARRAY_MEM_POOL *, uint64 handle, uint32 value);
void loaded_overflow_table_copy(ARRAY_MEM_POOL *, uint64 handle, uint32 *values, uint32 *data, uint32 offset);

uint64 loaded_overflow_table_create_new_block(ARRAY_MEM_POOL *, uint64 values, uint64 datas, uint32 new_value, uint32 new_data, STATE_MEM_POOL *);
uint64 loaded_overflow_table_insert_unique(ARRAY_MEM_POOL *, uint64 handle, uint32 value, uint32 data, STATE_MEM_POOL *);

uint64 loaded_overflow_table_delete(ARRAY_MEM_POOL *, uint64 handle, uint32 value, uint64 *target_size_2_data_slot_ptr);
void loaded_overflow_table_delete(ARRAY_MEM_POOL *, uint64 handle);

void loaded_overflow_table_copy(ARRAY_MEM_POOL *, uint64 handle, uint32 *values, uint32 *data, uint32 offset);
UINT32_ARRAY loaded_overflow_table_range_copy(ARRAY_MEM_POOL *, uint64 handle, uint32 first, uint32 *output, uint32 capacity);

////////////////////////////////////////////////////////////////////////////////

void loaded_one_way_bin_table_init(ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);
void loaded_one_way_bin_table_release(ONE_WAY_BIN_TABLE *, STATE_MEM_POOL *);
void loaded_one_way_bin_table_clear(ONE_WAY_BIN_TABLE *, STATE_MEM_POOL*);

uint32 loaded_one_way_bin_table_payload(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2);
uint32 loaded_one_way_bin_table_restrict(ONE_WAY_BIN_TABLE *, uint32 surr, uint32 *dest, uint32 *data);
UINT32_ARRAY loaded_one_way_bin_table_range_restrict(ONE_WAY_BIN_TABLE *, uint32 key, uint32 first, uint32 *output, uint32 capacity);

void loaded_one_way_bin_table_insert_unique(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2, uint32 data, STATE_MEM_POOL *);

uint32 loaded_one_way_bin_table_delete(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 surr2);
void loaded_one_way_bin_table_delete_by_key(ONE_WAY_BIN_TABLE *, uint32 surr1, uint32 *surrs2, uint32 *data);
