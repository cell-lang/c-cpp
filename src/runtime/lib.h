#include "utils.h"


enum OBJ_TYPE {
  // Inline objects
  TYPE_NOT_A_VALUE_OBJ      = 0,
  TYPE_SYMBOL               = 1,
  TYPE_INTEGER              = 2,
  TYPE_FLOAT                = 3,
  TYPE_EMPTY_REL            = 4,
  TYPE_EMPTY_SEQ            = 5,
  TYPE_NE_SEQ_UINT8_INLINE  = 6,
  TYPE_NE_SEQ_INT16_INLINE  = 7,
  TYPE_NE_SEQ_INT32_INLINE  = 8,

  // Always references
  TYPE_NE_INT_SEQ           =  9,
  TYPE_NE_FLOAT_SEQ         = 10,
  TYPE_NE_BOOL_SEQ          = 11,
  TYPE_NE_SEQ               = 12,
  TYPE_NE_SET               = 13,
  TYPE_NE_MAP               = 14,
  TYPE_NE_BIN_REL           = 15,
  TYPE_NE_TERN_REL          = 16,
  TYPE_AD_HOC_TAG_REC       = 17,
  TYPE_BOXED_OBJ            = 18
};

enum INT_BITS_TAG {
  INT_BITS_TAG_8  = 0,
  INT_BITS_TAG_16 = 1,
  INT_BITS_TAG_32 = 2,
  INT_BITS_TAG_64 = 3
};

const OBJ_TYPE MAX_INLINE_OBJ_TYPE          = TYPE_NE_SEQ_INT32_INLINE;

const OBJ_TYPE NE_SEQ_TYPE_RANGE_START      = TYPE_NE_SEQ_UINT8_INLINE;
const OBJ_TYPE NE_SEQ_TYPE_RANGE_END        = TYPE_NE_SEQ;

const OBJ_TYPE NE_INT_SEQ_TYPE_RANGE_START  = TYPE_NE_SEQ_UINT8_INLINE;
const OBJ_TYPE NE_INT_SEQ_TYPE_RANGE_END    = TYPE_NE_INT_SEQ;

////////////////////////////////////////////////////////////////////////////////

// ---------------- ---------------- ---------------- ----------------
//                                                    ----------------  symbol id          (16)
//                                   ---------------- ----------------  size               (32)
//                                                    ----------------  ad hoc repr id     (16)
//                                   ----------------                   inner tag id       (16)
//                  ----------------                                    tag id             (16)
//             ----                                                     unused?             (4)
//           --                                                         no of tags          (2)
//      -----                                                           obj type            (5)
// -----                                                                physical repr info  (5)


struct OBJ {
  union {
    int64   int_;
    double  float_;
    void   *ptr;
  } core_data;

  uint64 extra_data;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_OBJ {
  uint32 capacity;
  uint32 used;
  union {
    OBJ    obj[1];
    double float_[1];
    uint8  uint8_[1];
    int8   int8_[1];
    int16  int16_[1];
    int32  int32_[1];
    int64  int64_[1];
  } buffer;
};

struct SET_OBJ {
  OBJ buffer[1];
};

struct BIN_REL_OBJ {
  OBJ buffer[1];
};

struct TERN_REL_OBJ {
  OBJ buffer[1];
};

struct BOXED_OBJ {
  OBJ obj;
};

struct TREE_SET_NODE;

struct FAT_SET_PTR {
  union {
    OBJ *array;
    TREE_SET_NODE *tree;
  } ptr;
  uint32 size;
  bool is_array_or_empty;
};

struct TREE_SET_NODE {
  OBJ value;
  FAT_SET_PTR left;
  FAT_SET_PTR right;
  uint32 priority;
};

struct MIXED_REPR_SET_OBJ {
  SET_OBJ *array_repr;
  TREE_SET_NODE *tree_repr;
};

struct TREE_MAP_NODE;

struct FAT_MAP_PTR {
  union {
    OBJ *array;
    TREE_MAP_NODE *tree;
  } ptr;
  uint32 size;
  uint32 offset; // Zero if ptr points to a TREE_MAP_NODE
};

struct TREE_MAP_NODE {
  OBJ key;
  OBJ value;
  FAT_MAP_PTR left;
  FAT_MAP_PTR right;
  uint32 priority;
};

struct MIXED_REPR_MAP_OBJ {
  BIN_REL_OBJ *array_repr;
  TREE_MAP_NODE *tree_repr;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_ITER {
  OBJ    seq;
  uint32 idx;
  uint32 len;
};

struct SET_ITER {
  OBJ    *buffer;
  uint32  idx;
  uint32  size;
};

struct BIN_REL_ITER {
  union {
    struct {
      OBJ    *left_col;
      OBJ    *right_col;
      uint32 *rev_idxs; // If this is not null, we are iterating in right column order
    } bin_rel;

    struct {
      uint16 *fields;
      void *ptr;
      uint16 repr_id;
    } opt_rec;
  } iter;

  uint32  idx;
  uint32  end; // Non-inclusive upper bound

  enum {BRIT_BIN_REL, BRIT_OPT_REC} type;
};

struct TERN_REL_ITER {
  OBJ    *col1;
  OBJ    *col2;
  OBJ    *col3;
  uint32 *ordered_idxs; // If this is null, we iterate directly over the columns
  uint32  idx;
  uint32  end; // Non-inclusive upper bound
};

struct STREAM {
  OBJ    *buffer;
  uint32  capacity;
  uint32  count;
  OBJ     internal_buffer[32];
};

////////////////////////////////////////////////////////////////////////////////

const uint32 QUEUE_INLINE_SIZE = 16;

struct QUEUE_U32 {
  uint32 capacity;
  uint32 count;
  uint32 *array;
  uint32 inline_array[QUEUE_INLINE_SIZE];
};

struct QUEUE_U64 {
  uint32 capacity;
  uint32 count;
  uint64 *array;
  uint64 inline_array[QUEUE_INLINE_SIZE];
};

struct QUEUE_3U32 {
  uint32 capacity;
  uint32 count;
  uint32 (*array)[3];
  uint32 inline_array[QUEUE_INLINE_SIZE][3];
};

struct QUEUE_U32_I64 {
  uint32 capacity;
  uint32 count;
  uint32 *u32_array;
  uint32 inline_u32_array[QUEUE_INLINE_SIZE];
  int64 *i64_array;
  int64 inline_i64_array[QUEUE_INLINE_SIZE];
};

struct QUEUE_U32_FLOAT {
  uint32 capacity;
  uint32 count;
  uint32 *u32_array;
  uint32 inline_u32_array[QUEUE_INLINE_SIZE];
  double *float_array;
  double inline_float_array[QUEUE_INLINE_SIZE];
};

struct QUEUE_U32_OBJ {
  uint32 capacity;
  uint32 count;
  uint32 *u32_array;
  uint32 inline_u32_array[QUEUE_INLINE_SIZE];
  OBJ *obj_array;
  OBJ inline_obj_array[QUEUE_INLINE_SIZE];
};

////////////////////////////////////////////////////////////////////////////////

struct SURR_SET {
  uint64 *bitmap;
  uint32 capacity;
  uint32 count;
};

////////////////////////////////////////////////////////////////////////////////

struct UNARY_TABLE {
  uint64 *bitmap;
  uint32 capacity; // Number of bits
  uint32 count;
};

struct UNARY_TABLE_ITER {
  uint64 *bitmap;
  uint32 index;
  uint32 left;
};

struct UNARY_TABLE_AUX {
  QUEUE_U32 deletions;
  QUEUE_U32 insertions;
  uint32 init_capacity; // Capacity before the update is executed (DO WE STILL NEED THIS?)
  bool clear;
#ifndef NDEBUG
  bool prepared;
#endif
};

////////////////////////////////////////////////////////////////////////////////

struct ARRAY_MEM_POOL {
  uint64 *slots;
  uint32 size;
  uint32 head2, head4, head8, head16;
  bool alloc_double_space;
};

struct ONE_WAY_BIN_TABLE {
  ARRAY_MEM_POOL array_pool;
  uint64 *column;
  uint32 capacity;
  uint32 count;
};

////////////////////////////////////////////////////////////////////////////////

struct COL_UPDATE_BIT_MAP {
  uint32 inline_dirty[32];
  uint64 *bits; // Stored in state memory
  uint32 *more_dirty;
  uint32 num_bits_words;
  uint32 num_dirty;
};

struct COL_UPDATE_STATUS_MAP {
  COL_UPDATE_BIT_MAP bit_map;
};

////////////////////////////////////////////////////////////////////////////////

struct BIN_TABLE {
  ONE_WAY_BIN_TABLE forward;
  ONE_WAY_BIN_TABLE backward;
};

const uint32 BIN_TABLE_ITER_INLINE_SIZE = 256; //## TOO MUCH?

struct BIN_TABLE_ITER {
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  BIN_TABLE *table;
  uint32 *arg2s;
  uint32 capacity;
  uint32 idx_last;
  uint32 arg1;
  uint32 left;
};

struct BIN_TABLE_ITER_1 {
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  uint32 *args;
  uint32 left;
};

struct BIN_TABLE_ITER_2 {
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  uint32 *args;
  uint32 left;
};

struct BIN_TABLE_AUX {
  COL_UPDATE_BIT_MAP bit_map;
  QUEUE_U64 deletions;
  QUEUE_U32 deletions_1;
  QUEUE_U32 deletions_2;
  QUEUE_U64 insertions;
  bool clear;
};

struct SYM_BIN_TABLE_AUX {
  QUEUE_U64 deletions;
  QUEUE_U32 deletions_1;
  QUEUE_U64 insertions;
  bool clear;
};

////////////////////////////////////////////////////////////////////////////////

struct MASTER_BIN_TABLE {
  BIN_TABLE table;
  uint64 *slots;
  uint32 capacity;
  uint32 first_free;
};

struct MASTER_BIN_TABLE_ITER {
  uint64 *slots;
  uint32 index;
  uint32 left;
};

struct MASTER_BIN_TABLE_ITER_1 {
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  uint32 *arg2s;
  uint32 *surrs;
  uint32 left;
};

struct MASTER_BIN_TABLE_ITER_2 {
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  ONE_WAY_BIN_TABLE *forward;
  uint32 *arg1s;
  uint32 arg2;
  uint32 left;

};

struct MASTER_BIN_TABLE_AUX {
  QUEUE_U64 deletions;
  QUEUE_U32 deletions_1;
  QUEUE_U32 deletions_2;
  QUEUE_U32 insertions;
  QUEUE_U32 reinsertions;
  uint32 last_surr;
  bool clear;
};

////////////////////////////////////////////////////////////////////////////////

struct SYM_MASTER_BIN_TABLE_ITER_1 {
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  ONE_WAY_BIN_TABLE *forward;
  uint32 *other_args;
  uint32 arg;
  uint32 left;
};

struct SYM_MASTER_BIN_TABLE_AUX {
  QUEUE_U64 deletions;
  QUEUE_U32 deletions_1;
  QUEUE_U32 insertions;
  QUEUE_U32 reinsertions;
  uint32 last_surr;
  bool clear;
};

////////////////////////////////////////////////////////////////////////////////

struct SLAVE_TERN_TABLE_ITER {
  MASTER_BIN_TABLE_ITER master_iter;
  BIN_TABLE_ITER_1 slave_iter;
  BIN_TABLE *slave_table;
};

struct SLAVE_TERN_TABLE_ITER_1 {
  MASTER_BIN_TABLE_ITER_1 master_iter;
  BIN_TABLE_ITER_1 slave_iter;
  BIN_TABLE *slave_table;
};

struct SLAVE_TERN_TABLE_ITER_2 {
  MASTER_BIN_TABLE_ITER_2 master_iter;
  BIN_TABLE_ITER_1 slave_iter;
  BIN_TABLE *slave_table;
};

struct SLAVE_TERN_TABLE_ITER_12 {
  BIN_TABLE_ITER_1 slave_iter;
};

struct SLAVE_TERN_TABLE_ITER_3 {
  BIN_TABLE_ITER_2 slave_iter;
  MASTER_BIN_TABLE *master_table;
};

struct SLAVE_TERN_TABLE_ITER_13 {
  BIN_TABLE_ITER_2 slave_iter;
  MASTER_BIN_TABLE *master_table;
  uint32 arg1;
};

struct SLAVE_TERN_TABLE_ITER_23 {
  BIN_TABLE_ITER_2 slave_iter;
  MASTER_BIN_TABLE *master_table;
  uint32 arg2;
};

struct SLAVE_TERN_TABLE_AUX {
  BIN_TABLE_AUX slave_table_aux;
};

////////////////////////////////////////////////////////////////////////////////

struct SEMISYM_SLAVE_TERN_TABLE_ITER_1 {
  SYM_MASTER_BIN_TABLE_ITER_1 master_iter;
  BIN_TABLE_ITER_1 slave_iter;
  BIN_TABLE *slave_table;
};

////////////////////////////////////////////////////////////////////////////////

struct TERN_TABLE {
  MASTER_BIN_TABLE master;
  BIN_TABLE slave;
};

struct TERN_TABLE_AUX {
  MASTER_BIN_TABLE_AUX master;
  BIN_TABLE_AUX slave;
  QUEUE_U32 surr12_follow_ups;
  QUEUE_U32 insertions; //## THIS SHOULD BE USED ONLY WHEN THERE'S A 1-3 OR 2-3 KEY (WHAT ABOUT FOREIGN KEYS?)
};

////////////////////////////////////////////////////////////////////////////////

struct SEMISYM_TERN_TABLE_AUX {
  SYM_MASTER_BIN_TABLE_AUX master;
  BIN_TABLE_AUX slave;
  QUEUE_U32 surr12_follow_ups;
};

////////////////////////////////////////////////////////////////////////////////

struct OBJ_COL {
  OBJ *array;
  uint32 capacity;
  uint32 count;
};

struct OBJ_COL_ITER {
  OBJ *array;
  uint32 left; // Includes current value
  uint32 idx;
};

struct OBJ_COL_AUX {
  COL_UPDATE_STATUS_MAP status_map;
  QUEUE_U32 deletions;
  QUEUE_U32_OBJ insertions;
  QUEUE_U32_OBJ updates;
  bool clear;
};

////////////////////////////////////////////////////////////////////////////////

struct INT_COL {
  int64 *array;
  uint32 capacity;
  uint32 count;

  unordered_set<uint32> collisions;
};

struct INT_COL_ITER {
  int64 *array;
  uint32 left; // Includes current value
  uint32 idx;

