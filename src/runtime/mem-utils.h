const uint32 SEQ_BUFFER_FIELD_OFFSET = offsetof(SEQ_OBJ, buffer);

////////////////////////////////////////////////////////////////////////////////

#define MASK(S, W)      (((1ULL << (W)) - 1) << (S))
#define MAKE(V, S)      (((uint64) (V)) << (S))
#define GET(V, S, W)    (((V) & MASK((S), (W))) >> (S))
#define CLEAR(V, M)     ((V) & ~(M))

////////////////////////////////////////////////////////////////////////////////

const int TAGS_COUNT_SHIFT        = 52;
const int TYPE_SHIFT              = 54;
const int REPR_INFO_SHIFT         = 59;

const int TAGS_COUNT_WIDTH        = 2;
const int TYPE_WIDTH              = 5;
const int REPR_INFO_WIDTH         = 5;

const int SYMB_IDX_SHIFT          = 0;
const int LENGTH_SHIFT            = 0;
const int AD_HOC_REPR_ID_SHIFT    = 0;
const int INNER_TAG_SHIFT         = 16;
const int TAG_SHIFT               = 32;

const int SYMB_IDX_WIDTH          = 16;
const int LENGTH_WIDTH            = 32;
const int AD_HOC_REPR_ID_WIDTH    = 16;
const int TAG_WIDTH               = 16;

const uint64 TAGS_COUNT_MASK      = MASK(TAGS_COUNT_SHIFT, TAGS_COUNT_WIDTH);
const uint64 TYPE_MASK            = MASK(TYPE_SHIFT, TYPE_WIDTH);

const uint64 SYMB_IDX_MASK        = MASK(SYMB_IDX_SHIFT, SYMB_IDX_WIDTH);
const uint64 LENGTH_MASK          = MASK(LENGTH_SHIFT, LENGTH_WIDTH);
const uint64 AD_HOC_REPR_ID_MASK  = MASK(AD_HOC_REPR_ID_SHIFT, AD_HOC_REPR_ID_WIDTH);
const uint64 INNER_TAG_MASK       = MASK(INNER_TAG_SHIFT, TAG_WIDTH);
const uint64 TAG_MASK             = MASK(TAG_SHIFT, TAG_WIDTH);

////////////// Physical representation of (non-inline) sequences ///////////////

const int SEQ_TYPE_SHIFT          = REPR_INFO_SHIFT;
const int SEQ_TYPE_WIDTH          = 2;

const int ARRAY_OBJ_TAG           = 0;  // Full array object
const int ARRAY_SLICE_TAG         = 1;  // Array slice

const uint64 SEQ_TYPE_MASK        = MASK(SEQ_TYPE_SHIFT, SEQ_TYPE_WIDTH);

const uint64 ARRAY_OBJ_MASK       = MAKE(ARRAY_OBJ_TAG,   SEQ_TYPE_SHIFT);
const uint64 ARRAY_SLICE_MASK     = MAKE(ARRAY_SLICE_TAG, SEQ_TYPE_SHIFT);

// Only for sequences of integers
const int SIGNED_BIT_SHIFT        = SEQ_TYPE_SHIFT + SEQ_TYPE_WIDTH;

const uint64 SIGNED_BIT_MASK      = MAKE(1, SIGNED_BIT_SHIFT);

// Only for sequences of integers
const int INT_WIDTH_SHIFT         = SIGNED_BIT_SHIFT + 1;
const int INT_WIDTH_WIDTH         = 2;

const uint64 INT_WIDTH_MASK       = MASK(INT_WIDTH_SHIFT, INT_WIDTH_WIDTH);

const uint64 UINT8_ARRAY_MASK     = MAKE(INT_BITS_TAG_8,  INT_WIDTH_SHIFT);