  unordered_set<uint32> *collisions;
};

struct INT_COL_AUX {
  COL_UPDATE_STATUS_MAP status_map;
  QUEUE_U32 deletions;
  QUEUE_U32_I64 insertions;
  QUEUE_U32_I64 updates;
  bool clear;
};

////////////////////////////////////////////////////////////////////////////////

struct FLOAT_COL {
  double *array;
  uint32 capacity;
  uint32 count;
};

struct FLOAT_COL_ITER {
  double *array;
  uint32 left; // Includes current value
  uint32 idx;
};

struct FLOAT_COL_AUX {
  COL_UPDATE_STATUS_MAP status_map;
  QUEUE_U32 deletions;
  QUEUE_U32_FLOAT insertions;
  QUEUE_U32_FLOAT updates;
  bool clear;
};

////////////////////////////////////////////////////////////////////////////////

struct RAW_OBJ_COL {
  OBJ *array;
#ifndef NDEBUG
  uint32 capacity;
  uint32 count;
#endif
};

struct RAW_OBJ_COL_ITER {
  UNARY_TABLE_ITER iter;
  OBJ *array;
};

////////////////////////////////////////////////////////////////////////////////

struct RAW_INT_COL {
  int64 *array;
#ifndef NDEBUG
  uint32 capacity;
#endif
};

struct RAW_INT_COL_ITER {
  UNARY_TABLE_ITER iter;
  int64 *array;
};

////////////////////////////////////////////////////////////////////////////////

struct RAW_FLOAT_COL {
  double *array;
#ifndef NDEBUG
  uint32 capacity;
#endif
};

struct RAW_FLOAT_COL_ITER {
  UNARY_TABLE_ITER iter;
  double *array;
};

////////////////////////////////////////////////////////////////////////////////

struct OBJ_STORE {                // VALUE     NO VALUE
  OBJ    *values;                 //           blank
  uint32 *hashcode_or_next_free;  // hashcode  index of the next free slot (can be out of bounds)

  uint8  *refs_counters;
  unordered_map<uint32, uint32> extra_refs;

  uint32 *hashtable;  // 0xFFFFFFFF when there's no value in that bucket
  uint32 *buckets;    // Junk when there's no (next?) value

  uint32 capacity;
  uint32 count;
  uint32 first_free;
};


const uint32 INLINE_AUX_SIZE = 16;

struct OBJ_STORE_AUX_BATCH_RELEASE_ENTRY {
  uint32 surr;
  uint32 count;
};

struct OBJ_STORE_AUX_INSERT_ENTRY {
  OBJ    obj;
  uint32 hashcode;
  uint32 surr;
};

struct OBJ_STORE_AUX {
  uint32 deferred_capacity;
  uint32 deferred_count;
  uint32 *deferred_release_surrs;
  uint32 deferred_release_buffer[INLINE_AUX_SIZE];

  uint32 batch_deferred_capacity;
  uint32 batch_deferred_count;
  OBJ_STORE_AUX_BATCH_RELEASE_ENTRY *batch_deferred_release_entries;
  OBJ_STORE_AUX_BATCH_RELEASE_ENTRY batch_deferred_release_buffer[INLINE_AUX_SIZE];

  uint32 capacity;
  uint32 count;

  OBJ_STORE_AUX_INSERT_ENTRY *entries;
  OBJ_STORE_AUX_INSERT_ENTRY entries_buffer[INLINE_AUX_SIZE];

  uint32 *hashtable;
  uint32 *buckets;

  uint32 hash_range;
  uint32 last_surr;
};

////////////////////////////////////////////////////////////////////////////////

struct INT_STORE {
  // Bits  0 - 31: 32-bit value, or index of 64-bit value
  // Bits 32 - 61: index of next value in the bucket if used or next free index otherwise
  // Bits 62 - 63: tag: 00 used (32 bit), 01 used (64 bit), 10 free
  uint64 *slots;

  unordered_map<uint32, int64> large_ints;

  // INV_IDX when there's no value in that bucket
  uint32 *hashtable;

  uint8  *refs_counters;
  unordered_map<uint32, uint32> extra_refs;

  uint32 capacity;
  uint32 count;
  uint32 first_free;
};


// const uint32 INLINE_AUX_SIZE = 16;

struct INT_STORE_AUX_BATCH_RELEASE_ENTRY {
  uint32 surr;
  uint32 count;
};

struct INT_STORE_AUX_INSERT_ENTRY {
  int64  value;
  uint32 hashcode;
  uint32 surr;
};

struct INT_STORE_AUX {
  uint32 deferred_capacity;
  uint32 deferred_count;
  uint32 *deferred_release_surrs;
  uint32 deferred_release_buffer[INLINE_AUX_SIZE];

  uint32 batch_deferred_capacity;
  uint32 batch_deferred_count;
  INT_STORE_AUX_BATCH_RELEASE_ENTRY *batch_deferred_release_entries;
  INT_STORE_AUX_BATCH_RELEASE_ENTRY batch_deferred_release_buffer[INLINE_AUX_SIZE];

  uint32 capacity;
  uint32 count;

  INT_STORE_AUX_INSERT_ENTRY *entries;
  INT_STORE_AUX_INSERT_ENTRY entries_buffer[INLINE_AUX_SIZE];

  uint32 *hashtable;
  uint32 *buckets;

  uint32 hash_range;
  uint32 last_surr;
};

////////////////////////////////////////////////////////////////////////////////

struct PARSER;

////////////////////////////////////////////////////////////////////////////////

const uint64 MAX_SEQ_LEN = 0xFFFFFFFF;

const uint32 INVALID_INDEX = 0xFFFFFFFFU;

const uint16 symb_id_false    = 0;
const uint16 symb_id_true     = 1;
const uint16 symb_id_void     = 2;
const uint16 symb_id_string   = 3;
const uint16 symb_id_date     = 4;
const uint16 symb_id_time     = 5;
const uint16 symb_id_nothing  = 6;
const uint16 symb_id_just     = 7;
const uint16 symb_id_success  = 8;
const uint16 symb_id_failure  = 9;

////////////////////////////// auto-mem-alloc.cpp //////////////////////////////

typedef struct {
  void *subpools[9];
} STATE_MEM_POOL;

void init_mem_pool(STATE_MEM_POOL *);
void release_mem_pool(STATE_MEM_POOL *);

void *alloc_state_mem_block(STATE_MEM_POOL *, uint32);
void release_state_mem_block(STATE_MEM_POOL *, void *, uint32);

OBJ copy_to_pool(STATE_MEM_POOL *, OBJ);
void remove_from_pool(STATE_MEM_POOL *, OBJ);

uint32 obj_mem_size(OBJ);
OBJ copy_obj_to(OBJ, void **);

void *grab_mem(void **, uint32);

inline uint32 round_up_8(uint32 mem_size) {
  return (mem_size + 7) & ~7;
}

inline uint32 null_round_up_8(uint32 mem_size) {
  assert(mem_size % 8 == 0);
  return mem_size;
}

OBJ *alloc_state_mem_obj_array(STATE_MEM_POOL *, uint32 size);
OBJ *alloc_state_mem_blanked_obj_array(STATE_MEM_POOL *, uint32 size);
OBJ *extend_state_mem_obj_array(STATE_MEM_POOL *, OBJ *ptr, uint32 size, uint32 new_size);
OBJ *extend_state_mem_blanked_obj_array(STATE_MEM_POOL *, OBJ *ptr, uint32 size, uint32 new_size);
void release_state_mem_obj_array(STATE_MEM_POOL *, OBJ *ptr, uint32 size);

double *alloc_state_mem_float_array(STATE_MEM_POOL *, uint32 size);
double *extend_state_mem_float_array(STATE_MEM_POOL *, double *ptr, uint32 size, uint32 new_size);
void release_state_mem_float_array(STATE_MEM_POOL *, double *ptr, uint32 size);

uint64 *alloc_state_mem_uint64_array(STATE_MEM_POOL *, uint32 size);
uint64 *alloc_state_mem_zeroed_uint64_array(STATE_MEM_POOL *, uint32 size);
uint64 *extend_state_mem_uint64_array(STATE_MEM_POOL *, uint64 *ptr, uint32 size, uint32 new_size);
uint64 *extend_state_mem_zeroed_uint64_array(STATE_MEM_POOL *, uint64 *ptr, uint32 size, uint32 new_size);
void release_state_mem_uint64_array(STATE_MEM_POOL *, uint64 *ptr, uint32 size);

int64 *alloc_state_mem_int64_array(STATE_MEM_POOL *, uint32 size);
int64 *extend_state_mem_int64_array(STATE_MEM_POOL *, int64 *ptr, uint32 size, uint32 new_size);
void release_state_mem_int64_array(STATE_MEM_POOL *, int64 *ptr, uint32 size);

uint32 *alloc_state_mem_uint32_array(STATE_MEM_POOL *, uint32 size);
uint32 *alloc_state_mem_oned_uint32_array(STATE_MEM_POOL *, uint32 size);
uint32 *extend_state_mem_uint32_array(STATE_MEM_POOL *, uint32 *ptr, uint32 size, uint32 new_size);
void release_state_mem_uint32_array(STATE_MEM_POOL *, uint32 *ptr, uint32 size);

uint8 *alloc_state_mem_uint8_array(STATE_MEM_POOL *, uint32 size);
uint8 *extend_state_mem_zeroed_uint8_array(STATE_MEM_POOL *, uint8 *ptr, uint32 size, uint32 new_size);
void release_state_mem_uint8_array(STATE_MEM_POOL *, uint8 *ptr, uint32 size);

///////////////////////////////// mem-core.cpp /////////////////////////////////

void *alloc_eternal_block(uint32 byte_size);

void *alloc_static_block(uint32 byte_size);
void *release_static_block(void *ptr, uint32 byte_size);

void* new_obj(uint32 byte_size);

/////////////////////////////////// mem.cpp ////////////////////////////////////

uint64 seq_obj_mem_size(uint64 capacity);
uint64 uint8_seq_obj_mem_size(uint64 capacity);
uint64 int8_seq_obj_mem_size(uint64 capacity);
uint64 int16_seq_obj_mem_size(uint64 capacity);
uint64 int32_seq_obj_mem_size(uint64 capacity);
uint64 int64_seq_obj_mem_size(uint64 capacity);
uint64 set_obj_mem_size(uint64 size);
uint64 bin_rel_obj_mem_size(uint64 size);
uint64 tern_rel_obj_mem_size(uint64 size);
uint64 map_obj_mem_size(uint64 size);
uint64 boxed_obj_mem_size();

OBJ* get_left_col_array_ptr(BIN_REL_OBJ*);
OBJ* get_right_col_array_ptr(BIN_REL_OBJ *rel, uint32 size);
uint32 *get_right_to_left_indexes(BIN_REL_OBJ *rel, uint32 size);

OBJ *get_col_array_ptr(TERN_REL_OBJ *rel, uint32 size, int idx);
uint32 *get_rotated_index(TERN_REL_OBJ *rel, uint32 size, int amount);

SET_OBJ             *new_set(uint32 size);
TREE_SET_NODE       *new_tree_set_node();
MIXED_REPR_SET_OBJ  *new_mixed_repr_set();
BIN_REL_OBJ         *new_map(uint32 size); // Clears rev_idxs
TREE_MAP_NODE       *new_tree_map_node();
MIXED_REPR_MAP_OBJ  *new_mixed_repr_map();
BIN_REL_OBJ         *new_bin_rel(uint32 size);
TREE_MAP_NODE       *new_bin_tree_map();
TERN_REL_OBJ        *new_tern_rel(uint32 size);
BOXED_OBJ           *new_boxed_obj();

// Set used and capacity fields
SEQ_OBJ *new_obj_seq(uint32 length);
SEQ_OBJ *new_obj_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_float_seq(uint32 length);
SEQ_OBJ *new_float_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_uint8_seq(uint32 length);
SEQ_OBJ *new_uint8_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int8_seq(uint32 length);
SEQ_OBJ *new_int8_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int16_seq(uint32 length);
SEQ_OBJ *new_int16_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int32_seq(uint32 length);
SEQ_OBJ *new_int32_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int64_seq(uint32 length);
SEQ_OBJ *new_int64_seq(uint32 length, uint32 capacity);

OBJ *new_obj_array(uint32 size);

bool *new_bool_array(uint32 size);
double *new_float_array(uint32 size);

int64  *new_int64_array(uint32 size);
uint64 *new_uint64_array(uint32 size);
int32  *new_int32_array(uint32 size);
uint32 *new_uint32_array(uint32 size);
int16  *new_int16_array(uint32 size);
uint16 *new_uint16_array(uint32 size);
int8   *new_int8_array(uint32 size);
uint8  *new_uint8_array(uint32 size);

char *new_byte_array(uint32 size);
void *new_void_array(uint32 size);

// Extra memory is not initialized
OBJ    *resize_obj_array(OBJ* array, uint32 size, uint32 new_size);
double *resize_float_array(double* array, uint32 size, uint32 new_size);
uint32 *resize_uint32_array(uint32 *array, uint32 size, uint32 new_size);
uint64 *resize_uint64_array(uint64 *array, uint32 size, uint32 new_size);
int64  *resize_int64_array(int64 *array, uint32 size, uint32 new_size);

//////////////////////////////// mem-utils.cpp /////////////////////////////////

bool is_blank(OBJ);
bool is_symb(OBJ);
bool is_bool(OBJ);
bool is_int(OBJ);
bool is_float(OBJ);
bool is_seq(OBJ);
bool is_empty_seq(OBJ);
bool is_ne_seq(OBJ);
bool is_empty_rel(OBJ);
bool is_set(OBJ);
bool is_ne_set(OBJ);
bool is_bin_rel(OBJ);
bool is_ne_bin_rel(OBJ);
bool is_ne_map(OBJ);
bool is_tern_rel(OBJ);
bool is_ne_tern_rel(OBJ);
bool is_tag_obj(OBJ);

bool is_symb(OBJ, uint16);
bool is_int(OBJ, int64);

uint16 get_symb_id(OBJ);
bool   get_bool(OBJ);
int64  get_int(OBJ);
double get_float(OBJ);
uint32 read_size_field(OBJ);
uint16 get_tag_id(OBJ);
OBJ    get_inner_obj(OBJ);

OBJ make_blank_obj();
OBJ make_empty_seq();
OBJ make_empty_rel();
OBJ make_symb(uint16 symb_id);
OBJ make_bool(bool b);
OBJ make_int(uint64 value);
OBJ make_float(double value);
OBJ make_set(SET_OBJ*, uint32 size);
OBJ make_bin_rel(BIN_REL_OBJ*, uint32 size);
OBJ make_tern_rel(TERN_REL_OBJ*, uint32 size);
OBJ make_map(BIN_REL_OBJ*, uint32 size);
OBJ make_tag_obj(uint16 tag_id, OBJ obj);
OBJ make_opt_tag_rec(void *ptr, uint16 repr_id);

OBJ make_seq(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice(OBJ *ptr, uint32 length);

OBJ make_seq_float(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_float(double *ptr, uint32 length);

OBJ make_seq_uint8(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_uint8(uint8 *ptr, uint32 length);
OBJ make_seq_uint8_inline(uint64 data, uint32 length);

OBJ make_seq_int8(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int8(int8 *ptr, uint32 length);

OBJ make_seq_int16(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int16(int16 *ptr, uint32 length);
OBJ make_seq_int16_inline(uint64 data, uint32 length);

OBJ make_seq_int32(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int32(int32 *ptr, uint32 length);
OBJ make_seq_int32_inline(uint64 data, uint32 length);

OBJ make_seq_int64(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int64(int64 *ptr, uint32 length);

// Purely physical representation functions

bool is_inline_obj(OBJ);

bool is_opt_rec(OBJ);

bool is_opt_rec_or_tag_rec(OBJ);

bool is_signed(OBJ);

bool is_array_obj(OBJ);

int intrl_cmp(OBJ, OBJ);

uint32 read_size_field_unchecked(OBJ);

INT_BITS_TAG get_int_bits_tag(OBJ);

OBJ* get_seq_elts_ptr(OBJ);
OBJ* get_set_elts_ptr(OBJ);

double *get_seq_elts_ptr_float(OBJ);

uint8 *get_seq_elts_ptr_uint8(OBJ obj);
int8  *get_seq_elts_ptr_int8(OBJ obj);
void  *get_seq_elts_ptr_int8_or_uint8(OBJ seq);
int16 *get_seq_elts_ptr_int16(OBJ obj);
int32 *get_seq_elts_ptr_int32(OBJ obj);
int64 *get_seq_elts_ptr_int64(OBJ obj);

OBJ_TYPE get_obj_type(OBJ);

SEQ_OBJ      *get_seq_ptr(OBJ);
SET_OBJ      *get_set_ptr(OBJ);
BIN_REL_OBJ  *get_bin_rel_ptr(OBJ);
TERN_REL_OBJ *get_tern_rel_ptr(OBJ);
BOXED_OBJ    *get_boxed_obj_ptr(OBJ);

void* get_opt_tag_rec_ptr(OBJ);

void* get_opt_repr_ptr(OBJ);
uint16 get_opt_repr_id(OBJ);

OBJ_TYPE get_ref_obj_type(OBJ);
void* get_ref_obj_ptr(OBJ);

bool are_shallow_eq(OBJ, OBJ);
int shallow_cmp(OBJ, OBJ);

int comp_floats(double, double);

OBJ repoint_to_copy(OBJ, void*);
OBJ repoint_to_sliced_copy(OBJ, void*);

//////////////////////////////// basic-ops.cpp /////////////////////////////////

bool inline_eq(OBJ obj1, OBJ obj2);
bool are_eq(OBJ obj1, OBJ obj2);
bool is_out_of_range(SET_ITER &it);
bool is_out_of_range(SEQ_ITER &it);
bool is_out_of_range(BIN_REL_ITER &it);
bool is_out_of_range(TERN_REL_ITER &it);

bool contains(OBJ set, OBJ elem);
bool contains_br(OBJ rel, OBJ arg1, OBJ arg2);
bool contains_br_1(OBJ rel, OBJ arg1);
bool contains_br_2(OBJ rel, OBJ arg2);
bool contains_tr(OBJ rel, OBJ arg1, OBJ arg2, OBJ arg3);
bool contains_tr_1(OBJ rel, OBJ arg1);
bool contains_tr_2(OBJ rel, OBJ arg2);
bool contains_tr_3(OBJ rel, OBJ arg3);
bool contains_tr_12(OBJ rel, OBJ arg1, OBJ arg2);
bool contains_tr_13(OBJ rel, OBJ arg1, OBJ arg3);
bool contains_tr_23(OBJ rel, OBJ arg2, OBJ arg3);

bool has_field(OBJ rec, uint16 field_symb_id);

uint32 get_size(OBJ);
int64 float_bits(OBJ);
int64 rand_nat(int64 max);  // Non-deterministic
int64 unique_nat();         // Non-deterministic

OBJ get_curr_obj(SET_ITER &it);
OBJ get_curr_obj(SEQ_ITER &it);
OBJ get_curr_left_arg(BIN_REL_ITER &it);
OBJ get_curr_right_arg(BIN_REL_ITER &it);
OBJ tern_rel_it_get_left_arg(TERN_REL_ITER &it);
OBJ tern_rel_it_get_mid_arg(TERN_REL_ITER &it);
OBJ tern_rel_it_get_right_arg(TERN_REL_ITER &it);
OBJ set_only_elt(OBJ set);

OBJ lookup(OBJ rel, OBJ key);
OBJ lookup_field(OBJ rec, uint16 field_symb_id);

////////////////////////////////// instrs.cpp //////////////////////////////////

void init(STREAM &s);
void append(STREAM &s, OBJ obj);
OBJ build_seq(OBJ* elems, uint32 length);
OBJ build_set(OBJ* elems, uint32 size);
OBJ build_set(STREAM &s);
OBJ int_to_float(OBJ val);
OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len);
// OBJ update_seq_at(OBJ seq, OBJ idx, OBJ value);
OBJ rev_seq(OBJ seq);
OBJ internal_sort(OBJ set);
OBJ parse_value(OBJ str);
OBJ print_value(OBJ);
void get_set_iter(SET_ITER &it, OBJ set);
void get_seq_iter(SEQ_ITER &it, OBJ seq);
void move_forward(SET_ITER &it);
void move_forward(SEQ_ITER &it);
void move_forward(BIN_REL_ITER &it);
void move_forward(TERN_REL_ITER &it);
void fail();
void runtime_check(OBJ cond);

OBJ build_const_seq_uint8(const uint8 *array, uint32 size);
// OBJ build_const_seq_uint16(const uint16 *array, uint32 size);
// OBJ build_const_seq_uint32(const uint32 *array, uint32 size);
OBJ build_const_seq_int8(const int8 *array, uint32 size);
OBJ build_const_seq_int16(const int16 *array, uint32 size);
OBJ build_const_seq_int32(const int32 *array, uint32 size);
OBJ build_const_seq_int64(const int64 *array, uint32 size);

OBJ build_inline_const_seq_uint8(uint64 coded_seq, uint32 size);
OBJ build_inline_const_seq_int16(uint64 coded_seq, uint32 size);
OBJ build_inline_const_seq_int32(uint64 coded_seq, uint32 size);

////////////////////////////////////////////////////////////////////////////////

uint8 inline_uint8_at(uint64 packed_elts, uint32 idx);
int16 inline_int16_at(uint64 packed_elts, uint32 idx);
int32 inline_int32_at(uint64 packed_elts, uint32 idx);

uint64 inline_uint8_init_at(uint64 packed_elts, uint32 idx, uint8 value);
uint64 inline_int16_init_at(uint64 packed_elts, uint32 idx, int16 value);
uint64 inline_int32_init_at(uint64 packed_elts, uint32 idx, int32 value);

uint64 inline_uint8_pack(uint8 *array, uint32 size);
uint64 inline_int16_pack(int16 *array, uint32 size);

uint64 inline_uint8_slice(uint64 packed_elts, uint32 idx_first, uint32 count);
uint64 inline_int16_slice(uint64 packed_elts, uint32 idx_first, uint32 count);

uint64 inline_uint8_concat(uint64 left, uint32 left_len, uint64 right, uint32 right_len);
uint64 inline_int16_concat(uint64 left, uint32 left_len, uint64 right, uint32 right_len);

//////////////////////////////// bin-rel-obj.cpp ///////////////////////////////

bool index_has_been_built(BIN_REL_OBJ *rel, uint32 size);
void build_map_right_to_left_sorted_idx_array(BIN_REL_OBJ *, uint32 size);

OBJ build_bin_rel(OBJ *col1, OBJ *col2, uint32 size);
OBJ build_bin_rel(STREAM &strm1, STREAM &strm2);

OBJ build_map(OBJ* keys, OBJ* values, uint32 size);
OBJ build_map(STREAM &key_stream, STREAM &value_stream);

void get_bin_rel_iter(BIN_REL_ITER &it, OBJ rel);
void get_bin_rel_iter_1(BIN_REL_ITER &it, OBJ rel, OBJ arg1);
void get_bin_rel_iter_2(BIN_REL_ITER &it, OBJ rel, OBJ arg2);

/////////////////////////////// tern-rel-obj.cpp ///////////////////////////////

OBJ build_tern_rel(OBJ *col1, OBJ *col2, OBJ *col3, uint32 size);
OBJ build_tern_rel(STREAM &strm1, STREAM &strm2, STREAM &strm3);

void get_tern_rel_iter(TERN_REL_ITER &it, OBJ rel);
void get_tern_rel_iter_by(TERN_REL_ITER &it, OBJ rel, int col_idx, OBJ arg);
void get_tern_rel_iter_by(TERN_REL_ITER &it, OBJ rel, int major_col_idx, OBJ major_arg, OBJ minor_arg);

/////////////////////////////////// debug.cpp //////////////////////////////////

int get_call_stack_depth();

void push_call_info(const char* fn_name, uint32 arity, OBJ* params);
void pop_call_info();
void pop_try_mode_call_info(int depth);
void print_call_stack();
void dump_var(const char* name, OBJ value);
void print_assertion_failed_msg(const char* file, uint32 line, const char* text);

void soft_fail(const char *msg);
void impl_fail(const char *msg);
// void physical_fail();
void internal_fail();

////////////////////////////////// sorting.cpp /////////////////////////////////

void sort_u32(uint32 *array, uint32 len);
void sort_u64(uint64 *array, uint32 len);
void sort_3u32(uint32 *array, uint32 len);

void sort_unique_u32(uint32 *, uint32 *);
void sort_unique_3u32(uint32 *, uint32 *);

void stable_index_sort(uint32 *indexes, uint32 count, OBJ *values);
void stable_index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *minor_sort);
void stable_index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort);

void index_sort(uint32 *indexes, uint32 count, OBJ *values);
void index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *minor_sort);
void index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort);

/////////////////////////////////// algs.cpp ///////////////////////////////////

bool sorted_u32_array_contains(uint32 *array, uint32 len, uint32 value);
bool sorted_u64_array_contains(uint64 *array, uint32 len, uint64 value);
bool sorted_3u32_array_contains(uint32 *array, uint32 len, uint32, uint32, uint32);

uint32 sort_unique(OBJ* objs, uint32 size);
uint32 sort_and_check_no_dups(OBJ* keys, OBJ* values, uint32 size);
void sort_obj_array(OBJ* objs, uint32 len);

uint64 encoded_index_or_insertion_point_in_unique_sorted_array(OBJ *sorted_array, uint32 len, OBJ obj);

uint32 find_obj(OBJ* sorted_array, uint32 len, OBJ obj, bool &found); //## WHAT SHOULD THIS RETURN? ANY VALUE IN THE [0, 2^32-1] IS A VALID SEQUENCE INDEX, SO WHAT COULD BE USED TO REPRESENT "NOT FOUND"?
uint32 find_objs_range(OBJ *sorted_array, uint32 len, OBJ obj, uint32 &count);
uint32 find_idxs_range(uint32 *sorted_idx_array, OBJ *values, uint32 len, OBJ obj, uint32 &count);
uint32 find_objs_range(OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg, uint32 &count);
uint32 find_idxs_range(uint32 *index, OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg, uint32 &count);

int comp_objs(OBJ obj1, OBJ obj2);
int cmp_objs(OBJ obj1, OBJ obj2);

///////////////////////////////// surr-set.cpp /////////////////////////////////

uint32 surr_set_size(SURR_SET *);

void surr_set_init(SURR_SET *);
void surr_set_clear(SURR_SET *);
bool surr_set_try_insert(SURR_SET *, uint32);

///////////////////////////////// datetime.cpp /////////////////////////////////

void get_year_month_day(int32 epoc_days, int32 &year, int32 &month, int32 &day);
bool is_valid_date(int32 year, int32 month, int32 day);
bool date_time_is_within_range(int32 days_since_epoc, int64 day_time_ns);
int32 days_since_epoc(int32 year, int32 month, int32 day);
int64 epoc_time_ns(int32 days_since_epoc, int64 day_time_ns);

//////////////////////////////// symb-table.cpp ////////////////////////////////

const char *symb_to_raw_str(uint16);
uint16 lookup_enc_symb_id(const uint64 *, uint32);
uint16 lookup_symb_id(const char *, uint32);

/////////////////////////////// inter-utils.cpp ////////////////////////////////

OBJ /*owned_*/str_to_obj(const char* c_str);

char* obj_to_str(OBJ str_obj);
void obj_to_str(OBJ str_obj, char *buffer, uint32 size);

uint8* obj_to_byte_array(OBJ byte_seq_obj, uint32 &size);

uint64 char_buffer_size(OBJ str_obj);

//////////////////////////////// conversion.cpp ////////////////////////////////

OBJ convert_bool_seq(const bool *array, uint32 size);
OBJ convert_int32_seq(const int32 *array, uint32 size);
OBJ convert_int_seq(const int64 *array, uint32 size);
OBJ convert_float_seq(const double *array, uint32 size);
OBJ convert_text(const char *buffer);

// void export_as_c_string(OBJ obj, char *buffer, uint32 capacity);
uint32 export_as_bool_array(OBJ obj, bool *array, uint32 capacity);
uint32 export_as_long_long_array(OBJ obj, int64 *array, uint32 capacity);
uint32 export_as_float_array(OBJ obj, double *array, uint32 capacity);
void export_literal_as_c_string(OBJ obj, char *buffer, uint32 capacity);

///////////////////////////////// printing.cpp /////////////////////////////////

typedef enum {TEXT, SUB_START, SUB_END} EMIT_ACTION;

void print_obj(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data);

void print(OBJ);
void print_to_buffer_or_file(OBJ obj, char* buffer, uint32 max_size, const char* fname);
uint32 printed_obj(OBJ obj, char* buffer, uint32 max_size);
char *printed_obj(OBJ obj, char *alloc(void *, uint32), void *data);

////////////////////////////////// parsing.cpp /////////////////////////////////

const uint32 PARSER_BUFF_SIZE = 4096;

struct PARSER {
  uint32 (*read)(void *, uint8 *, uint32);
  void *read_state;

  // Byte stream state
  uint8 buffer[PARSER_BUFF_SIZE];
  uint32 offset;
  uint32 count;
  bool eof;
  bool read_error;

  // Character stream state
  int32 next_char;
  uint32 line;
  uint32 column;
};

void init_parser(PARSER *, uint32 (*)(void *, uint8 *, uint32), void *);
bool consume_non_ws_char(PARSER *, char);
bool consume_non_ws_chars(PARSER *, char, char);
bool read_label(PARSER *parser, uint16 *value);
bool parse_obj(PARSER *, OBJ *);
bool skip_value(PARSER *);

bool next_non_ws_char_is(PARSER *, char);

bool parse(const char *text, uint32 size, OBJ *var, uint32 *error_offset);


typedef struct {
  FILE *fp;
} READ_FILE_STATE;

uint32 read_file(void *read_state, uint8 *buffer, uint32 capacity);

///////////////////////////////// writing.cpp //////////////////////////////////

typedef struct {
  FILE *fp;
  bool success;
} WRITE_FILE_STATE;

void write_str(WRITE_FILE_STATE *, const char *);
void write_symb(WRITE_FILE_STATE *, uint16);
void write_obj(WRITE_FILE_STATE *, OBJ);
bool finish_write(WRITE_FILE_STATE *);

///////////////////////////// os-interface-xxx.cpp /////////////////////////////

uint64 get_tick_count();   // Impure

////////////////////////////////// hashing.cpp /////////////////////////////////

uint32 compute_hashcode(OBJ obj);
uint32 hashcode_64(uint64);

////////////////////////////////// concat.cpp //////////////////////////////////

bool no_sum32_overflow(uint64 x, uint64 y);

OBJ concat_ints(OBJ, OBJ);
OBJ concat_floats(OBJ, OBJ);

OBJ concat(OBJ left, OBJ right);
OBJ append(OBJ seq, OBJ obj);

OBJ in_place_concat_uint8(SEQ_OBJ *seq_ptr, uint32 length, uint8 *new_elts, uint32 count);
OBJ in_place_concat_obj(SEQ_OBJ *seq_ptr, uint32 length, OBJ *new_elts, uint32 count);

OBJ in_place_concat_obj_uint8(SEQ_OBJ *seq_ptr, uint32 length, uint8 *new_elts, uint32 count);

OBJ concat_uint8(uint8 *elts1, uint32 len1, uint8 *elts2, uint32 len2);
OBJ concat_obj(OBJ *elts1, uint32 len1, OBJ *elts2, uint32 len2);

OBJ concat_uint8_obj(uint8 *elts1, uint32 len1, OBJ *elts2, uint32 len2);
OBJ concat_obj_uint8(OBJ *elts1, uint32 len1, uint8 *elts2, uint32 len2);

///////////////////////////// not implemented yet //////////////////////////////

int64  get_inner_long(OBJ);

bool   *get_seq_next_frag_bool(OBJ seq, uint32 offset, bool *buffer, uint32 capacity, uint32 *count_var);
int64  *get_seq_next_frag_int64(OBJ seq, uint32 offset, int64 *buffer, uint32 capacity, uint32 *count_var);
double *get_seq_next_frag_double(OBJ seq, uint32 offset, double *buffer, uint32 capacity, uint32 *count_var);
OBJ    *get_seq_next_frag_obj(OBJ seq, uint32 offset, OBJ *buffer, uint32 capacity, uint32 *count_var);

OBJ build_seq_int64(int64* array, int32 size);
OBJ build_seq_int32(int32* array, int32 size);
OBJ build_seq_uint32(uint32* array, int32 size);
OBJ build_seq_int16(int16* array, int32 size);
OBJ build_seq_uint16(uint16* array, int32 size);
OBJ build_seq_int8(int8* array, int32 size);
OBJ build_seq_uint8(uint8* array, int32 size);

OBJ build_seq_bool(bool* array, int32 size);
OBJ build_seq_double(double* array, int32 size);

OBJ build_record(uint16 *labels, OBJ *value, int32 count);

double float_pow(double, double);
double float_sqrt(double);
int64 float_round(double);
int32 cast_int32(int64);
OBJ set_insert(OBJ, OBJ);
OBJ set_remove(OBJ, OBJ);
OBJ set_key_value(OBJ, OBJ, OBJ);
OBJ drop_key(OBJ, OBJ);
OBJ make_tag_int(uint16, int64);

bool tree_set_contains(TREE_SET_NODE *, OBJ);
bool tree_map_contains(TREE_MAP_NODE *, OBJ key, OBJ value);
bool tree_map_contains_key(TREE_MAP_NODE *, OBJ);
bool tree_map_lookup(TREE_MAP_NODE *, OBJ key, OBJ *value);

void rearrange_set_as_array(MIXED_REPR_SET_OBJ *, uint32 size);
void rearrange_map_as_array(MIXED_REPR_MAP_OBJ *, uint32 size);

OBJ *rearrange_if_needed_and_get_set_elts_ptr(OBJ);
BIN_REL_OBJ *rearrange_if_needed_and_get_bin_rel_ptr(OBJ);

uint8 as_byte(int64);
int16 as_short(int64);
int32 as_int(int64);

OBJ    *array_append(OBJ*,    int32, int32&, OBJ);
bool   *array_append(bool*,   int32, int32&, bool);
double *array_append(double*, int32, int32&, double);
int64  *array_append(int64*,  int32, int32&, int64);
int32  *array_append(int32*,  int32, int32&, int32);
uint32 *array_append(uint32*, int32, int32&, uint32);
int16  *array_append(int16*,  int32, int32&, int16);
uint16 *array_append(uint16*, int32, int32&, uint16);
int8   *array_append(int8*,   int32, int32&, int8);
uint8  *array_append(uint8*,  int32, int32&, uint8);

OBJ array_at(OBJ*,    int32, int32);
OBJ array_at(bool*,   int32, int32);
OBJ array_at(double*, int32, int32);
OBJ array_at(int64*,  int32, int32);
OBJ array_at(int32*,  int32, int32);
OBJ array_at(uint32*, int32, int32);
OBJ array_at(int16*,  int32, int32);
OBJ array_at(uint16*, int32, int32);
OBJ array_at(int8*,   int32, int32);
OBJ array_at(uint8*,  int32, int32);

bool bool_array_at(bool*, int32, int32);
double float_array_at(double*, int32, int32);

int64 int_array_at(int64*,  int32, int32);
int64 int_array_at(int32*,  int32, int32);
int64 int_array_at(uint32*, int32, int32);
int64 int_array_at(int16*,  int32, int32);
int64 int_array_at(uint16*, int32, int32);
int64 int_array_at(int8*,   int32, int32);
int64 int_array_at(uint8*,  int32, int32);

OBJ    get_obj_at(OBJ, int64);
double get_float_at(OBJ, int64);
int64  get_int_at(OBJ, int64);

int64 get_int_at_unchecked(OBJ seq, uint32 idx);

void copy_int64_range_unchecked(OBJ seq, uint32 start, uint32 len, int64 *buffer);
void copy_int32_range_unchecked(OBJ seq, uint32 start, uint32 len, int32 *buffer);
void copy_int16_range_unchecked(OBJ seq, uint32 start, uint32 len, int16 *buffer);
void copy_int8_range_unchecked(OBJ seq, uint32 start, uint32 len, int8 *buffer);
void copy_uint8_range_unchecked(OBJ seq, uint32 start, uint32 len, uint8 *buffer);

bool is_ne_int_seq(OBJ);
bool is_ne_float_seq(OBJ);

uint32 next_size(uint32 base_size, uint32 min_size);

OBJ copy_obj(OBJ); // mem-copying.cpp

bool needs_copying(void*);

//////////////////////////////// mem-alloc.cpp /////////////////////////////////

void switch_mem_stacks();
void unswitch_mem_stacks();
void clear_unused_mem();

///////////////////////////////// mem-core.cpp /////////////////////////////////

void switch_to_static_allocator();
void switch_to_twin_stacks_allocator();

//////////////////////////// key-checking-utils.cpp ////////////////////////////

void col_update_bit_map_init(COL_UPDATE_BIT_MAP *bit_map);
void col_update_bit_map_clear(COL_UPDATE_BIT_MAP *bit_map);
bool col_update_bit_map_check_and_set(COL_UPDATE_BIT_MAP *bit_map, uint32 index, STATE_MEM_POOL *);
void col_update_bit_map_set(COL_UPDATE_BIT_MAP *bit_map, uint32 index, STATE_MEM_POOL *);
bool col_update_bit_map_is_set(COL_UPDATE_BIT_MAP *bit_map, uint32 index);

void col_update_status_map_init(COL_UPDATE_STATUS_MAP *);
void col_update_status_map_clear(COL_UPDATE_STATUS_MAP *);
bool col_update_status_map_is_dirty(COL_UPDATE_STATUS_MAP *);
void col_update_status_map_mark_deletion(COL_UPDATE_STATUS_MAP *, uint32 index, STATE_MEM_POOL *);
bool col_update_status_map_check_and_mark_insertion(COL_UPDATE_STATUS_MAP *, uint32 index, STATE_MEM_POOL *);
bool col_update_status_map_deleted_flag_is_set(COL_UPDATE_STATUS_MAP *, uint32 index);

/////////////////////////////// unary-table.cpp ////////////////////////////////

void unary_table_init(UNARY_TABLE *, STATE_MEM_POOL *);

bool unary_table_contains(UNARY_TABLE *, uint32);
uint64 unary_table_size(UNARY_TABLE *); //## WHY IS THIS A uint64

uint32 unary_table_capacity(UNARY_TABLE *);

bool unary_table_insert(UNARY_TABLE *, uint32, STATE_MEM_POOL *);
bool unary_table_delete(UNARY_TABLE *, uint32);
void unary_table_clear(UNARY_TABLE *);

void unary_table_copy_to(UNARY_TABLE *table, OBJ (*)(void *, uint32), void *, STREAM *stream);
void unary_table_write(WRITE_FILE_STATE *, UNARY_TABLE *, OBJ (*)(void *, uint32), void *);

void unary_table_iter_init(UNARY_TABLE *, UNARY_TABLE_ITER *);
void unary_table_iter_move_forward(UNARY_TABLE_ITER *);
bool unary_table_iter_is_out_of_range(UNARY_TABLE_ITER *);
uint32 unary_table_iter_get(UNARY_TABLE_ITER *);

///////////////////////////// unary-table-aux.cpp //////////////////////////////

void   unary_table_aux_init(UNARY_TABLE_AUX *, STATE_MEM_POOL *);

uint32 unary_table_aux_insert(UNARY_TABLE_AUX *, uint32);
void   unary_table_aux_delete(UNARY_TABLE_AUX *, uint32);
void   unary_table_aux_clear(UNARY_TABLE *, UNARY_TABLE_AUX *);

void   unary_table_aux_apply(UNARY_TABLE *, UNARY_TABLE_AUX *, void (*)(void *, uint32), void (*)(void *, void *, uint32), void *, void *, STATE_MEM_POOL *);
void   unary_table_aux_reset(UNARY_TABLE_AUX *);

void unary_table_aux_prepare(UNARY_TABLE_AUX *);
bool unary_table_aux_contains(UNARY_TABLE *, UNARY_TABLE_AUX *, uint32);
bool unary_table_aux_is_empty(UNARY_TABLE *, UNARY_TABLE_AUX *);

///////////////////////////////// bin-table.cpp ////////////////////////////////

void bin_table_init(BIN_TABLE *, STATE_MEM_POOL *);

uint32 bin_table_size(BIN_TABLE *);
uint32 bin_table_count_1(BIN_TABLE *, uint32 arg1);
uint32 bin_table_count_2(BIN_TABLE *, uint32 arg2);

bool bin_table_contains(BIN_TABLE *, uint32 arg1, uint32 arg2);
bool bin_table_contains_1(BIN_TABLE *, uint32 arg1);
bool bin_table_contains_2(BIN_TABLE *, uint32 arg2);

uint32 bin_table_lookup_1(BIN_TABLE *, uint32 arg1);
uint32 bin_table_lookup_2(BIN_TABLE *, uint32 arg2);

uint32 bin_table_restrict_1(BIN_TABLE *, uint32 arg1, uint32 *args2);
uint32 bin_table_restrict_2(BIN_TABLE *, uint32 arg2, uint32 *args1);

bool bin_table_insert(BIN_TABLE *, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);
bool bin_table_delete(BIN_TABLE *, uint32 arg1, uint32 arg2);
void bin_table_delete_1(BIN_TABLE *, uint32 arg1);
void bin_table_delete_2(BIN_TABLE *, uint32 arg2);
void bin_table_clear(BIN_TABLE *, STATE_MEM_POOL *);

bool bin_table_col_1_is_key(BIN_TABLE *);
bool bin_table_col_2_is_key(BIN_TABLE *);

void bin_table_copy_to(BIN_TABLE *, OBJ (*)(void *, uint32), void *, OBJ (*)(void *, uint32), void *, STREAM *, STREAM *);
void bin_table_write(WRITE_FILE_STATE *, BIN_TABLE *, OBJ (*)(void *, uint32), void *, OBJ (*)(void *, uint32), void *, bool as_map, bool flipped);

void bin_table_iter_init_empty(BIN_TABLE_ITER *);
void bin_table_iter_init(BIN_TABLE *, BIN_TABLE_ITER *);
void bin_table_iter_move_forward(BIN_TABLE_ITER *);
bool bin_table_iter_is_out_of_range(BIN_TABLE_ITER *);
uint32 bin_table_iter_get_1(BIN_TABLE_ITER *);
uint32 bin_table_iter_get_2(BIN_TABLE_ITER *);

void bin_table_iter_1_init_empty(BIN_TABLE_ITER_1 *);
void bin_table_iter_1_init(BIN_TABLE *, BIN_TABLE_ITER_1 *, uint32 arg1);
void bin_table_iter_1_move_forward(BIN_TABLE_ITER_1 *);
bool bin_table_iter_1_is_out_of_range(BIN_TABLE_ITER_1 *);
uint32 bin_table_iter_1_get_1(BIN_TABLE_ITER_1 *);

void bin_table_iter_2_init_empty(BIN_TABLE_ITER_2 *);
void bin_table_iter_2_init(BIN_TABLE *, BIN_TABLE_ITER_2 *, uint32 arg2);
void bin_table_iter_2_move_forward(BIN_TABLE_ITER_2 *);
bool bin_table_iter_2_is_out_of_range(BIN_TABLE_ITER_2 *);
uint32 bin_table_iter_2_get_1(BIN_TABLE_ITER_2 *);

/////////////////////////////// bin-table-aux.cpp //////////////////////////////

void bin_table_aux_init(BIN_TABLE_AUX *, STATE_MEM_POOL *);

void bin_table_aux_clear(BIN_TABLE_AUX *);
void bin_table_aux_delete(BIN_TABLE *, BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void bin_table_aux_delete_1(BIN_TABLE_AUX *, uint32 arg1);
void bin_table_aux_delete_2(BIN_TABLE_AUX *, uint32 arg2);
void bin_table_aux_insert(BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);

bool bin_table_aux_check_key_1(BIN_TABLE *, BIN_TABLE_AUX *, STATE_MEM_POOL *);
bool bin_table_aux_check_key_2(BIN_TABLE *, BIN_TABLE_AUX *, STATE_MEM_POOL *);

void bin_table_aux_apply(BIN_TABLE *, BIN_TABLE_AUX *, void (*)(void *, uint32), void (*)(void *, void *, uint32), void *, void *, void (*)(void *, uint32), void (*)(void *, void *, uint32), void *, void *, STATE_MEM_POOL *);
void bin_table_aux_reset(BIN_TABLE_AUX *);

bool bin_table_aux_was_deleted(BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);

void bin_table_aux_prepare(BIN_TABLE_AUX *);
bool bin_table_aux_contains(BIN_TABLE *, BIN_TABLE_AUX *, uint32, uint32);
bool bin_table_aux_contains_1(BIN_TABLE *, BIN_TABLE_AUX *, uint32);
bool bin_table_aux_contains_2(BIN_TABLE *, BIN_TABLE_AUX *, uint32);
bool bin_table_aux_is_empty(BIN_TABLE *, BIN_TABLE_AUX *);

bool bin_table_aux_check_foreign_key_unary_table_1_forward(BIN_TABLE *, BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool bin_table_aux_check_foreign_key_unary_table_2_forward(BIN_TABLE *, BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);

bool bin_table_aux_check_foreign_key_unary_table_1_backward(BIN_TABLE *, BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool bin_table_aux_check_foreign_key_unary_table_2_backward(BIN_TABLE *, BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool bin_table_aux_check_foreign_key_master_bin_table_surr_backward(BIN_TABLE *, BIN_TABLE_AUX *, MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *);

/////////////////////////////// sym-bin-table.cpp //////////////////////////////

void sym_bin_table_init(BIN_TABLE *, STATE_MEM_POOL *);

uint32 sym_bin_table_size(BIN_TABLE *);
bool sym_bin_table_contains(BIN_TABLE *, uint32 arg1, uint32 arg2);
bool sym_bin_table_contains_1(BIN_TABLE *, uint32 arg);
uint32 sym_bin_table_count(BIN_TABLE *, uint32 arg);
uint32 sym_bin_table_restrict(BIN_TABLE *, uint32 arg, uint32 *other_args);
uint32 sym_bin_table_lookup(BIN_TABLE *, uint32 arg);

bool sym_bin_table_insert(BIN_TABLE *, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);
bool sym_bin_table_delete(BIN_TABLE *, uint32 arg1, uint32 arg2);
void sym_bin_table_delete_1(BIN_TABLE *, uint32 arg);
void sym_bin_table_clear(BIN_TABLE *, STATE_MEM_POOL *);

void sym_bin_table_copy_to(BIN_TABLE *, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *, STREAM *);
void sym_bin_table_write(WRITE_FILE_STATE *write_state, BIN_TABLE *, OBJ (*surr_to_obj)(void *, uint32), void *store);

void sym_bin_table_iter_init_empty(BIN_TABLE_ITER *);
void sym_bin_table_iter_init(BIN_TABLE *, BIN_TABLE_ITER *);
void sym_bin_table_iter_move_forward(BIN_TABLE_ITER *);
bool sym_bin_table_iter_is_out_of_range(BIN_TABLE_ITER *);
uint32 sym_bin_table_iter_get_1(BIN_TABLE_ITER *);
uint32 sym_bin_table_iter_get_2(BIN_TABLE_ITER *);

void sym_bin_table_iter_1_init_empty(BIN_TABLE_ITER_1 *);
void sym_bin_table_iter_1_init(BIN_TABLE *, BIN_TABLE_ITER_1 *, uint32 arg);
void sym_bin_table_iter_1_move_forward(BIN_TABLE_ITER_1 *);
bool sym_bin_table_iter_1_is_out_of_range(BIN_TABLE_ITER_1 *);
uint32 sym_bin_table_iter_1_get_1(BIN_TABLE_ITER_1 *);

///////////////////////////// sym-bin-table-aux.cpp ////////////////////////////

void sym_bin_table_aux_init(SYM_BIN_TABLE_AUX *, STATE_MEM_POOL *);
void sym_bin_table_aux_reset(SYM_BIN_TABLE_AUX *);

void sym_bin_table_aux_insert(SYM_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void sym_bin_table_aux_delete(SYM_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void sym_bin_table_aux_delete_1(SYM_BIN_TABLE_AUX *, uint32 arg);
void sym_bin_table_aux_clear(SYM_BIN_TABLE_AUX *);

void sym_bin_table_aux_apply(BIN_TABLE *, SYM_BIN_TABLE_AUX *, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *);

///////////////////////////// master-bin-table.cpp /////////////////////////////

void master_bin_table_init(MASTER_BIN_TABLE *table, STATE_MEM_POOL *);

uint32 master_bin_table_size(MASTER_BIN_TABLE *table);
uint32 master_bin_table_count_1(MASTER_BIN_TABLE *table, uint32 arg1);
uint32 master_bin_table_count_2(MASTER_BIN_TABLE *table, uint32 arg2);

bool master_bin_table_contains(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2);
bool master_bin_table_contains_1(MASTER_BIN_TABLE *table, uint32 arg1);
bool master_bin_table_contains_2(MASTER_BIN_TABLE *table, uint32 arg2);
bool master_bin_table_contains_surr(MASTER_BIN_TABLE *table, uint32 surr);

uint32 master_bin_table_restrict_1(MASTER_BIN_TABLE *table, uint32 arg1, uint32 *args2, uint32 *surrs);
uint32 master_bin_table_restrict_2(MASTER_BIN_TABLE *table, uint32 arg2, uint32 *args1);

uint32 master_bin_table_lookup_1(MASTER_BIN_TABLE *table, uint32 arg1);
uint32 master_bin_table_lookup_2(MASTER_BIN_TABLE *table, uint32 arg2);

uint32 master_bin_table_lookup_surrogate(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2);

uint32 master_bin_table_get_arg_1(MASTER_BIN_TABLE *, uint32 surr);
uint32 master_bin_table_get_arg_2(MASTER_BIN_TABLE *, uint32 surr);

void master_bin_table_clear(MASTER_BIN_TABLE *table, STATE_MEM_POOL *);
bool master_bin_table_delete(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2);
void master_bin_table_delete_1(MASTER_BIN_TABLE *table, uint32 arg1);
void master_bin_table_delete_2(MASTER_BIN_TABLE *table, uint32 arg2);

int32 master_bin_table_insert_ex(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);
bool master_bin_table_insert(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);

// bool master_bin_table_col_1_is_key(MASTER_BIN_TABLE *table);
// bool master_bin_table_col_2_is_key(MASTER_BIN_TABLE *table);

void master_bin_table_copy_to(MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2);
void master_bin_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool flipped);

void master_bin_table_iter_init_empty(MASTER_BIN_TABLE_ITER *);
void master_bin_table_iter_init(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_ITER *);
void master_bin_table_iter_move_forward(MASTER_BIN_TABLE_ITER *);
bool master_bin_table_iter_is_out_of_range(MASTER_BIN_TABLE_ITER *);
uint32 master_bin_table_iter_get_1(MASTER_BIN_TABLE_ITER *);
uint32 master_bin_table_iter_get_2(MASTER_BIN_TABLE_ITER *);
uint32 master_bin_table_iter_get_surr(MASTER_BIN_TABLE_ITER *);

void master_bin_table_iter_1_init_empty(MASTER_BIN_TABLE_ITER_1 *);
void master_bin_table_iter_1_init(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_ITER_1 *, uint32 arg1);
void master_bin_table_iter_1_init_surrs(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_ITER_1 *, uint32 arg1);
void master_bin_table_iter_1_move_forward(MASTER_BIN_TABLE_ITER_1 *);
bool master_bin_table_iter_1_is_out_of_range(MASTER_BIN_TABLE_ITER_1 *);
uint32 master_bin_table_iter_1_get_1(MASTER_BIN_TABLE_ITER_1 *);
uint32 master_bin_table_iter_1_get_2(MASTER_BIN_TABLE_ITER_1 *);
uint32 master_bin_table_iter_1_get_surr(MASTER_BIN_TABLE_ITER_1 *);

void master_bin_table_iter_2_init_empty(MASTER_BIN_TABLE_ITER_2 *);
void master_bin_table_iter_2_init(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_ITER_2 *, uint32 arg2);
void master_bin_table_iter_2_move_forward(MASTER_BIN_TABLE_ITER_2 *);
bool master_bin_table_iter_2_is_out_of_range(MASTER_BIN_TABLE_ITER_2 *);
uint32 master_bin_table_iter_2_get_1(MASTER_BIN_TABLE_ITER_2 *);
uint32 master_bin_table_iter_2_get_2(MASTER_BIN_TABLE_ITER_2 *);
uint32 master_bin_table_iter_2_get_surr(MASTER_BIN_TABLE_ITER_2 *);

/////////////////////////// master-bin-table-aux.cpp ///////////////////////////

void master_bin_table_aux_init(MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *);
void master_bin_table_aux_clear(MASTER_BIN_TABLE_AUX *table_aux);
void master_bin_table_aux_delete(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2);
void master_bin_table_aux_delete_1(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1);
void master_bin_table_aux_delete_2(MASTER_BIN_TABLE_AUX *table_aux, uint32 arg2);

uint32 master_bin_table_aux_insert(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);

uint32 master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);

// bool master_bin_table_aux_check_key_1(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux);
// bool master_bin_table_aux_check_key_2(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux);

void master_bin_table_aux_apply(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, STATE_MEM_POOL *);
void master_bin_table_aux_reset(MASTER_BIN_TABLE_AUX *table_aux);

void master_bin_table_aux_prepare(MASTER_BIN_TABLE_AUX *);
bool master_bin_table_aux_contains_1(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, uint32);
bool master_bin_table_aux_contains_2(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, uint32);
bool master_bin_table_aux_contains_surr(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, uint32);
bool master_bin_table_aux_is_empty(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *);

bool master_bin_table_aux_check_foreign_key_unary_table_1_forward(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool master_bin_table_aux_check_foreign_key_unary_table_2_forward(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);

bool master_bin_table_aux_check_foreign_key_unary_table_1_backward(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool master_bin_table_aux_check_foreign_key_unary_table_2_backward(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);

/////////////////////////// sym-master-bin-table.cpp ///////////////////////////

void sym_master_bin_table_init(MASTER_BIN_TABLE *, STATE_MEM_POOL *);

uint32 sym_master_bin_table_size(MASTER_BIN_TABLE *);
bool sym_master_bin_table_contains(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2);
bool sym_master_bin_table_contains_1(MASTER_BIN_TABLE *, uint32 arg);
bool sym_master_bin_table_contains_surr(MASTER_BIN_TABLE *, uint32 surr);
uint32 sym_master_bin_table_count(MASTER_BIN_TABLE *, uint32 arg);
uint32 sym_master_bin_table_restrict(MASTER_BIN_TABLE *, uint32 arg, uint32 *other_args);
uint32 sym_master_bin_table_lookup(MASTER_BIN_TABLE *, uint32 arg);
uint32 sym_master_bin_table_lookup_surrogate(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2);
uint32 sym_master_bin_table_get_arg_1(MASTER_BIN_TABLE *, uint32 surr);
uint32 sym_master_bin_table_get_arg_2(MASTER_BIN_TABLE *, uint32 surr);

int32 sym_master_bin_table_insert_ex(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);
bool sym_master_bin_table_insert(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);

bool sym_master_bin_table_delete(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2);
void sym_master_bin_table_delete_1(MASTER_BIN_TABLE *, uint32 arg);
void sym_master_bin_table_clear(MASTER_BIN_TABLE *, STATE_MEM_POOL *);

void sym_master_bin_table_copy_to(MASTER_BIN_TABLE *, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *, STREAM *);
void sym_master_bin_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *, OBJ (*surr_to_obj)(void *, uint32), void *store);

void sym_master_bin_table_iter_init_empty(MASTER_BIN_TABLE_ITER *);
void sym_master_bin_table_iter_init(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_ITER *);
void sym_master_bin_table_iter_move_forward(MASTER_BIN_TABLE_ITER *);
bool sym_master_bin_table_iter_is_out_of_range(MASTER_BIN_TABLE_ITER *);
uint32 sym_master_bin_table_iter_get_1(MASTER_BIN_TABLE_ITER *);
uint32 sym_master_bin_table_iter_get_2(MASTER_BIN_TABLE_ITER *);
uint32 sym_master_bin_table_iter_get_surr(MASTER_BIN_TABLE_ITER *);

void sym_master_bin_table_iter_1_init_empty(SYM_MASTER_BIN_TABLE_ITER_1 *);
void sym_master_bin_table_iter_1_init(MASTER_BIN_TABLE *, SYM_MASTER_BIN_TABLE_ITER_1 *, uint32 arg);
void sym_master_bin_table_iter_1_move_forward(SYM_MASTER_BIN_TABLE_ITER_1 *);
bool sym_master_bin_table_iter_1_is_out_of_range(SYM_MASTER_BIN_TABLE_ITER_1 *);
uint32 sym_master_bin_table_iter_1_get_1(SYM_MASTER_BIN_TABLE_ITER_1 *);
uint32 sym_master_bin_table_iter_1_get_surr(SYM_MASTER_BIN_TABLE_ITER_1 *);

///////////////////////// sym-master-bin-table-aux.cpp /////////////////////////

void sym_master_bin_table_aux_init(SYM_MASTER_BIN_TABLE_AUX *, STATE_MEM_POOL *);
void sym_master_bin_table_aux_reset(SYM_MASTER_BIN_TABLE_AUX *);

uint32 sym_master_bin_table_aux_insert(MASTER_BIN_TABLE *, SYM_MASTER_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);

void sym_master_bin_table_aux_delete(SYM_MASTER_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void sym_master_bin_table_aux_delete_1(SYM_MASTER_BIN_TABLE_AUX *, uint32 arg);
void sym_master_bin_table_aux_clear(SYM_MASTER_BIN_TABLE_AUX *);

uint32 sym_master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *, SYM_MASTER_BIN_TABLE_AUX *, uint32 arg1, uint32 arg2);

void sym_master_bin_table_aux_apply(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool);

bool semisym_tern_table_aux_check_key_12(TERN_TABLE *, SEMISYM_TERN_TABLE_AUX *, STATE_MEM_POOL *);
bool semisym_tern_table_aux_check_key_3(TERN_TABLE *, SEMISYM_TERN_TABLE_AUX *, STATE_MEM_POOL *);

///////////////////////////// slave-tern-table.cpp /////////////////////////////

bool slave_tern_table_insert(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *);
bool slave_tern_table_insert(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3, STATE_MEM_POOL *);
bool slave_tern_table_delete(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3);
void slave_tern_table_clear(BIN_TABLE *slave_table, STATE_MEM_POOL *);

uint32 slave_tern_table_size(BIN_TABLE *slave_table);
uint32 slave_tern_table_count_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1);
uint32 slave_tern_table_count_2(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2);
uint32 slave_tern_table_count_3(BIN_TABLE *slave_table, uint32 arg3);
uint32 slave_tern_table_count_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2);
uint32 slave_tern_table_count_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3);
uint32 slave_tern_table_count_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2, uint32 arg3);

bool slave_tern_table_contains(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3);
bool slave_tern_table_contains_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1);
bool slave_tern_table_contains_2(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2);
bool slave_tern_table_contains_3(BIN_TABLE *slave_table, uint32 arg3);
bool slave_tern_table_contains_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2);
bool slave_tern_table_contains_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3);
bool slave_tern_table_contains_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2, uint32 arg3);

uint32 slave_tern_table_lookup_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2);
uint32 slave_tern_table_lookup_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3);
uint32 slave_tern_table_lookup_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2, uint32 arg3);