const uint64 INT8_ARRAY_MASK      = MAKE(INT_BITS_TAG_8,  INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;
const uint64 INT16_ARRAY_MASK     = MAKE(INT_BITS_TAG_16, INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;
const uint64 INT32_ARRAY_MASK     = MAKE(INT_BITS_TAG_32, INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;
const uint64 INT64_ARRAY_MASK     = MAKE(INT_BITS_TAG_64, INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;

const uint64 SEQ_INFO_MASK        = TYPE_MASK | SEQ_TYPE_MASK | SIGNED_BIT_MASK | INT_WIDTH_MASK;

/////////////////////// Physical representation of sets ////////////////////////

const uint32 SET_TYPE_SHIFT       = REPR_INFO_SHIFT;
const uint32 SET_TYPE_WIDTH       = 1;

const uint64 SET_TYPE_MASK        = MASK(SET_TYPE_SHIFT, SEQ_TYPE_WIDTH);

const uint32 ARRAY_SET_TAG        = 0; // Sorted array
const uint32 BIN_TREE_SET_TAG     = 1; // Binary tree

const uint64 ARRAY_SET_MASK       = MAKE(ARRAY_SET_TAG,    SET_TYPE_SHIFT);
const uint64 BIN_TREE_SET_MASK    = MAKE(BIN_TREE_SET_TAG, SET_TYPE_SHIFT);

/////////////////////// Physical representation of maps ////////////////////////

const uint32 MAP_TYPE_SHIFT       = REPR_INFO_SHIFT;
const uint32 MAP_TYPE_WIDTH       = 2;

const uint64 MAP_TYPE_MASK        = MASK(MAP_TYPE_SHIFT, MAP_TYPE_WIDTH);

const uint64 ARRAY_MAP_TAG        = 0;  // Sorted array
const uint64 BIN_TREE_MAP_TAG     = 1;  // Binary tree
const uint64 AD_HOC_RECORD_TAG    = 2;  // Ad-hoc record

const uint64 ARRAY_MAP_MASK       = MAKE(ARRAY_MAP_TAG,     MAP_TYPE_SHIFT);
const uint64 BIN_TREE_MAP_MASK    = MAKE(BIN_TREE_MAP_TAG,  MAP_TYPE_SHIFT);
const uint64 AD_HOC_RECORD_MASK   = MAKE(AD_HOC_RECORD_TAG, MAP_TYPE_SHIFT);

////////////////////////////////////////////////////////////////////////////////

#define MAKE_TYPE(T)              MAKE(T, TYPE_SHIFT)
#define MAKE_LENGTH(L)            MAKE(L, LENGTH_SHIFT)
#define MAKE_AD_HOC_REPR_ID(I)    MAKE(I, AD_HOC_REPR_ID_SHIFT)
#define MAKE_TAGS_COUNT(C)        MAKE(C, TAGS_COUNT_SHIFT)
#define MAKE_INNER_TAG(T)         MAKE(T, INNER_TAG_SHIFT)
#define MAKE_SYMB_IDX(I)          MAKE(I, SYMB_IDX_SHIFT)
#define MAKE_TAG(T)               MAKE(T, TAG_SHIFT)

////////////////////////////////////////////////////////////////////////////////

const uint64 NOT_A_VALUE_OBJ_BASE_MASK      = MAKE_TYPE(TYPE_NOT_A_VALUE_OBJ);
const uint64 SYMBOL_BASE_MASK               = MAKE_TYPE(TYPE_SYMBOL);
const uint64 INTEGER_MASK                   = MAKE_TYPE(TYPE_INTEGER);
const uint64 FLOAT_MASK                     = MAKE_TYPE(TYPE_FLOAT);
const uint64 EMPTY_SEQ_MASK                 = MAKE_TYPE(TYPE_EMPTY_SEQ);
const uint64 EMPTY_REL_MASK                 = MAKE_TYPE(TYPE_EMPTY_REL);
const uint64 NE_SEQ_UINT8_INLINE_BASE_MASK  = MAKE_TYPE(TYPE_NE_SEQ_UINT8_INLINE);
const uint64 NE_SEQ_INT16_INLINE_BASE_MASK  = MAKE_TYPE(TYPE_NE_SEQ_INT16_INLINE);
const uint64 NE_SEQ_INT32_INLINE_BASE_MASK  = MAKE_TYPE(TYPE_NE_SEQ_INT32_INLINE);

const uint64 NE_INT_SEQ_BASE_MASK           = MAKE_TYPE(TYPE_NE_INT_SEQ);
const uint64 NE_FLOAT_SEQ_BASE_MASK         = MAKE_TYPE(TYPE_NE_FLOAT_SEQ);
// const uint64 NE_BOOL_SEQ_BASE_MASK          = MAKE_TYPE(TYPE_NE_BOOL_SEQ);
const uint64 NE_SEQ_BASE_MASK               = MAKE_TYPE(TYPE_NE_SEQ);
const uint64 NE_SET_BASE_MASK               = MAKE_TYPE(TYPE_NE_SET);
const uint64 NE_MAP_BASE_MASK               = MAKE_TYPE(TYPE_NE_MAP);
const uint64 NE_BIN_REL_BASE_MASK           = MAKE_TYPE(TYPE_NE_BIN_REL);
const uint64 NE_TERN_REL_BASE_MASK          = MAKE_TYPE(TYPE_NE_TERN_REL);
const uint64 AD_HOC_TAG_REC_BASE_MASK       = MAKE_TYPE(TYPE_AD_HOC_TAG_REC);
const uint64 BOXED_OBJ_BASE_MASK            = MAKE_TYPE(TYPE_BOXED_OBJ);

const uint64 EX_TYPE_SYMBOL               = TYPE_SYMBOL              << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_EMPTY_REL            = TYPE_EMPTY_REL           << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_EMPTY_SEQ            = TYPE_EMPTY_SEQ           << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_NE_SET               = TYPE_NE_SET              << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_NE_MAP               = TYPE_NE_MAP              << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_NE_BIN_REL           = TYPE_NE_BIN_REL          << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_NE_TERN_REL          = TYPE_NE_TERN_REL         << TAGS_COUNT_WIDTH;
const uint64 EX_TYPE_AD_HOC_TAG_REC       = TYPE_AD_HOC_TAG_REC      << TAGS_COUNT_WIDTH;

////////////////////////////////////////////////////////////////////////////////

const uint64 NULL_OBJ_CORE_DATA = 0xFFFFFFFFFFFFFFFF;

////////////////////////////////////////////////////////////////////////////////


inline OBJ_TYPE get_obj_type(OBJ obj) {
  return (OBJ_TYPE) GET(obj.extra_data, TYPE_SHIFT, TYPE_WIDTH);
}

inline uint32 get_tags_count(OBJ obj) {
  return GET(obj.extra_data, TAGS_COUNT_SHIFT, TAGS_COUNT_WIDTH);
}

inline uint32 get_ex_type(OBJ obj) {
  return GET(obj.extra_data, TAGS_COUNT_SHIFT, TYPE_WIDTH + TAGS_COUNT_WIDTH);
}

inline uint32 get_phys_repr_info(OBJ obj) {
  return obj.extra_data >> REPR_INFO_SHIFT;
}

////////////////////////////////////////////////////////////////////////////////

inline int get_seq_type(OBJ seq) {
#ifndef NDEBUG
  OBJ_TYPE type = get_obj_type(seq);
  assert(type == TYPE_NE_INT_SEQ | type == TYPE_NE_FLOAT_SEQ | type == TYPE_NE_BOOL_SEQ | type == TYPE_NE_SEQ);
#endif

  return GET(seq.extra_data, SEQ_TYPE_SHIFT, SEQ_TYPE_WIDTH);
}

inline INT_BITS_TAG get_int_bits_tag(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(INT_WIDTH_SHIFT + INT_WIDTH_WIDTH == 64);
  assert((seq.extra_data >> INT_WIDTH_SHIFT) < 4);

  return (INT_BITS_TAG) (seq.extra_data >> INT_WIDTH_SHIFT);
}

inline int get_set_type(OBJ set) {
  assert(get_obj_type(set) == TYPE_NE_SET);
  return GET(set.extra_data, SEQ_TYPE_SHIFT, SEQ_TYPE_WIDTH);
}

inline bool is_array_set(OBJ set) {
  return get_set_type(set) == ARRAY_SET_TAG;
}

inline bool is_mixed_repr_set(OBJ set) {
  return get_set_type(set) == BIN_TREE_SET_TAG;
}

inline int get_map_type(OBJ map) {
  assert(get_obj_type(map) == TYPE_NE_MAP);
  return GET(map.extra_data, MAP_TYPE_SHIFT, MAP_TYPE_WIDTH);
}

inline bool is_array_map(OBJ obj) {
  return get_map_type(obj) == ARRAY_MAP_TAG;
}

inline bool is_mixed_repr_map(OBJ obj) {
  return get_map_type(obj) == BIN_TREE_MAP_TAG;
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_single_tag_type(OBJ_TYPE type) {
  const uint32 BIT_MAP  = (1 << TYPE_NE_INT_SEQ) |
                          (1 << TYPE_NE_FLOAT_SEQ) |
                          (1 << TYPE_NE_BOOL_SEQ) |
                          (1 << TYPE_NE_SEQ) |
                          (1 << TYPE_NE_SET) |
                          (1 << TYPE_NE_MAP) |
                          (1 << TYPE_NE_BIN_REL) |
                          (1 << TYPE_NE_TERN_REL);

  return ((BIT_MAP >> type) & 1) == 1;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_blank_obj() {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = 0;
  return obj;
}

inline OBJ make_null_obj() {
  OBJ obj;
  obj.core_data.int_ = NULL_OBJ_CORE_DATA;
  obj.extra_data = 0;
  return obj;
}

inline OBJ make_symb(uint16 symb_id) {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = symb_id | SYMBOL_BASE_MASK;
  return obj;
}

inline OBJ make_bool(bool b) {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = (b ? symb_id_true : symb_id_false) | SYMBOL_BASE_MASK;
  return obj;
}

inline OBJ make_int(uint64 value) {
  OBJ obj;
  obj.core_data.int_ = value;
  obj.extra_data = INTEGER_MASK;
  return obj;
}

inline OBJ make_float(double value) {
  OBJ obj;
  obj.core_data.float_ = value;
  obj.extra_data = FLOAT_MASK;
  return obj;
}

inline OBJ make_empty_seq() {
  OBJ obj;
  obj.core_data.ptr = NULL;
  obj.extra_data = EMPTY_SEQ_MASK;
  return obj;
}

inline OBJ make_empty_rel() {
  OBJ obj;
  obj.core_data.ptr = NULL;
  obj.extra_data = EMPTY_REL_MASK;
  return obj;
}

inline OBJ make_seq_uint8_inline(uint64 data, uint32 length) {
  assert(length > 0 & length <= 8);

#ifndef NDEBUG
  assert(length == 8 || (data >> (8 * length) == 0));
  for (int i=length ; i < 8 ; i++)
    assert(inline_uint8_at(data, i) == 0);
#endif

  OBJ obj;
  obj.core_data.int_ = data;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_UINT8_INLINE_BASE_MASK;
  return obj;
}

inline OBJ make_seq_int16_inline(uint64 data, uint32 length) {
  assert(length > 0 & length <= 4);

  assert(length == 4 || (data >> (16 * length) == 0));
  for (int i=length ; i < 4 ; i++)
    assert(inline_int16_at(data, i) == 0);

  OBJ obj;
  obj.core_data.int_ = data;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_INT16_INLINE_BASE_MASK;
  return obj;
}

inline OBJ make_seq_int32_inline(uint64 data, uint32 length) {
  assert(length > 0 & length <= 2);

  assert(length == 2 || (data >> (32 * length) == 0));
  for (int i=length ; i < 2 ; i++)
    assert(inline_int32_at(data, i) == 0);

  OBJ obj;
  obj.core_data.int_ = data;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_INT32_INLINE_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_seq_uint8(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.uint8_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | UINT8_ARRAY_MASK;
  return obj;
}

inline OBJ make_slice_uint8(uint8 *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | UINT8_ARRAY_MASK;
  return obj;
}

inline OBJ make_seq_int8(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.uint8_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT8_ARRAY_MASK;
  return obj;
}

inline OBJ make_slice_int8(int8 *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT8_ARRAY_MASK;
  return obj;
}

inline OBJ make_seq_int16(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 4);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int16_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT16_ARRAY_MASK;
  return obj;
}

inline OBJ make_slice_int16(int16 *ptr, uint32 length) {
  assert(ptr != NULL & length > 4);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT16_ARRAY_MASK;
  return obj;
}

inline OBJ make_seq_int32(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 2);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int32_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT32_ARRAY_MASK;
  return obj;
}

inline OBJ make_slice_int32(int32 *ptr, uint32 length) {
  assert(ptr != NULL & length > 2);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT32_ARRAY_MASK;
  return obj;
}

inline OBJ make_seq_int64(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int64_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT64_ARRAY_MASK;
  return obj;
}

inline OBJ make_slice_int64(int64 *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT64_ARRAY_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_seq_float(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.float_;
  obj.extra_data = MAKE_LENGTH(length) | NE_FLOAT_SEQ_BASE_MASK | ARRAY_OBJ_MASK;
  return obj;
}

inline OBJ make_slice_float(double *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_FLOAT_SEQ_BASE_MASK | ARRAY_SLICE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_seq(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.obj;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_BASE_MASK | ARRAY_OBJ_MASK;
  return obj;
}

inline OBJ make_slice(OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_BASE_MASK | ARRAY_SLICE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_set(SET_OBJ *ptr, uint32 size) {
  assert(ptr != NULL & size > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_SET_BASE_MASK | ARRAY_SET_MASK;
  return obj;
}

inline OBJ make_mixed_repr_set(MIXED_REPR_SET_OBJ *ptr, uint32 size) {
  assert(ptr != NULL && size > 8);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_SET_BASE_MASK | BIN_TREE_SET_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_map(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_MAP_BASE_MASK | ARRAY_MAP_MASK;
  return obj;
}

inline OBJ make_mixed_repr_map(MIXED_REPR_MAP_OBJ *ptr, uint32 size) {
  assert(ptr != NULL && size > 6);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_MAP_BASE_MASK | BIN_TREE_MAP_MASK;
  assert(is_mixed_repr_map(obj));
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_bin_rel(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_BIN_REL_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_tern_rel(TERN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_TERN_REL_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_tag_int(uint16 tag_id, int64 value) {
  OBJ obj;
  obj.core_data.int_ = value;
  obj.extra_data = INTEGER_MASK | MAKE_TAG(tag_id) | MAKE_TAGS_COUNT(1);
  assert(get_tag_id(obj) == tag_id);
  assert(get_int(get_inner_obj(obj)) == value);
  assert(get_inner_long(obj) == value);
  return obj;
}

inline int64 get_inner_long(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_INTEGER);
  assert(get_tags_count(obj) == 1);
  return obj.core_data.int_;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ make_opt_tag_rec(void *ptr, uint16 repr_id) {
  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_AD_HOC_REPR_ID(repr_id) | AD_HOC_TAG_REC_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

inline uint16 get_symb_id(OBJ obj) {
  assert(is_symb(obj));
  return GET(obj.extra_data, SYMB_IDX_SHIFT, SYMB_IDX_WIDTH);
}

inline bool get_bool(OBJ obj) {
  assert(is_bool(obj));
  return get_symb_id(obj) == symb_id_true;
}

inline int64 get_int(OBJ obj) {
  assert(is_int(obj));
  return obj.core_data.int_;
}

inline double get_float(OBJ obj) {
  assert(is_float(obj));
  return obj.core_data.float_;
}

inline uint32 read_size_field_unchecked(OBJ obj) {
  return GET(obj.extra_data, LENGTH_SHIFT, LENGTH_WIDTH);
}

inline uint32 read_size_field(OBJ obj) {
  assert(is_seq(obj) | is_set(obj) | is_bin_rel(obj) | is_tern_rel(obj));
  assert(get_phys_repr_info(obj) != AD_HOC_RECORD_TAG);
  return GET(obj.extra_data, LENGTH_SHIFT, LENGTH_WIDTH);
}

inline uint16 get_tag_id(OBJ obj) {
  assert(is_tag_obj(obj));
  assert(get_tags_count(obj) != 0 || get_obj_type(obj) == TYPE_AD_HOC_TAG_REC);

  if (get_tags_count(obj) != 0)
    return GET(obj.extra_data, TAG_SHIFT, TAG_WIDTH);
  else
    return opt_repr_get_tag_id(get_opt_repr_id(obj));
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_array_obj(OBJ seq) {
  return get_seq_type(seq) == ARRAY_OBJ_TAG;
}

inline bool is_signed(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data & SIGNED_BIT_MASK) == SIGNED_BIT_MASK;
}

inline bool is_8_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_8;
}

inline bool is_16_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_16;
}

inline bool is_32_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_32;
}

inline bool is_64_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_64;
}

// Returns type + representation bits, with the sequence type bits (object vs slice) cleared
// Leaves only the object type, the sign bit and the element width bits
inline uint64 get_cleaned_up_seq_type_repr_bits(OBJ seq) {
  return (seq.extra_data >> TYPE_SHIFT) & ~(SEQ_TYPE_MASK >> TYPE_SHIFT);
}

inline bool is_u8_array(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ | get_obj_type(seq) == TYPE_NE_SEQ_UINT8_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT16_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT32_INLINE);
  bool it_is = get_cleaned_up_seq_type_repr_bits(seq) == ((NE_INT_SEQ_BASE_MASK | UINT8_ARRAY_MASK) >> TYPE_SHIFT);
  assert(it_is == (get_obj_type(seq) == TYPE_NE_INT_SEQ && !is_signed(seq) && is_8_bit_wide(seq)));
  return it_is;
}

inline bool is_i8_array(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ | get_obj_type(seq) == TYPE_NE_SEQ_UINT8_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT16_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT32_INLINE);
  bool it_is = get_cleaned_up_seq_type_repr_bits(seq) == ((NE_INT_SEQ_BASE_MASK | INT8_ARRAY_MASK) >> TYPE_SHIFT);
  assert(it_is == (get_obj_type(seq) == TYPE_NE_INT_SEQ && is_signed(seq) && is_8_bit_wide(seq)));
  return it_is;
}

inline bool is_i16_array(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ | get_obj_type(seq) == TYPE_NE_SEQ_UINT8_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT16_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT32_INLINE);
  bool it_is = get_cleaned_up_seq_type_repr_bits(seq) == ((NE_INT_SEQ_BASE_MASK | INT16_ARRAY_MASK) >> TYPE_SHIFT);
  assert(it_is == (get_obj_type(seq) == TYPE_NE_INT_SEQ && is_signed(seq) && is_16_bit_wide(seq)));
  return it_is;
}

inline bool is_i32_array(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ | get_obj_type(seq) == TYPE_NE_SEQ_UINT8_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT16_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT32_INLINE);
  bool it_is = get_cleaned_up_seq_type_repr_bits(seq) == ((NE_INT_SEQ_BASE_MASK | INT32_ARRAY_MASK) >> TYPE_SHIFT);
  assert(it_is == (get_obj_type(seq) == TYPE_NE_INT_SEQ && is_signed(seq) && is_32_bit_wide(seq)));
  return it_is;
}

inline bool is_i64_array(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ | get_obj_type(seq) == TYPE_NE_SEQ_UINT8_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT16_INLINE | get_obj_type(seq) == TYPE_NE_SEQ_INT32_INLINE);
  bool it_is = get_cleaned_up_seq_type_repr_bits(seq) == ((NE_INT_SEQ_BASE_MASK | INT64_ARRAY_MASK) >> TYPE_SHIFT);
  assert(it_is == (get_obj_type(seq) == TYPE_NE_INT_SEQ && is_signed(seq) && is_64_bit_wide(seq)));
  return it_is;
}

////////////////////////////////////////////////////////////////////////////////

inline uint8 *get_seq_elts_ptr_uint8(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(!is_signed(seq) & is_8_bit_wide(seq));
  return (uint8 *) seq.core_data.ptr;
}

inline int8 *get_seq_elts_ptr_int8(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_8_bit_wide(seq));
  return (int8 *) seq.core_data.ptr;
}

inline void *get_seq_elts_ptr_int8_or_uint8(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_8_bit_wide(seq));
  return seq.core_data.ptr;
}

inline int16 *get_seq_elts_ptr_int16(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_16_bit_wide(seq));
  return (int16 *) seq.core_data.ptr;
}

inline int32 *get_seq_elts_ptr_int32(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_32_bit_wide(seq));
  return (int32 *) seq.core_data.ptr;
}

inline int64 *get_seq_elts_ptr_int64(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_64_bit_wide(seq));
  return (int64 *) seq.core_data.ptr;
}