bool slave_tern_table_cols_12_are_key(BIN_TABLE *slave_table);
bool slave_tern_table_col_3_is_key(BIN_TABLE *slave_table);

void slave_tern_table_copy_to(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3);
void slave_tern_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3);

void slave_tern_table_iter_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER *iter);
void slave_tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *iter);
bool slave_tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *iter);
uint32 slave_tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *iter);
uint32 slave_tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *iter);
uint32 slave_tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *iter);

void slave_tern_table_iter_1_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_1 *iter, uint32 arg1);
void slave_tern_table_iter_1_move_forward(SLAVE_TERN_TABLE_ITER_1 *iter);
bool slave_tern_table_iter_1_is_out_of_range(SLAVE_TERN_TABLE_ITER_1 *iter);
uint32 slave_tern_table_iter_1_get_1(SLAVE_TERN_TABLE_ITER_1 *iter);
uint32 slave_tern_table_iter_1_get_2(SLAVE_TERN_TABLE_ITER_1 *iter);

void slave_tern_table_iter_2_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_2 *iter, uint32 arg2);
void slave_tern_table_iter_2_move_forward(SLAVE_TERN_TABLE_ITER_2 *iter);
bool slave_tern_table_iter_2_is_out_of_range(SLAVE_TERN_TABLE_ITER_2 *iter);
uint32 slave_tern_table_iter_2_get_1(SLAVE_TERN_TABLE_ITER_2 *iter);
uint32 slave_tern_table_iter_2_get_2(SLAVE_TERN_TABLE_ITER_2 *iter);

void slave_tern_table_iter_12_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_12 *iter, uint32 arg1, uint32 arg2);
void slave_tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *iter);
bool slave_tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *iter);
uint32 slave_tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *iter);

void slave_tern_table_iter_3_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_3 *iter, uint32 arg3);
void slave_tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *iter);
bool slave_tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *iter);
uint32 slave_tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *iter);
uint32 slave_tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *iter);

void slave_tern_table_iter_13_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_13 *iter, uint32 arg1, uint32 arg3);
void slave_tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *iter);
bool slave_tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *iter);
uint32 slave_tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *iter);

void slave_tern_table_iter_23_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_23 *iter, uint32 arg2, uint32 arg3);
void slave_tern_table_iter_23_move_forward(SLAVE_TERN_TABLE_ITER_23 *iter);
bool slave_tern_table_iter_23_is_out_of_range(SLAVE_TERN_TABLE_ITER_23 *iter);
uint32 slave_tern_table_iter_23_get_1(SLAVE_TERN_TABLE_ITER_23 *iter);

/////////////////////////// slave-tern-table-aux.cpp ///////////////////////////

void slave_tern_table_aux_init(SLAVE_TERN_TABLE_AUX *, STATE_MEM_POOL *);

void slave_tern_table_aux_clear(SLAVE_TERN_TABLE_AUX *);
void slave_tern_table_aux_delete(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg2, uint32 arg3);
void slave_tern_table_aux_delete_12(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void slave_tern_table_aux_delete_13(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg3);
void slave_tern_table_aux_delete_23(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg2, uint32 arg3);
void slave_tern_table_aux_delete_1(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1);
void slave_tern_table_aux_delete_2(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg2);
void slave_tern_table_aux_delete_3(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg3);

void slave_tern_table_aux_insert(SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg2, uint32 arg3);
void slave_tern_table_aux_insert(MASTER_BIN_TABLE *, MASTER_BIN_TABLE_AUX *, SLAVE_TERN_TABLE_AUX *, uint32, uint32, uint32);

bool slave_tern_table_aux_check_key_12(BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, STATE_MEM_POOL *);
bool slave_tern_table_aux_check_key_3(BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, STATE_MEM_POOL *);

void slave_tern_table_aux_apply(MASTER_BIN_TABLE *, BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *);
void slave_tern_table_aux_reset(SLAVE_TERN_TABLE_AUX *);

bool tern_table_aux_check_foreign_key_unary_table_1_forward(TERN_TABLE *tabl, TERN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool tern_table_aux_check_foreign_key_unary_table_2_forward(TERN_TABLE *tabl, TERN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
bool tern_table_aux_check_foreign_key_unary_table_3_forward(TERN_TABLE *tabl, TERN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *);
// bool tern_table_aux_check_foreign_key_bin_table_forward(TERN_TABLE *, TERN_TABLE_AUX *, BIN_TABLE *, BIN_TABLE_AUX *);

///////////////////////// semisym-slave-tern-table.cpp /////////////////////////

bool semisym_slave_tern_table_insert(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *);
bool semisym_slave_tern_table_insert(BIN_TABLE *, uint32 surr12, uint32 arg3, STATE_MEM_POOL *);

bool semisym_slave_tern_table_delete(BIN_TABLE *, uint32 surr12, uint32 arg3);
void semisym_slave_tern_table_clear(BIN_TABLE *, STATE_MEM_POOL *);

uint32 semisym_slave_tern_table_size(BIN_TABLE *);

bool semisym_slave_tern_table_contains(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg1, uint32 arg2, uint32 arg3);
bool semisym_slave_tern_table_contains_12(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg1, uint32 arg2);
bool semisym_slave_tern_table_contains_13(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg12, uint32 arg3);
bool semisym_slave_tern_table_contains_1(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg);
bool semisym_slave_tern_table_contains_3(BIN_TABLE *, uint32 arg3);

uint32 semisym_slave_tern_table_lookup_12(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg1, uint32 arg2);
uint32 semisym_slave_tern_table_lookup_13(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg12, uint32 arg3);

uint32 semisym_slave_tern_table_count_12(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg1, uint32 arg2);
uint32 semisym_slave_tern_table_count_13(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg12, uint32 arg3);
uint32 semisym_slave_tern_table_count_1(MASTER_BIN_TABLE *, BIN_TABLE *, uint32 arg12);
uint32 semisym_slave_tern_table_count_3(BIN_TABLE *, uint32 arg3);

bool semisym_slave_tern_table_cols_12_are_key(BIN_TABLE *);
bool semisym_slave_tern_table_col_3_is_key(BIN_TABLE *);

void semisym_slave_tern_table_copy_to(MASTER_BIN_TABLE *, BIN_TABLE *, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *, STREAM *, STREAM *);
void semisym_slave_tern_table_write(WRITE_FILE_STATE *, MASTER_BIN_TABLE *, BIN_TABLE *, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3);

void semisym_slave_tern_table_iter_init(MASTER_BIN_TABLE *, BIN_TABLE *, SLAVE_TERN_TABLE_ITER *);
void semisym_slave_tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *);
bool semisym_slave_tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *);
uint32 semisym_slave_tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *);
uint32 semisym_slave_tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *);
uint32 semisym_slave_tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *);