inline double *get_seq_elts_ptr_float(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_FLOAT_SEQ);
  return (double *) seq.core_data.ptr;
}

inline OBJ *get_seq_elts_ptr(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_SEQ);
  return (OBJ *) seq.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ *get_set_elts_ptr(OBJ set) {
  assert(is_ne_set(set) && is_array_set(set));
  return (OBJ *) set.core_data.ptr;
}

inline MIXED_REPR_SET_OBJ *get_mixed_repr_set_ptr(OBJ set) {
  assert(is_ne_set(set) && is_mixed_repr_set(set));
  return (MIXED_REPR_SET_OBJ *) set.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

inline SEQ_OBJ* get_seq_ptr(OBJ seq) {
  assert(is_array_obj(seq));
  return (SEQ_OBJ *) (((char *) seq.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET);
}

// SET_OBJ* get_set_ptr(OBJ obj) {
//   assert(get_obj_type(obj) == TYPE_NE_SET & obj.core_data.ptr != NULL);
//   return (SET_OBJ *) obj.core_data.ptr;
// }

inline BIN_REL_OBJ *get_bin_rel_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_NE_BIN_REL || (get_obj_type(obj) == TYPE_NE_MAP && is_array_map(obj)));
  assert(obj.core_data.ptr != NULL);
  return (BIN_REL_OBJ *) obj.core_data.ptr;
}