void semisym_slave_tern_table_iter_1_init(MASTER_BIN_TABLE *, BIN_TABLE *, SEMISYM_SLAVE_TERN_TABLE_ITER_1 *, uint32 arg1);
void semisym_slave_tern_table_iter_1_move_forward(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *);
bool semisym_slave_tern_table_iter_1_is_out_of_range(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *);
uint32 semisym_slave_tern_table_iter_1_get_1(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *);
uint32 semisym_slave_tern_table_iter_1_get_2(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *);

void semisym_slave_tern_table_iter_3_init(MASTER_BIN_TABLE *, BIN_TABLE *, SLAVE_TERN_TABLE_ITER_3 *, uint32 arg3);
void semisym_slave_tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *);
bool semisym_slave_tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *);
uint32 semisym_slave_tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *);
uint32 semisym_slave_tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *);

void semisym_slave_tern_table_iter_12_init(MASTER_BIN_TABLE *, BIN_TABLE *, SLAVE_TERN_TABLE_ITER_12 *, uint32 arg1, uint32 arg2);
void semisym_slave_tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *);
bool semisym_slave_tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *);
uint32 semisym_slave_tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *);

void semisym_slave_tern_table_iter_13_init(MASTER_BIN_TABLE *, BIN_TABLE *, SLAVE_TERN_TABLE_ITER_13 *, uint32 arg1, uint32 arg3);
void semisym_slave_tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *);
bool semisym_slave_tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *);
uint32 semisym_slave_tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *);

/////////////////////// semisym-slave-tern-table-aux.cpp ///////////////////////

void semisym_slave_tern_table_aux_init(SLAVE_TERN_TABLE_AUX *, STATE_MEM_POOL *);
void semisym_slave_tern_table_aux_reset(SLAVE_TERN_TABLE_AUX *);

void semisym_slave_tern_table_aux_clear(SLAVE_TERN_TABLE_AUX *);
void semisym_slave_tern_table_aux_delete(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg2, uint32 arg3);
void semisym_slave_tern_table_aux_delete_12(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void semisym_slave_tern_table_aux_delete_13(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1, uint32 arg3);
void semisym_slave_tern_table_aux_delete_1(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg1);
void semisym_slave_tern_table_aux_delete_3(MASTER_BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, uint32 arg3);

void slave_tern_table_aux_insert(MASTER_BIN_TABLE *, SYM_MASTER_BIN_TABLE_AUX *, SLAVE_TERN_TABLE_AUX *, uint32, uint32, uint32);

void semisym_slave_tern_table_aux_apply(MASTER_BIN_TABLE *, BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *);

bool semisym_slave_tern_table_aux_check_key_12(BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, STATE_MEM_POOL *);
bool semisym_slave_tern_table_aux_check_key_3(BIN_TABLE *, SLAVE_TERN_TABLE_AUX *, STATE_MEM_POOL *);

//////////////////////////////// tern-table.cpp ////////////////////////////////

bool tern_table_insert(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *);
bool tern_table_delete(TERN_TABLE *table, uint32 surr1, uint32 surr2, uint32 arg3);
void tern_table_clear(TERN_TABLE *table, STATE_MEM_POOL *);

uint32 tern_table_size(TERN_TABLE *table);

uint32 tern_table_count_1(TERN_TABLE *table, uint32 arg1);
uint32 tern_table_count_2(TERN_TABLE *table, uint32 arg2);
uint32 tern_table_count_3(TERN_TABLE *table, uint32 arg3);
uint32 tern_table_count_12(TERN_TABLE *table, uint32 arg1, uint32 arg2);
uint32 tern_table_count_13(TERN_TABLE *table, uint32 arg1, uint32 arg3);
uint32 tern_table_count_23(TERN_TABLE *table, uint32 arg2, uint32 arg3);

bool tern_table_contains(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3);
bool tern_table_contains_1(TERN_TABLE *table, uint32 arg1);
bool tern_table_contains_2(TERN_TABLE *table, uint32 arg2);
bool tern_table_contains_3(TERN_TABLE *table, uint32 arg3);
bool tern_table_contains_12(TERN_TABLE *table, uint32 arg1, uint32 arg2);
bool tern_table_contains_13(TERN_TABLE *table, uint32 arg1, uint32 arg3);
bool tern_table_contains_23(TERN_TABLE *table, uint32 arg2, uint32 arg3);

uint32 tern_table_lookup_12(TERN_TABLE *table, uint32 arg1, uint32 arg2);
uint32 tern_table_lookup_13(TERN_TABLE *table, uint32 arg1, uint32 arg3);
uint32 tern_table_lookup_23(TERN_TABLE *table, uint32 arg2, uint32 arg3);

bool tern_table_cols_12_are_key(TERN_TABLE *table);
bool tern_table_cols_13_are_key(TERN_TABLE *table);
bool tern_table_cols_23_are_key(TERN_TABLE *table);
bool tern_table_col_3_is_key(TERN_TABLE *table);

void tern_table_copy_to(TERN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3);
void tern_table_write(WRITE_FILE_STATE *write_state, TERN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3);

void tern_table_iter_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER *iter);
void tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *iter);
bool tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *iter);
uint32 tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *iter);
uint32 tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *iter);
uint32 tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *iter);

void tern_table_iter_1_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_1 *iter, uint32 arg1);
void tern_table_iter_1_move_forward(SLAVE_TERN_TABLE_ITER_1 *iter);
bool tern_table_iter_1_is_out_of_range(SLAVE_TERN_TABLE_ITER_1 *iter);
uint32 tern_table_iter_1_get_1(SLAVE_TERN_TABLE_ITER_1 *iter);
uint32 tern_table_iter_1_get_2(SLAVE_TERN_TABLE_ITER_1 *iter);

void tern_table_iter_2_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_2 *iter, uint32 arg2);
void tern_table_iter_2_move_forward(SLAVE_TERN_TABLE_ITER_2 *iter);
bool tern_table_iter_2_is_out_of_range(SLAVE_TERN_TABLE_ITER_2 *iter);
uint32 tern_table_iter_2_get_1(SLAVE_TERN_TABLE_ITER_2 *iter);
uint32 tern_table_iter_2_get_2(SLAVE_TERN_TABLE_ITER_2 *iter);

void tern_table_iter_3_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_3 *iter, uint32 arg3);
void tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *iter);
bool tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *iter);
uint32 tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *iter);
uint32 tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *iter);

void tern_table_iter_12_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_12 *iter, uint32 arg1, uint32 arg2);
void tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *iter);
bool tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *iter);
uint32 tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *iter);

void tern_table_iter_13_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_13 *iter, uint32 arg1, uint32 arg3);
void tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *iter);
bool tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *iter);
uint32 tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *iter);

void tern_table_iter_23_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_23 *iter, uint32 arg2, uint32 arg3);
void tern_table_iter_23_move_forward(SLAVE_TERN_TABLE_ITER_23 *iter);
bool tern_table_iter_23_is_out_of_range(SLAVE_TERN_TABLE_ITER_23 *iter);
uint32 tern_table_iter_23_get_1(SLAVE_TERN_TABLE_ITER_23 *iter);

////////////////////////////// tern-table-aux.cpp //////////////////////////////

void tern_table_aux_init(TERN_TABLE_AUX *, STATE_MEM_POOL *);
void tern_table_aux_reset(TERN_TABLE_AUX *);

void tern_table_aux_insert(TERN_TABLE_AUX *, uint32 arg1, uint32 arg2, uint32 arg3);

void tern_table_aux_delete(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg1, uint32 arg2, uint32 arg3);
void tern_table_aux_delete_12(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg1, uint32 arg2);
void tern_table_aux_delete_13(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg1, uint32 arg3);
void tern_table_aux_delete_23(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg2, uint32 arg3);
void tern_table_aux_delete_1(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg1);
void tern_table_aux_delete_2(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg2);
void tern_table_aux_delete_3(TERN_TABLE *, TERN_TABLE_AUX *, uint32 arg3);

void tern_table_aux_clear(TERN_TABLE_AUX *);

void tern_table_aux_apply(TERN_TABLE *, TERN_TABLE_AUX *, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *);

bool tern_table_aux_check_key_3(TERN_TABLE *, TERN_TABLE_AUX *);
bool tern_table_aux_check_key_12(TERN_TABLE *, TERN_TABLE_AUX *);
bool tern_table_aux_check_key_13(TERN_TABLE *, TERN_TABLE_AUX *);
bool tern_table_aux_check_key_23(TERN_TABLE *, TERN_TABLE_AUX *);

bool tern_table_aux_prepare(TERN_TABLE_AUX *);
bool tern_table_aux_contains_1(TERN_TABLE *, TERN_TABLE_AUX *, uint32);
bool tern_table_aux_contains_2(TERN_TABLE *, TERN_TABLE_AUX *, uint32);
bool tern_table_aux_contains_3(TERN_TABLE *, TERN_TABLE_AUX *, uint32);

//////////////////////////// semisym-tern-table.cpp ////////////////////////////

bool semisym_tern_table_insert(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool);

void semisym_tern_table_clear(TERN_TABLE *table, STATE_MEM_POOL *mem_pool);

uint32 semisym_tern_table_size(TERN_TABLE *table);

uint32 semisym_tern_table_count_1(TERN_TABLE *table, uint32 arg1);
uint32 semisym_tern_table_count_3(TERN_TABLE *table, uint32 arg3);
uint32 semisym_tern_table_count_12(TERN_TABLE *table, uint32 arg1, uint32 arg2);
uint32 semisym_tern_table_count_13(TERN_TABLE *table, uint32 arg1, uint32 arg3);

bool semisym_tern_table_contains(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3);
bool semisym_tern_table_contains_1(TERN_TABLE *table, uint32 arg1);
bool semisym_tern_table_contains_3(TERN_TABLE *table, uint32 arg3);
bool semisym_tern_table_contains_12(TERN_TABLE *table, uint32 arg1, uint32 arg2);
bool semisym_tern_table_contains_13(TERN_TABLE *table, uint32 arg1, uint32 arg3);

uint32 semisym_tern_table_lookup_12(TERN_TABLE *table, uint32 arg1, uint32 arg2);
uint32 semisym_tern_table_lookup_13(TERN_TABLE *table, uint32 arg1, uint32 arg3);

bool semisym_tern_table_cols_12_are_key(TERN_TABLE *table);
bool semisym_tern_table_cols_13_are_key(TERN_TABLE *table);
bool semisym_tern_table_col_3_is_key(TERN_TABLE *table);

void semisym_tern_table_copy_to(TERN_TABLE *table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3);
void semisym_tern_table_write(WRITE_FILE_STATE *write_state, TERN_TABLE *table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3);

void semisym_tern_table_iter_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER *iter);
void semisym_tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *iter);
bool semisym_tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *iter);
uint32 semisym_tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *iter);
uint32 semisym_tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *iter);
uint32 semisym_tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *iter);

void semisym_tern_table_iter_1_init(TERN_TABLE *table, SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter, uint32 arg1);
void semisym_tern_table_iter_1_move_forward(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter);
bool semisym_tern_table_iter_1_is_out_of_range(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter);
uint32 semisym_tern_table_iter_1_get_1(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter);
uint32 semisym_tern_table_iter_1_get_2(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter);

void semisym_tern_table_iter_3_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_3 *iter, uint32 arg3);
void semisym_tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *iter);
bool semisym_tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *iter);
uint32 semisym_tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *iter);
uint32 semisym_tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *iter);

void semisym_tern_table_iter_12_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_12 *iter, uint32 arg1, uint32 arg2);
void semisym_tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *iter);
bool semisym_tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *iter);
uint32 semisym_tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *iter);

void semisym_tern_table_iter_13_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_13 *iter, uint32 arg1, uint32 arg3);
void semisym_tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *iter);
bool semisym_tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *iter);
uint32 semisym_tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *iter);

////////////////////////// semisym-tern-table-aux.cpp //////////////////////////

void semisym_tern_table_aux_init(TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool);
void semisym_tern_table_aux_reset(TERN_TABLE_AUX *table_aux);

void semisym_tern_table_aux_insert(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3);

void semisym_tern_table_aux_delete(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3);
void semisym_tern_table_aux_delete_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2);
void semisym_tern_table_aux_delete_13(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3);
void semisym_tern_table_aux_delete_1(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1);
void semisym_tern_table_aux_delete_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg3);
void semisym_tern_table_aux_clear(TERN_TABLE_AUX *table_aux);

void semisym_tern_table_aux_apply(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, void (*incr_rc_1_2)(void *, uint32), void (*decr_rc_1_2)(void *, void *, uint32), void *store_1_2, void *store_aux_1_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool);

bool semisym_tern_table_aux_check_key_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux);
bool semisym_tern_table_aux_check_key_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux);

//////////////////////////////// raw-int-col.cpp ///////////////////////////////

void raw_int_col_init(UNARY_TABLE *, RAW_INT_COL *, STATE_MEM_POOL *);
void raw_int_col_resize(RAW_INT_COL *, uint32 capacity, uint32 new_capacity, STATE_MEM_POOL *);

int64 raw_int_col_lookup(UNARY_TABLE *, RAW_INT_COL *, uint32 idx);

void raw_int_col_insert(RAW_INT_COL *, uint32 idx, int64 value, STATE_MEM_POOL *);
void raw_int_col_update(UNARY_TABLE *, RAW_INT_COL *, uint32 idx, int64 value, STATE_MEM_POOL *);

void raw_int_col_copy_to(UNARY_TABLE *, RAW_INT_COL *, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2);
void raw_int_col_write(WRITE_FILE_STATE *, UNARY_TABLE *, RAW_INT_COL *, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip);

void raw_int_col_iter_init(UNARY_TABLE *, RAW_INT_COL *, RAW_INT_COL_ITER *);
bool raw_int_col_iter_is_out_of_range(RAW_INT_COL_ITER *);
uint32 raw_int_col_iter_get_idx(RAW_INT_COL_ITER *);
int64 raw_int_col_iter_get_value(RAW_INT_COL_ITER *);
void raw_int_col_iter_move_forward(RAW_INT_COL_ITER *);

////////////////////////////// raw-int-col-aux.cpp /////////////////////////////

void raw_int_col_aux_apply(UNARY_TABLE *, UNARY_TABLE_AUX *, RAW_INT_COL *, INT_COL_AUX *, STATE_MEM_POOL *);

bool raw_int_col_aux_check_key_1(UNARY_TABLE *, RAW_INT_COL *, INT_COL_AUX *, STATE_MEM_POOL *);

/////////////////////////////// raw-float-col.cpp //////////////////////////////

void raw_float_col_init(UNARY_TABLE *, RAW_FLOAT_COL *, STATE_MEM_POOL *);
void raw_float_col_resize(RAW_FLOAT_COL *, uint32 capacity, uint32 new_capacity, STATE_MEM_POOL *);

double raw_float_col_lookup(UNARY_TABLE *, RAW_FLOAT_COL *, uint32 idx);

void raw_float_col_insert(RAW_FLOAT_COL *, uint32 idx, double value, STATE_MEM_POOL *);
void raw_float_col_update(UNARY_TABLE *, RAW_FLOAT_COL *, uint32 idx, double value, STATE_MEM_POOL *);

void raw_float_col_copy_to(UNARY_TABLE *, RAW_FLOAT_COL *, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *, STREAM *);
void raw_float_col_write(WRITE_FILE_STATE *, UNARY_TABLE *, RAW_FLOAT_COL *, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip);

void raw_float_col_iter_init(UNARY_TABLE *, RAW_FLOAT_COL *, RAW_FLOAT_COL_ITER *);
bool raw_float_col_iter_is_out_of_range(RAW_FLOAT_COL_ITER *);
uint32 raw_float_col_iter_get_idx(RAW_FLOAT_COL_ITER *);
double raw_float_col_iter_get_value(RAW_FLOAT_COL_ITER *);
void raw_float_col_iter_move_forward(RAW_FLOAT_COL_ITER *);

///////////////////////////// raw-float-col-aux.cpp ////////////////////////////

void raw_float_col_aux_apply(UNARY_TABLE *, UNARY_TABLE_AUX *, RAW_FLOAT_COL *, FLOAT_COL_AUX *, STATE_MEM_POOL *);

bool raw_float_col_aux_check_key_1(UNARY_TABLE *, RAW_FLOAT_COL *, FLOAT_COL_AUX *, STATE_MEM_POOL *);

//////////////////////////////// raw-obj-col.cpp ///////////////////////////////

void   raw_obj_col_init(UNARY_TABLE *, RAW_OBJ_COL *, STATE_MEM_POOL *);
void   raw_obj_col_resize(RAW_OBJ_COL *, uint32, uint32, STATE_MEM_POOL *);

// bool   raw_obj_col_contains_1(OBJ_COL *, uint32);
OBJ    raw_obj_col_lookup(UNARY_TABLE *, RAW_OBJ_COL *, uint32);

void   raw_obj_col_insert(RAW_OBJ_COL *, uint32, OBJ, STATE_MEM_POOL *);
void   raw_obj_col_update(UNARY_TABLE *, RAW_OBJ_COL *, uint32, OBJ, STATE_MEM_POOL *);
void   raw_obj_col_delete(RAW_OBJ_COL *, uint32, STATE_MEM_POOL *);
void   raw_obj_col_clear(UNARY_TABLE *, RAW_OBJ_COL *, STATE_MEM_POOL *);

void   raw_obj_col_copy_to(UNARY_TABLE *, RAW_OBJ_COL *, OBJ (*)(void *, uint32), void *, STREAM *, STREAM *);
void   raw_obj_col_write(WRITE_FILE_STATE *, UNARY_TABLE *, RAW_OBJ_COL *, OBJ (*)(void *, uint32), void *, bool);

void   raw_obj_col_iter_init(UNARY_TABLE *, RAW_OBJ_COL *, RAW_OBJ_COL_ITER *);
bool   raw_obj_col_iter_is_out_of_range(RAW_OBJ_COL_ITER *);
uint32 raw_obj_col_iter_get_idx(RAW_OBJ_COL_ITER *);
OBJ    raw_obj_col_iter_get_value(RAW_OBJ_COL_ITER *);
void   raw_obj_col_iter_move_forward(RAW_OBJ_COL_ITER *);

////////////////////////////// raw-obj-col-aux.cpp /////////////////////////////

// void raw_obj_col_aux_clear(OBJ_COL_AUX *);
// void raw_obj_col_aux_delete_1(OBJ_COL_AUX *, uint32);
// void raw_obj_col_aux_insert(OBJ_COL_AUX *, uint32, OBJ);
// void raw_obj_col_aux_update(OBJ_COL_AUX *, uint32, OBJ);

bool raw_obj_col_aux_check_key_1(UNARY_TABLE *, RAW_OBJ_COL *, OBJ_COL_AUX *, STATE_MEM_POOL *);

void raw_obj_col_aux_apply(UNARY_TABLE *, UNARY_TABLE_AUX *, RAW_OBJ_COL *, OBJ_COL_AUX *, STATE_MEM_POOL *);

// bool raw_obj_col_aux_contains_1(RAW_OBJ_COL *, OBJ_COL_AUX *, uint32);
// OBJ  raw_obj_col_aux_lookup(RAW_OBJ_COL *, OBJ_COL_AUX *, uint32);

////////////////////////////////// int-col.cpp /////////////////////////////////

void int_col_init(INT_COL *column, STATE_MEM_POOL *);

uint32 int_col_size(INT_COL*);

bool int_col_contains_1(INT_COL *column, uint32 idx);
int64 int_col_lookup(INT_COL *column, uint32 idx);

void int_col_insert(INT_COL *column, uint32 idx, int64 value, STATE_MEM_POOL *);
void int_col_update(INT_COL *column, uint32 idx, int64 value, STATE_MEM_POOL *);
bool int_col_delete(INT_COL *column, uint32 idx, STATE_MEM_POOL *);
void int_col_clear(INT_COL *column, STATE_MEM_POOL *);