inline MIXED_REPR_MAP_OBJ *get_mixed_repr_map_ptr(OBJ map) {
  assert(is_ne_map(map) && is_mixed_repr_map(map));
  return (MIXED_REPR_MAP_OBJ *) map.core_data.ptr;
}

inline TERN_REL_OBJ *get_tern_rel_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_NE_TERN_REL & obj.core_data.ptr != NULL);
  return (TERN_REL_OBJ *) obj.core_data.ptr;
}

inline BOXED_OBJ *get_boxed_obj_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_BOXED_OBJ & obj.core_data.ptr != NULL);
  return (BOXED_OBJ *) obj.core_data.ptr;
}

inline void* get_opt_tag_rec_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_AD_HOC_TAG_REC);
  return obj.core_data.ptr;
}

inline void* get_opt_repr_ptr(OBJ obj) {
#ifndef NDEBUG
  OBJ_TYPE type = get_obj_type(obj);
  assert(type == TYPE_AD_HOC_TAG_REC || (type == TYPE_NE_MAP & get_map_type(obj) == AD_HOC_RECORD_TAG));
#endif

  return obj.core_data.ptr;
}

inline uint16 get_opt_repr_id(OBJ obj) {
  return GET(obj.extra_data, AD_HOC_REPR_ID_SHIFT, AD_HOC_REPR_ID_WIDTH);
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_blank(OBJ obj) {
  return obj.extra_data == 0 && obj.core_data.int_ == 0;
}

inline bool is_null(OBJ obj) {
  return obj.extra_data == 0 && obj.core_data.int_ == NULL_OBJ_CORE_DATA;
}

inline bool is_symb(OBJ obj) {
  assert((get_ex_type(obj) == EX_TYPE_SYMBOL) == (get_tags_count(obj) == 0 & get_obj_type(obj) == TYPE_SYMBOL));
  return get_ex_type(obj) == EX_TYPE_SYMBOL;
}

inline bool is_bool(OBJ obj) {
  assert(symb_id_false == 0 & symb_id_true == 1); // The correctness of the body depends on this assumption
  return (obj.extra_data >> 1) == (SYMBOL_BASE_MASK >> 1);
}

inline bool is_int(OBJ obj) {
  return obj.extra_data == INTEGER_MASK;
}

inline bool is_float(OBJ obj) {
  return obj.extra_data == FLOAT_MASK;
}

inline bool is_seq(OBJ obj) {
  uint32 type = get_obj_type(obj);
  return type >= TYPE_EMPTY_SEQ & type <= NE_SEQ_TYPE_RANGE_END & get_tags_count(obj) == 0;
}

inline bool is_empty_seq(OBJ obj) {
  return obj.extra_data == EMPTY_SEQ_MASK;
}

inline bool is_ne_seq(OBJ obj) {
  uint32 type = get_obj_type(obj);
  return type >= NE_SEQ_TYPE_RANGE_START & type <= NE_SEQ_TYPE_RANGE_END & get_tags_count(obj) == 0;
}

inline bool is_ne_int_seq(OBJ obj) {
  uint32 type = get_obj_type(obj);
  return type >= NE_INT_SEQ_TYPE_RANGE_START & type <= NE_INT_SEQ_TYPE_RANGE_END & get_tags_count(obj) == 0;
}

inline bool is_ne_float_seq(OBJ obj) {
  return get_tags_count(obj) == 0 & get_obj_type(obj) == TYPE_NE_FLOAT_SEQ;
}

inline bool is_empty_rel(OBJ obj) {
  return obj.extra_data == EMPTY_REL_MASK;
}

inline bool is_ne_set(OBJ obj) {
  assert((get_ex_type(obj) == EX_TYPE_NE_SET) == ((get_obj_type(obj) == TYPE_NE_SET) & get_tags_count(obj) == 0));
  return get_ex_type(obj) == EX_TYPE_NE_SET;
}

inline bool is_set(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == EX_TYPE_NE_SET | ex_type == EX_TYPE_EMPTY_REL;
}

inline bool is_ne_bin_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == EX_TYPE_NE_MAP | ex_type == EX_TYPE_NE_BIN_REL;
}

inline bool is_ne_map(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == EX_TYPE_NE_MAP;
}

inline bool is_bin_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == EX_TYPE_EMPTY_REL | ex_type == EX_TYPE_NE_MAP | ex_type == EX_TYPE_NE_BIN_REL;
}

inline bool is_ne_tern_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == EX_TYPE_NE_TERN_REL;
}

inline bool is_tern_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == EX_TYPE_EMPTY_REL | ex_type == EX_TYPE_NE_TERN_REL;
}

inline bool is_tag_obj(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return get_obj_type(obj) == TYPE_AD_HOC_TAG_REC | get_tags_count(obj) != 0;
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_symb(OBJ obj, uint16 symb_id) {
  return obj.extra_data == (symb_id | SYMBOL_BASE_MASK);
}

inline bool is_int(OBJ obj, int64 n) {
  return obj.core_data.int_ == n & obj.extra_data == INTEGER_MASK;
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_opt_rec(OBJ obj) {
  return get_ex_type(obj) == EX_TYPE_NE_MAP && get_map_type(obj) == AD_HOC_RECORD_TAG;
}

inline bool is_opt_rec_or_tag_rec(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return (ex_type == EX_TYPE_NE_MAP && get_map_type(obj) == AD_HOC_RECORD_TAG) |
         ex_type == EX_TYPE_AD_HOC_TAG_REC;
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_inline_obj(OBJ obj) {
  return get_obj_type(obj) <= MAX_INLINE_OBJ_TYPE;
}

////////////////////////////////////////////////////////////////////////////////

inline void *get_ref_obj_ptr(OBJ obj) {
  OBJ_TYPE type = get_obj_type(obj);
  assert(type > MAX_INLINE_OBJ_TYPE & type <= TYPE_BOXED_OBJ);

  if (type == TYPE_NE_SEQ)
    return ((char *) obj.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET;

  return obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

// obj1 < obj2  ->  1
// obj1 = obj2  ->  0
// obj1 > obj2  -> -1

inline int comp_objs_(OBJ obj1, OBJ obj2) {
  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  uint64 log_extra_data_1 = extra_data_1 << REPR_INFO_WIDTH;
  uint64 log_extra_data_2 = extra_data_2 << REPR_INFO_WIDTH;

  if (log_extra_data_1 != log_extra_data_2)
    return log_extra_data_1 < log_extra_data_2 ? 1 : -1;

  OBJ_TYPE type = (OBJ_TYPE) (log_extra_data_1 >> (REPR_INFO_WIDTH + TYPE_SHIFT));
  assert(type == get_obj_type(obj1) & type == get_obj_type(obj2));

  int64 core_data_1 = obj1.core_data.int_;
  int64 core_data_2 = obj2.core_data.int_;

  assert((core_data_1 != core_data_2) | (extra_data_1 == extra_data_2));

  if (core_data_1 == core_data_2)
    return 0;

  if (type <= MAX_INLINE_OBJ_TYPE)
    return core_data_1 < core_data_2 ? 1 : -1;

  extern int (*intrl_cmp_disp_table[])(OBJ, OBJ);
  return intrl_cmp_disp_table[type - MAX_INLINE_OBJ_TYPE - 1](obj1, obj2);

  // int intrl_cmp_non_inline(OBJ_TYPE type, OBJ obj1, OBJ obj2);
  // return intrl_cmp_non_inline(type, obj1, obj2);
}

inline int comp_objs(OBJ obj1, OBJ obj2) {
  int cr = comp_objs_(obj1, obj2);
  assert(!(is_inline_obj(obj1) & !is_inline_obj(obj2)) || cr == 1);
  assert(!(!is_inline_obj(obj1) & is_inline_obj(obj2)) || cr == -1);
  return cr;
}

inline int intrl_cmp_ad_hoc_type_fields(OBJ obj1, OBJ obj2) {
  return intrl_cmp(obj1, obj2);
}

inline bool are_eq_(OBJ obj1, OBJ obj2) {
  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  uint64 log_extra_data_1 = extra_data_1 << REPR_INFO_WIDTH;
  uint64 log_extra_data_2 = extra_data_2 << REPR_INFO_WIDTH;

  if (log_extra_data_1 != log_extra_data_2)
    return false;

  if (obj1.core_data.int_ == obj2.core_data.int_)
    return true;

  OBJ_TYPE type = (OBJ_TYPE) (log_extra_data_1 >> (REPR_INFO_WIDTH + TYPE_SHIFT));
  assert(type == get_obj_type(obj1) & type == get_obj_type(obj2));

  if (type <= MAX_INLINE_OBJ_TYPE)
    return false;

  extern int (*intrl_cmp_disp_table[])(OBJ, OBJ);
  return intrl_cmp_disp_table[type - MAX_INLINE_OBJ_TYPE - 1](obj1, obj2) == 0;
}

inline bool are_eq(OBJ obj1, OBJ obj2) {
  bool res = are_eq_(obj1, obj2);
  assert(res == (intrl_cmp(obj1, obj2) == 0));
  return res;
}

////////////////////////////////////////////////////////////////////////////////

inline bool are_shallow_eq(OBJ obj1, OBJ obj2) {
  return obj1.core_data.int_ == obj2.core_data.int_ && obj1.extra_data == obj2.extra_data;
}

inline int shallow_cmp_(OBJ obj1, OBJ obj2) {
  assert(is_inline_obj(obj1) & is_inline_obj(obj2));

  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  if (extra_data_1 != extra_data_2)
    return (extra_data_1 << REPR_INFO_WIDTH) < (extra_data_2 << REPR_INFO_WIDTH) ? 1 : -1;

  int64 core_data_1 = obj1.core_data.int_;
  int64 core_data_2 = obj2.core_data.int_;

  if (core_data_1 != core_data_2)
    return core_data_1 < core_data_2 ? 1 : -1;

  return 0;
}

inline int shallow_cmp(OBJ obj1, OBJ obj2) {
  int res = shallow_cmp_(obj1, obj2);
  assert(res == intrl_cmp(obj1, obj2));
  return res;
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ repoint_to_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  obj.core_data.ptr = new_ptr;
  return obj;
}

inline OBJ repoint_to_array_set_copy(OBJ obj, SET_OBJ *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SET_TYPE_MASK) | ARRAY_SET_MASK;
  return new_obj;
}

inline OBJ repoint_to_array_map_copy(OBJ obj, BIN_REL_OBJ *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, MAP_TYPE_MASK) | ARRAY_MAP_MASK;
  return new_obj;
}

inline OBJ repoint_to_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_TYPE_MASK) | ARRAY_SLICE_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}

inline OBJ repoint_to_uint8_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_INFO_MASK) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | UINT8_ARRAY_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}

inline OBJ repoint_to_int8_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_INFO_MASK) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT8_ARRAY_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}

inline OBJ repoint_to_int16_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_INFO_MASK) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT16_ARRAY_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}

inline OBJ repoint_to_int32_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_INFO_MASK) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT32_ARRAY_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}

inline OBJ repoint_to_int64_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_INFO_MASK) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT64_ARRAY_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}

inline OBJ repoint_to_float_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  OBJ new_obj;
  new_obj.core_data.ptr = new_ptr;
  new_obj.extra_data = CLEAR(obj.extra_data, SEQ_INFO_MASK) | NE_FLOAT_SEQ_BASE_MASK | ARRAY_SLICE_MASK;

  assert(are_eq(new_obj, obj));

  return new_obj;
}