void int_col_copy_to(INT_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2);
void int_col_write(WRITE_FILE_STATE *write_state, INT_COL *col, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip);

void slave_int_col_copy_to(MASTER_BIN_TABLE *, INT_COL *, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3);
void slave_int_col_write(WRITE_FILE_STATE *, MASTER_BIN_TABLE *, INT_COL *, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, uint32 idx1, uint32 idx2, uint32 idx3);

void int_col_iter_init(INT_COL *column, INT_COL_ITER *iter);
bool int_col_iter_is_out_of_range(INT_COL_ITER *iter);
uint32 int_col_iter_get_idx(INT_COL_ITER *iter);
int64 int_col_iter_get_value(INT_COL_ITER *iter);
void int_col_iter_move_forward(INT_COL_ITER *iter);

//////////////////////////////// int-col-aux.cpp ///////////////////////////////

void int_col_aux_init(INT_COL_AUX *col_aux, STATE_MEM_POOL *);
void int_col_aux_reset(INT_COL_AUX *col_aux);

void int_col_aux_clear(INT_COL_AUX *col_aux);
void int_col_aux_delete_1(INT_COL_AUX *col_aux, uint32 index);
void int_col_aux_insert(INT_COL_AUX *col_aux, uint32 index, int64 value);
void int_col_aux_update(INT_COL_AUX *col_aux, uint32 index, int64 value);

void int_col_aux_apply(INT_COL *col, INT_COL_AUX *col_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *);
void int_col_aux_slave_apply(INT_COL *, INT_COL_AUX *, STATE_MEM_POOL *);

bool int_col_aux_check_key_1(INT_COL *col, INT_COL_AUX *col_aux, STATE_MEM_POOL *);

void int_col_aux_prepare(INT_COL_AUX *);
bool int_col_aux_contains_1(INT_COL *, INT_COL_AUX *, uint32);
bool int_col_aux_is_empty(INT_COL *, INT_COL_AUX *);
// int64 int_col_aux_lookup(INT_COL *, INT_COL_AUX *, uint32);

///////////////////////////////// float-col.cpp ////////////////////////////////

void float_col_init(FLOAT_COL *column, STATE_MEM_POOL *);

uint32 float_col_size(FLOAT_COL *);

bool float_col_contains_1(FLOAT_COL *column, uint32 idx);
double float_col_lookup(FLOAT_COL *column, uint32 idx);

void float_col_insert(FLOAT_COL *column, uint32 idx, double value, STATE_MEM_POOL *);
void float_col_update(FLOAT_COL *column, uint32 idx, double value, STATE_MEM_POOL *);
bool float_col_delete(FLOAT_COL *column, uint32 idx, STATE_MEM_POOL *);
void float_col_clear(FLOAT_COL *column, STATE_MEM_POOL *);

void float_col_copy_to(FLOAT_COL *, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2);
void float_col_write(WRITE_FILE_STATE *, FLOAT_COL *, OBJ (*surr_to_obj)(void *, uint32), void *store, bool flip);

void slave_float_col_copy_to(MASTER_BIN_TABLE *, FLOAT_COL *, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3);
void slave_float_col_write(WRITE_FILE_STATE *, MASTER_BIN_TABLE *, FLOAT_COL *, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, uint32 idx1, uint32 idx2, uint32 idx3);

void float_col_iter_init(FLOAT_COL *column, FLOAT_COL_ITER *iter);
bool float_col_iter_is_out_of_range(FLOAT_COL_ITER *iter);
uint32 float_col_iter_get_idx(FLOAT_COL_ITER *iter);
double float_col_iter_get_value(FLOAT_COL_ITER *iter);
void float_col_iter_move_forward(FLOAT_COL_ITER *iter);

/////////////////////////////// float-col-aux.cpp //////////////////////////////

void float_col_aux_init(FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *);
void float_col_aux_reset(FLOAT_COL_AUX *col_aux);

void float_col_aux_clear(FLOAT_COL_AUX *col_aux);
void float_col_aux_delete_1(FLOAT_COL_AUX *col_aux, uint32 index);
void float_col_aux_insert(FLOAT_COL_AUX *col_aux, uint32 index, double value);
void float_col_aux_update(FLOAT_COL_AUX *col_aux, uint32 index, double value);

void float_col_aux_apply(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *);
void float_col_aux_slave_apply(FLOAT_COL *, FLOAT_COL_AUX *, STATE_MEM_POOL *);

bool float_col_aux_check_key_1(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, STATE_MEM_POOL *);

void float_col_aux_prepare(FLOAT_COL_AUX *);
bool float_col_aux_contains_1(FLOAT_COL *, FLOAT_COL_AUX *, uint32);
bool float_col_aux_is_empty(FLOAT_COL *, FLOAT_COL_AUX *);
// double float_col_aux_lookup(FLOAT_COL *col, FLOAT_COL_AUX *col_aux, uint32 surr_1);

////////////////////////////////// obj-col.cpp /////////////////////////////////

void   obj_col_init(OBJ_COL *column, STATE_MEM_POOL *);

uint32 obj_col_size(OBJ_COL *);

bool   obj_col_contains_1(OBJ_COL *column, uint32 idx);
OBJ    obj_col_lookup(OBJ_COL *column, uint32 idx);

void   obj_col_insert(OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *);
void   obj_col_update(OBJ_COL *column, uint32 idx, OBJ value, STATE_MEM_POOL *);
bool   obj_col_delete(OBJ_COL *column, uint32 idx, STATE_MEM_POOL *);
void   obj_col_clear(OBJ_COL *column, STATE_MEM_POOL *);

void   obj_col_copy_to(OBJ_COL *, OBJ (*)(void *, uint32), void *, STREAM *, STREAM *);
void   obj_col_write(WRITE_FILE_STATE *, OBJ_COL *, OBJ (*)(void *, uint32), void *, bool);

void   slave_obj_col_copy_to(MASTER_BIN_TABLE *, OBJ_COL *, OBJ (*)(void *, uint32), void *, OBJ (*)(void *, uint32), void *, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3);
void   slave_obj_col_write(WRITE_FILE_STATE *, MASTER_BIN_TABLE *, OBJ_COL *, OBJ (*)(void *, uint32), void *, OBJ (*)(void *, uint32), void *, uint32 idx1, uint32 idx2, uint32 idx3);

void   obj_col_iter_init(OBJ_COL *column, OBJ_COL_ITER *iter);
bool   obj_col_iter_is_out_of_range(OBJ_COL_ITER *iter);
uint32 obj_col_iter_get_idx(OBJ_COL_ITER *iter);
OBJ    obj_col_iter_get_value(OBJ_COL_ITER *iter);
void   obj_col_iter_move_forward(OBJ_COL_ITER *iter);

//////////////////////////////// obj-col-aux.cpp ///////////////////////////////

void obj_col_aux_init(OBJ_COL_AUX *col_aux, STATE_MEM_POOL *);
void obj_col_aux_reset(OBJ_COL_AUX *col_aux);

void obj_col_aux_clear(OBJ_COL_AUX *col_aux);
void obj_col_aux_delete_1(OBJ_COL *, OBJ_COL_AUX *col_aux, uint32 index);
void obj_col_aux_insert(OBJ_COL_AUX *col_aux, uint32 index, OBJ value);
void obj_col_aux_update(OBJ_COL_AUX *col_aux, uint32 index, OBJ value);

bool obj_col_aux_check_key_1(OBJ_COL *, OBJ_COL_AUX *, STATE_MEM_POOL *);

void obj_col_aux_apply(OBJ_COL *col, OBJ_COL_AUX *col_aux, void (*)(void *, uint32), void (*)(void *, void *, uint32), void *, void *, STATE_MEM_POOL *);
void obj_col_aux_slave_apply(OBJ_COL *, OBJ_COL_AUX *, STATE_MEM_POOL *);

void obj_col_aux_prepare(OBJ_COL_AUX *);
bool obj_col_aux_contains_1(OBJ_COL *, OBJ_COL_AUX *, uint32);
bool obj_col_aux_is_empty(OBJ_COL *, OBJ_COL_AUX *);
// OBJ  obj_col_aux_lookup(OBJ_COL *, OBJ_COL_AUX *, uint32);

//////////////////////////////// int-store.cpp /////////////////////////////////

void int_store_init(INT_STORE *store, STATE_MEM_POOL *);

uint32 int_store_insert_or_add_ref(INT_STORE *store, STATE_MEM_POOL *, int64 value);

uint32 int_store_value_to_surr(INT_STORE *store, int64 value);
int64 int_store_surr_to_value(INT_STORE *store, uint32 surr);

OBJ int_store_surr_to_obj(void *, uint32);

////////////////////////////// int-store-aux.cpp ///////////////////////////////

void int_store_init_aux(INT_STORE_AUX *store_aux);

void int_store_mark_for_deferred_release(INT_STORE *store, INT_STORE_AUX *store_aux, uint32 surr);
void int_store_mark_for_batch_deferred_release(INT_STORE *store, INT_STORE_AUX *store_aux, uint32 surr, uint32 count);

void int_store_apply_deferred_releases(INT_STORE *store, INT_STORE_AUX *store_aux);

void int_store_apply(INT_STORE *store, INT_STORE_AUX *store_aux, STATE_MEM_POOL *);
void int_store_reset_aux(INT_STORE_AUX *store_aux);

uint32 int_store_value_to_surr(INT_STORE *store, INT_STORE_AUX *store_aux, OBJ value);
uint32 int_store_lookup_or_insert_value(INT_STORE *store, INT_STORE_AUX *store_aux, STATE_MEM_POOL *, int64 value);

void int_store_incr_rc(void *store, uint32 surr);
void int_store_decr_rc(void *store, void *store_aux, uint32 surr);

//////////////////////////////// obj-store.cpp /////////////////////////////////

void obj_store_init(OBJ_STORE *store, STATE_MEM_POOL *);

uint32 obj_store_insert_or_add_ref(OBJ_STORE *store, STATE_MEM_POOL *, OBJ value);

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ value);
OBJ obj_store_surr_to_value(OBJ_STORE *store, uint32 surr);

OBJ obj_store_surr_to_obj(void *, uint32);

////////////////////////////// obj-store-aux.cpp ///////////////////////////////

void obj_store_init_aux(OBJ_STORE_AUX *store_aux);

void obj_store_mark_for_deferred_release(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, uint32 surr);
void obj_store_mark_for_batch_deferred_release(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, uint32 surr, uint32 count);

void obj_store_apply_deferred_releases(OBJ_STORE *store, OBJ_STORE_AUX *store_aux);

void obj_store_apply(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, STATE_MEM_POOL *);
void obj_store_reset_aux(OBJ_STORE_AUX *store_aux);

uint32 obj_store_value_to_surr(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, OBJ value);
uint32 obj_store_lookup_or_insert_value(OBJ_STORE *store, OBJ_STORE_AUX *store_aux, STATE_MEM_POOL *, OBJ value);

void obj_store_incr_rc(void *store, uint32 surr);
void obj_store_decr_rc(void *store, void *store_aux, uint32 surr);

////////////////////////////////// queues.cpp //////////////////////////////////

void queue_u32_init(QUEUE_U32 *);
void queue_u32_insert(QUEUE_U32 *, uint32);
void queue_u32_sort_unique(QUEUE_U32 *);
bool queue_u32_sorted_contains(QUEUE_U32 *, uint32);
void queue_u32_prepare(QUEUE_U32 *);
void queue_u32_reset(QUEUE_U32 *);
bool queue_u32_contains(QUEUE_U32 *, uint32);
bool queue_u32_unique_count(QUEUE_U32 *);

void queue_u32_obj_init(QUEUE_U32_OBJ *);
void queue_u32_obj_insert(QUEUE_U32_OBJ *, uint32, OBJ);
void queue_u32_obj_reset(QUEUE_U32_OBJ *);
void queue_u32_obj_prepare(QUEUE_U32_OBJ *);
bool queue_u32_obj_contains_1(QUEUE_U32_OBJ *, uint32);

void queue_u32_double_init(QUEUE_U32_FLOAT *);
void queue_u32_double_insert(QUEUE_U32_FLOAT *, uint32, double);
void queue_u32_double_reset(QUEUE_U32_FLOAT *);
void queue_u32_double_prepare(QUEUE_U32_FLOAT *);
bool queue_u32_double_contains_1(QUEUE_U32_FLOAT *, uint32);

void queue_u32_i64_init(QUEUE_U32_I64 *);
void queue_u32_i64_insert(QUEUE_U32_I64 *, uint32, int64);
void queue_u32_i64_reset(QUEUE_U32_I64 *);
void queue_u32_i64_prepare(QUEUE_U32_I64 *);
bool queue_u32_i64_contains_1(QUEUE_U32_I64 *, uint32);

void queue_u64_init(QUEUE_U64 *);
void queue_u64_insert(QUEUE_U64 *, uint64);
void queue_u64_prepare(QUEUE_U64 *);
void queue_u64_sort_unique(QUEUE_U64 *);
bool queue_u64_contains(QUEUE_U64 *, uint64);
void queue_u64_flip_words(QUEUE_U64 *);
void queue_u64_reset(QUEUE_U64 *);
void queue_u64_prepare_1(QUEUE_U64 *);
bool queue_u64_contains_1(QUEUE_U64 *, uint32);
void queue_u64_prepare_2(QUEUE_U64 *);
bool queue_u64_contains_2(QUEUE_U64 *, uint32);
bool queue_u64_unique_count(QUEUE_U64 *);

void queue_2u32_insert(QUEUE_U32 *, uint32, uint32);

void queue_3u32_insert(QUEUE_U32 *, uint32, uint32, uint32);
void queue_3u32_prepare(QUEUE_U32 *);
void queue_3u32_sort_unique(QUEUE_U32 *);
void queue_3u32_permute_132(QUEUE_U32 *);
void queue_3u32_permute_231(QUEUE_U32 *);
bool queue_3u32_contains(QUEUE_U32 *, uint32, uint32, uint32);
bool queue_3u32_contains_12(QUEUE_U32 *, uint32, uint32);
bool queue_3u32_contains_1(QUEUE_U32 *, uint32);
bool queue_3u32_contains_2(QUEUE_U32 *, uint32);
bool queue_3u32_contains_3(QUEUE_U32 *, uint32);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "extern.h"
#include "mem-utils.h"
#include "basic-ops.h"
#include "seqs.h"
#include "instrs.h"
