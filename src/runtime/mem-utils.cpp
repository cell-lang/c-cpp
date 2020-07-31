#include "lib.h"
#include "extern.h"


const uint32 SEQ_BUFFER_FIELD_OFFSET = offsetof(SEQ_OBJ, buffer);

////////////////////////////////////////////////////////////////////////////////

#define MASK(S, W)      (((1ULL << (W)) - 1) << (S))
#define MAKE(V, S)      (((uint64) (V)) << (S))
#define GET(V, S, W)    (((V) & MASK((S), (W))) >> (S))
#define CLEAR(V, M)     ((V) & ~(M))

////////////////////////////////////////////////////////////////////////////////

const int TYPE_SHIFT              = 52;
const int TAGS_COUNT_SHIFT        = 57;
const int REPR_INFO_SHIFT         = 59;

const int TYPE_WIDTH              = 5;
const int TAGS_COUNT_WIDTH        = 2;
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

const uint64 TYPE_MASK            = MASK(TYPE_SHIFT, TYPE_WIDTH);
const uint64 TAGS_COUNT_MASK      = MASK(TAGS_COUNT_SHIFT, TAGS_COUNT_WIDTH);

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

const uint64 UINT8_ARRAY_MASK     = MAKE(INT_BITS_TAG_8,  INT_WIDTH_SHIFT);

const uint64 INT8_ARRAY_MASK      = MAKE(INT_BITS_TAG_8,  INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;
const uint64 INT16_ARRAY_MASK     = MAKE(INT_BITS_TAG_16, INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;
const uint64 INT32_ARRAY_MASK     = MAKE(INT_BITS_TAG_32, INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;
const uint64 INT64_ARRAY_MASK     = MAKE(INT_BITS_TAG_64, INT_WIDTH_SHIFT) | SIGNED_BIT_MASK;

/////////////////////// Physical representation of maps ////////////////////////

const int MAP_TYPE_SHIFT          = REPR_INFO_SHIFT;
const int MAP_TYPE_WIDTH          = 2;

const int ARRAY_MAP_TAG           = 0;  // Sorted array
const int BIN_TREE_MAP_TAG        = 1;  // Binary tree
const int AD_HOC_RECORD_TAG       = 2;  // Ad-hoc record

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

////////////////////////////////////////////////////////////////////////////////

void append_bits(uint64 word, int leftmost, int count, char *str) {
  assert(count > 0);
  assert(leftmost >= 0 & leftmost < 64);
  assert(leftmost - count + 1 >= 0);

  char *buff = str + strlen(str);

  for (int i=0 ; i < count ; i++)
    buff[i] = '0' + ((word >> (leftmost - i)) & 1);

  buff[count] = '\0';
}

void print_ref(OBJ obj) {
  static int frags[] = {5, 2, 5, 4, 16, 16, 16};
  static const char *type_names[] = {
    "TYPE_NOT_A_VALUE_OBJ",
    "TYPE_SYMBOL",
    "TYPE_INTEGER",
    "TYPE_FLOAT",
    "TYPE_EMPTY_REL",
    "TYPE_EMPTY_SEQ",
    "TYPE_NE_SEQ_UINT8_INLINE",
    "TYPE_NE_SEQ_INT16_INLINE",
    "TYPE_NE_SEQ_INT32_INLINE",
    "TYPE_NE_INT_SEQ",
    "TYPE_NE_FLOAT_SEQ",
    "TYPE_NE_BOOL_SEQ",
    "TYPE_NE_SEQ",
    "TYPE_NE_SET",
    "TYPE_NE_MAP",
    "TYPE_NE_BIN_REL",
    "TYPE_NE_TERN_REL",
    "TYPE_AD_HOC_TAG_REC",
    "TYPE_BOXED_OBJ"
  };

  char buffer[256];
  buffer[0] = '\0';

  int bit_idx = 63;
  for (int i=0 ; i < sizeof(frags)/sizeof(int) ; i++) {
    if (i > 0)
      strcat(buffer, " ");
    append_bits(obj.extra_data, bit_idx, frags[i], buffer);
    bit_idx -= frags[i];
  }

  assert(bit_idx == -1);

  OBJ_TYPE type = get_obj_type(obj);
  const char *type_name = type < sizeof(type_names)/sizeof(char*) ? type_names[type] : "INVALID_TYPE";

  printf("%s - (%s)\n", buffer, type_name);
  fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////

OBJ_TYPE get_obj_type(OBJ obj) {
  return (OBJ_TYPE) GET(obj.extra_data, TYPE_SHIFT, TYPE_WIDTH);
}

uint32 get_tags_count(OBJ obj) {
  return GET(obj.extra_data, TAGS_COUNT_SHIFT, TAGS_COUNT_WIDTH);
}

uint32 get_ex_type(OBJ obj) {
  return GET(obj.extra_data, TYPE_SHIFT, TYPE_WIDTH + TAGS_COUNT_WIDTH);
}

uint32 get_phys_repr_info(OBJ obj) {
  return obj.extra_data >> REPR_INFO_SHIFT;
}

////////////////////////////////////////////////////////////////////////////////

int get_seq_type(OBJ seq) {
#ifndef NDEBUG
  OBJ_TYPE type = get_obj_type(seq);
  assert(type == TYPE_NE_INT_SEQ | type == TYPE_NE_FLOAT_SEQ | type == TYPE_NE_BOOL_SEQ | type == TYPE_NE_SEQ);
#endif

  return GET(seq.extra_data, SEQ_TYPE_SHIFT, SEQ_TYPE_WIDTH);
}

INT_BITS_TAG get_int_bits_tag(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(INT_WIDTH_SHIFT + INT_WIDTH_WIDTH == 64);
  assert((seq.extra_data >> INT_WIDTH_SHIFT) < 4);

  return (INT_BITS_TAG) (seq.extra_data >> INT_WIDTH_SHIFT);
}

int get_map_type(OBJ map) {
  assert(get_obj_type(map) == TYPE_NE_MAP);
  return GET(map.extra_data, MAP_TYPE_SHIFT, MAP_TYPE_WIDTH);
}

bool is_array_map(OBJ obj) {
  return get_map_type(obj) == ARRAY_MAP_TAG;
}

bool is_bin_tree_map(OBJ obj) {
  return get_map_type(obj) == BIN_TREE_MAP_TAG;
}

////////////////////////////////////////////////////////////////////////////////

bool is_single_tag_type(OBJ_TYPE type) {
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

OBJ make_blank_obj() {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = 0;
  return obj;
}

OBJ make_symb(uint16 symb_id) {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = symb_id | SYMBOL_BASE_MASK;
  return obj;
}

OBJ make_bool(bool b) {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = (b ? symb_id_true : symb_id_false) | SYMBOL_BASE_MASK;
  return obj;
}

OBJ make_int(uint64 value) {
  OBJ obj;
  obj.core_data.int_ = value;
  obj.extra_data = INTEGER_MASK;
  return obj;
}

OBJ make_float(double value) {
  OBJ obj;
  obj.core_data.float_ = value;
  obj.extra_data = FLOAT_MASK;
  return obj;
}

OBJ make_empty_seq() {
  OBJ obj;
  obj.core_data.ptr = NULL;
  obj.extra_data = EMPTY_SEQ_MASK;
  return obj;
}

OBJ make_empty_rel() {
  OBJ obj;
  obj.core_data.ptr = NULL;
  obj.extra_data = EMPTY_REL_MASK;
  return obj;
}

OBJ make_seq_uint8_inline(uint64 data, uint32 length) {
  assert(length > 0 & length <= 8);

  assert(length == 8 || (data >> (8 * length) == 0));
  for (int i=length ; i < 8 ; i++)
    assert(inline_uint8_at(data, i) == 0);

  OBJ obj;
  obj.core_data.int_ = data;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_UINT8_INLINE_BASE_MASK;
  return obj;
}

OBJ make_seq_int16_inline(uint64 data, uint32 length) {
  assert(length > 0 & length <= 4);

  assert(length == 4 || (data >> (16 * length) == 0));
  for (int i=length ; i < 4 ; i++)
    assert(inline_int16_at(data, i) == 0);

  OBJ obj;
  obj.core_data.int_ = data;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_INT16_INLINE_BASE_MASK;
  return obj;
}

OBJ make_seq_int32_inline(uint64 data, uint32 length) {
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

OBJ make_seq_uint8(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.uint8_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | UINT8_ARRAY_MASK;
  return obj;
}

OBJ make_slice_uint8(uint8 *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | UINT8_ARRAY_MASK;
  return obj;
}

OBJ make_seq_int8(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.uint8_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT8_ARRAY_MASK;
  return obj;
}

OBJ make_slice_int8(int8 *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT8_ARRAY_MASK;
  return obj;
}

OBJ make_seq_int16(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 4);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int16_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT16_ARRAY_MASK;
  return obj;
}

OBJ make_slice_int16(int16 *ptr, uint32 length) {
  assert(ptr != NULL & length > 4);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT16_ARRAY_MASK;
  return obj;
}

OBJ make_seq_int32(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 2);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int32_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT32_ARRAY_MASK;
  return obj;
}

OBJ make_slice_int32(int32 *ptr, uint32 length) {
  assert(ptr != NULL & length > 2);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT32_ARRAY_MASK;
  return obj;
}

OBJ make_seq_int64(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int64_;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_OBJ_MASK | INT64_ARRAY_MASK;
  return obj;
}

OBJ make_slice_int64(int64 *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_INT_SEQ_BASE_MASK | ARRAY_SLICE_MASK | INT64_ARRAY_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_seq_float(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.float_;
  obj.extra_data = MAKE_LENGTH(length) | NE_FLOAT_SEQ_BASE_MASK | ARRAY_OBJ_MASK;
  return obj;
}

OBJ make_slice_float(double *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_FLOAT_SEQ_BASE_MASK | ARRAY_SLICE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_seq(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

#ifndef NDEBUG
  for (int i=0 ; i < length ; i++)
    assert(get_obj_type(ptr->buffer.obj[i]) != TYPE_NOT_A_VALUE_OBJ);
#endif

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.obj;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_BASE_MASK | ARRAY_OBJ_MASK;
  return obj;
}

OBJ make_slice(OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

#ifndef NDEBUG
  for (int i=0 ; i < length ; i++)
    assert(get_obj_type(ptr[i]) != TYPE_NOT_A_VALUE_OBJ);
#endif

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_BASE_MASK | ARRAY_SLICE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_set(SET_OBJ *ptr, uint32 size) {
  assert(ptr != NULL & size > 0);

#ifndef NDEBUG
  for (int i=0 ; i < size ; i++)
    assert(get_obj_type(ptr->buffer[i]) != TYPE_NOT_A_VALUE_OBJ);
#endif

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_SET_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_map(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

#ifndef NDEBUG
  for (int i=0 ; i < 2 * size ; i++)
    assert(get_obj_type(ptr->buffer[i]) != TYPE_NOT_A_VALUE_OBJ);
#endif

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_MAP_BASE_MASK | ARRAY_MAP_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_bin_rel(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

#ifndef NDEBUG
  for (int i=0 ; i < 2 * size ; i++)
    assert(get_obj_type(ptr->buffer[i]) != TYPE_NOT_A_VALUE_OBJ);
#endif

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_BIN_REL_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_tern_rel(TERN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

#ifndef NDEBUG
  for (int i=0 ; i < 3 * size ; i++)
    assert(get_obj_type(ptr->buffer[i]) != TYPE_NOT_A_VALUE_OBJ);
#endif

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_TERN_REL_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_opt_tag_rec(void *ptr, uint16 repr_id) {
  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_AD_HOC_REPR_ID(repr_id) | AD_HOC_TAG_REC_BASE_MASK;
  return obj;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_tag_obj_(uint16 tag_id, OBJ obj) {
  OBJ_TYPE type = get_obj_type(obj);
  uint8 tags_count = get_tags_count(obj);

  if (tags_count == 0) {
    int phys_repr = get_phys_repr_info(obj);
    if (phys_repr == AD_HOC_RECORD_TAG) {
      uint16 repr_id = get_opt_repr_id(obj);
      uint16 repr_tag_id = opt_repr_get_tag_id(repr_id);
      if (tag_id == repr_tag_id)
        return make_opt_tag_rec(get_opt_repr_ptr(obj), repr_id);
    }

    obj.extra_data |= MAKE_TAG(tag_id) | MAKE_TAGS_COUNT(1);
    return obj;
  }

  if (tags_count == 1 & !is_single_tag_type(type)) {
    uint16 curr_tag_id = GET(obj.extra_data, TAG_SHIFT, TAG_WIDTH);
    uint64 mask_diff = MAKE_INNER_TAG(curr_tag_id) | MAKE_TAG(tag_id) | MAKE_TAGS_COUNT(2);
    // No need to clear the inner tag here, it's already 0
    obj.extra_data = CLEAR(obj.extra_data, TAG_MASK | TAGS_COUNT_MASK) | mask_diff;
    return obj;
  }

  OBJ boxed_obj;
  BOXED_OBJ *ptr = new_boxed_obj();
  ptr->obj = obj;
  boxed_obj.core_data.ptr = ptr;
  boxed_obj.extra_data = BOXED_OBJ_BASE_MASK | MAKE_TAG(tag_id) | MAKE_TAGS_COUNT(1);
  return boxed_obj;
}


OBJ make_tag_obj(uint16 tag_id, OBJ obj) {
  OBJ tag_obj = make_tag_obj_(tag_id, obj);
  assert(get_tag_id(tag_obj) == tag_id);
  assert(are_shallow_eq(get_inner_obj(tag_obj), obj));
  return tag_obj;
}

////////////////////////////////////////////////////////////////////////////////

uint16 get_symb_id(OBJ obj) {
  assert(is_symb(obj));
  return GET(obj.extra_data, SYMB_IDX_SHIFT, SYMB_IDX_WIDTH);
}

bool get_bool(OBJ obj) {
  assert(is_bool(obj));
  return get_symb_id(obj) == symb_id_true;
}

int64 get_int(OBJ obj) {
  assert(is_int(obj));
  return obj.core_data.int_;
}

double get_float(OBJ obj) {
  assert(is_float(obj));
  return obj.core_data.float_;
}

uint32 read_size_field_unchecked(OBJ obj) {
  return GET(obj.extra_data, LENGTH_SHIFT, LENGTH_WIDTH);
}

uint32 read_size_field(OBJ obj) {
  assert(is_seq(obj) | is_set(obj) | is_bin_rel(obj) | is_tern_rel(obj));
  assert(get_phys_repr_info(obj) != AD_HOC_RECORD_TAG);
  return GET(obj.extra_data, LENGTH_SHIFT, LENGTH_WIDTH);
}

uint16 get_tag_id(OBJ obj) {
  assert(is_tag_obj(obj));
  assert(get_tags_count(obj) != 0 || get_obj_type(obj) == TYPE_AD_HOC_TAG_REC);

  if (get_tags_count(obj) != 0)
    return GET(obj.extra_data, TAG_SHIFT, TAG_WIDTH);
  else
    return opt_repr_get_tag_id(get_opt_repr_id(obj));
}

OBJ get_inner_obj(OBJ obj) {
  assert(is_tag_obj(obj));

  uint32 tags_count = get_tags_count(obj);

  if (tags_count == 1) {
    OBJ_TYPE type = get_obj_type(obj);
    if (type != TYPE_BOXED_OBJ) {
      obj.extra_data = CLEAR(obj.extra_data, TAG_MASK | TAGS_COUNT_MASK);
      return obj;
    }
    else
      return ((BOXED_OBJ *) obj.core_data.ptr)->obj;
  }
  else if (tags_count == 2) {
    uint16 inner_tag = GET(obj.extra_data, INNER_TAG_SHIFT, TAG_WIDTH);
    uint64 cleared_extra_data = CLEAR(obj.extra_data, INNER_TAG_MASK | TAG_MASK | TAGS_COUNT_MASK);
    obj.extra_data = cleared_extra_data | MAKE_TAG(inner_tag) | MAKE_TAGS_COUNT(1);
    return obj;
  }
  else {
    assert(tags_count == 0);
    assert(get_obj_type(obj) == TYPE_AD_HOC_TAG_REC);

    obj.extra_data = CLEAR(obj.extra_data, TYPE_MASK) | NE_MAP_BASE_MASK | AD_HOC_RECORD_MASK;
    return obj;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool is_array_obj(OBJ seq) {
  return get_seq_type(seq) == ARRAY_OBJ_TAG;
}

bool is_signed(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data & SIGNED_BIT_MASK) == SIGNED_BIT_MASK;
}

int is_8_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_8;
}

int is_16_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_16;
}

int is_32_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_32;
}

int is_64_bit_wide(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  return (seq.extra_data >> INT_WIDTH_SHIFT) == INT_BITS_TAG_64;
}

////////////////////////////////////////////////////////////////////////////////

uint8 *get_seq_elts_ptr_uint8(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(!is_signed(seq) & is_8_bit_wide(seq));
  return (uint8 *) seq.core_data.ptr;
}

int8 *get_seq_elts_ptr_int8(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_8_bit_wide(seq));
  return (int8 *) seq.core_data.ptr;
}

int16 *get_seq_elts_ptr_int16(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_16_bit_wide(seq));
  return (int16 *) seq.core_data.ptr;
}

int32 *get_seq_elts_ptr_int32(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_32_bit_wide(seq));
  return (int32 *) seq.core_data.ptr;
}

int64 *get_seq_elts_ptr_int64(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);
  assert(is_signed(seq) & is_64_bit_wide(seq));
  return (int64 *) seq.core_data.ptr;
}

double *get_seq_elts_ptr_float(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_FLOAT_SEQ);
  return (double *) seq.core_data.ptr;
}

OBJ *get_seq_elts_ptr(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_SEQ);
  return (OBJ *) seq.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_set_elts_ptr(OBJ set) {
  assert(is_ne_set(set));
  return (OBJ *) set.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ* get_seq_ptr(OBJ seq) {
  assert(is_array_obj(seq));
  return (SEQ_OBJ *) (((char *) seq.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET);
}

// SET_OBJ* get_set_ptr(OBJ obj) {
//   assert(get_obj_type(obj) == TYPE_NE_SET & obj.core_data.ptr != NULL);
//   return (SET_OBJ *) obj.core_data.ptr;
// }

BIN_REL_OBJ *get_bin_rel_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_NE_MAP | get_obj_type(obj) == TYPE_NE_BIN_REL);
  assert(obj.core_data.ptr != NULL);
  return (BIN_REL_OBJ *) obj.core_data.ptr;
}

TERN_REL_OBJ *get_tern_rel_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_NE_TERN_REL & obj.core_data.ptr != NULL);
  return (TERN_REL_OBJ *) obj.core_data.ptr;
}

BOXED_OBJ *get_boxed_obj_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_BOXED_OBJ & obj.core_data.ptr != NULL);
  return (BOXED_OBJ *) obj.core_data.ptr;
}

void* get_opt_tag_rec_ptr(OBJ obj) {
  assert(get_obj_type(obj) == TYPE_AD_HOC_TAG_REC);
  return obj.core_data.ptr;
}

void* get_opt_repr_ptr(OBJ obj) {
#ifndef NDEBUG
  OBJ_TYPE type = get_obj_type(obj);
  assert(type == TYPE_AD_HOC_TAG_REC || (type == TYPE_NE_MAP & get_map_type(obj) == AD_HOC_RECORD_TAG));
#endif

  return obj.core_data.ptr;
}

uint16 get_opt_repr_id(OBJ obj) {
  return GET(obj.extra_data, AD_HOC_REPR_ID_SHIFT, AD_HOC_REPR_ID_WIDTH);
}

////////////////////////////////////////////////////////////////////////////////

bool is_blank(OBJ obj) {
  return obj.extra_data == 0;
}

bool is_symb(OBJ obj) {
  assert(
    (get_ex_type(obj) == TYPE_SYMBOL)
      ==
    (get_tags_count(obj) == 0 & get_obj_type(obj) == TYPE_SYMBOL)
  );

  return get_ex_type(obj) == TYPE_SYMBOL;
}

bool is_bool(OBJ obj) {
  assert(symb_id_false == 0 & symb_id_true == 1); // The correctness of the body depends on this assumption
  return (obj.extra_data >> 1) == (SYMBOL_BASE_MASK >> 1);
}

bool is_int(OBJ obj) {
  return obj.extra_data == INTEGER_MASK;
}

bool is_float(OBJ obj) {
  return obj.extra_data == FLOAT_MASK;
}

bool is_seq(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type >= TYPE_EMPTY_SEQ & ex_type <= NE_SEQ_TYPE_RANGE_END;
}

bool is_empty_seq(OBJ obj) {
  return obj.extra_data == EMPTY_SEQ_MASK;
}

bool is_ne_seq(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type >= NE_SEQ_TYPE_RANGE_START & ex_type <= NE_SEQ_TYPE_RANGE_END;
}

bool is_empty_rel(OBJ obj) {
  return obj.extra_data == EMPTY_REL_MASK;
}

bool is_ne_set(OBJ obj) {
  assert(
    (get_ex_type(obj) == TYPE_NE_SET) ==
    ((get_obj_type(obj) == TYPE_NE_SET) & get_tags_count(obj) == 0)
  );
  return get_ex_type(obj) == TYPE_NE_SET;
}

bool is_set(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_SET | ex_type == TYPE_EMPTY_REL;
}

bool is_ne_bin_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_MAP | ex_type == TYPE_NE_BIN_REL;
}

bool is_ne_map(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_MAP;
}

bool is_bin_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_EMPTY_REL | ex_type == TYPE_NE_MAP | ex_type == TYPE_NE_BIN_REL;
}

bool is_ne_tern_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_TERN_REL;
}

bool is_tern_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_EMPTY_REL | ex_type == TYPE_NE_TERN_REL;
}

bool is_tag_obj(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_AD_HOC_TAG_REC | ex_type > MAX_OBJ_TYPE;
}

////////////////////////////////////////////////////////////////////////////////

bool is_symb(OBJ obj, uint16 symb_id) {
  return obj.extra_data == (symb_id | SYMBOL_BASE_MASK);
}

bool is_int(OBJ obj, int64 n) {
  return obj.core_data.int_ == n & obj.extra_data == INTEGER_MASK;
}

////////////////////////////////////////////////////////////////////////////////

bool is_opt_rec(OBJ obj) {
  return get_ex_type(obj) == TYPE_NE_MAP && get_map_type(obj) == AD_HOC_RECORD_TAG;
}

bool is_opt_rec_or_tag_rec(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return (get_ex_type(obj) == TYPE_NE_MAP && get_map_type(obj) == AD_HOC_RECORD_TAG) |
         ex_type == TYPE_AD_HOC_TAG_REC;
}

////////////////////////////////////////////////////////////////////////////////

bool is_inline_obj(OBJ obj) {
  return get_obj_type(obj) <= MAX_INLINE_OBJ_TYPE;
}

////////////////////////////////////////////////////////////////////////////////

void *get_ref_obj_ptr(OBJ obj) {
  OBJ_TYPE type = get_obj_type(obj);
  assert(type > MAX_INLINE_OBJ_TYPE & type <= MAX_OBJ_TYPE);

  if (type == TYPE_NE_SEQ)
    return ((char *) obj.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET;

  return obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) int64 intrl_cmp_obj_arrays(OBJ *elts1, OBJ *elts2, uint32 count) {
  for (int i=0 ; i < count ; i++) {
    int64 cr = intrl_cmp(elts1[i], elts2[i]);
    if (cr != 0)
      return cr;
  }
  return 0;
}

__attribute__ ((noinline)) int64 intrl_cmp_ne_int_seqs(OBJ obj1, OBJ obj2) {
  assert(read_size_field_unchecked(obj1) == read_size_field_unchecked(obj2));

  uint32 len = read_size_field_unchecked(obj1);

  for (uint32 i=0 ; i < len ; i++) {
    int64 elt1 = get_int_at_unchecked(obj1, i);
    int64 elt2 = get_int_at_unchecked(obj2, i);

    if (elt1 != elt2)
      return elt2 - elt1;
  }

  return 0;
}

__attribute__ ((noinline)) int64 intrl_cmp(OBJ obj1, OBJ obj2) {
  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  uint64 log_extra_data_1 = extra_data_1 << REPR_INFO_WIDTH;
  uint64 log_extra_data_2 = extra_data_2 << REPR_INFO_WIDTH;

  if (log_extra_data_1 != log_extra_data_2)
    return log_extra_data_2 - log_extra_data_1;

  OBJ_TYPE type = (OBJ_TYPE) ((log_extra_data_1 >> (REPR_INFO_WIDTH + TYPE_SHIFT)) & 0x1F);
  assert(type == get_obj_type(obj1) & type == get_obj_type(obj2));

  if (type <= MAX_INLINE_OBJ_TYPE)
    return obj2.core_data.int_ - obj1.core_data.int_;

  switch (type) {
    case TYPE_NE_INT_SEQ:
      return intrl_cmp_ne_int_seqs(obj1, obj2);

    case TYPE_NE_FLOAT_SEQ:
      int64 intrl_cmp_ne_float_seq(OBJ, OBJ);
      return intrl_cmp_ne_float_seq(obj1, obj2);

    case TYPE_NE_BOOL_SEQ:
      internal_fail();
      // return intrl_cmp_NE_BOOL_SEQ();

    case TYPE_NE_SEQ:
      return intrl_cmp_obj_arrays((OBJ *) obj1.core_data.ptr, (OBJ *) obj2.core_data.ptr, read_size_field_unchecked(obj1));

    case TYPE_NE_SET:
      return intrl_cmp_obj_arrays((OBJ *) obj1.core_data.ptr, (OBJ *) obj2.core_data.ptr, read_size_field_unchecked(obj1));

    case TYPE_NE_MAP:
      int64 intrl_cmp_ne_maps(OBJ obj1, OBJ obj2);
      return intrl_cmp_ne_maps(obj1, obj2);

    case TYPE_NE_BIN_REL:
      return intrl_cmp_obj_arrays(((BIN_REL_OBJ *) obj1.core_data.ptr)->buffer, ((BIN_REL_OBJ *) obj2.core_data.ptr)->buffer, 2 * read_size_field_unchecked(obj1));

    case TYPE_NE_TERN_REL:
      return intrl_cmp_obj_arrays(((TERN_REL_OBJ *) obj1.core_data.ptr)->buffer, ((TERN_REL_OBJ *) obj2.core_data.ptr)->buffer, 3 * read_size_field_unchecked(obj1));

    case TYPE_AD_HOC_TAG_REC:
      return opt_repr_cmp(obj1.core_data.ptr, obj2.core_data.ptr, get_opt_repr_id(obj1));

    case TYPE_BOXED_OBJ:
      return intrl_cmp(((BOXED_OBJ *) obj1.core_data.ptr)->obj, ((BOXED_OBJ *) obj2.core_data.ptr)->obj);

    default:
      internal_fail();
  }
}

////////////////////////////////////////////////////////////////////////////////

bool are_shallow_eq(OBJ obj1, OBJ obj2) {
  return obj1.core_data.int_ == obj2.core_data.int_ && obj1.extra_data == obj2.extra_data;
}

int64 shallow_cmp(OBJ obj1, OBJ obj2) {
  assert(is_inline_obj(obj1) & is_inline_obj(obj2));

  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  if (extra_data_1 != extra_data_2)
    return (extra_data_2 << REPR_INFO_WIDTH) - (extra_data_1 << REPR_INFO_WIDTH);
  else
    return obj2.core_data.int_ - obj1.core_data.int_;
}

//## REMOVE
int comp_floats(double x, double y) {
  uint64 n = *((uint64 *) &x);
  uint64 m = *((uint64 *) &y);

  if (n < m)
    return 1;

  if (n > m)
    return -1;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

OBJ repoint_to_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  obj.core_data.ptr = new_ptr;
  return obj;
}

OBJ repoint_to_sliced_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));

  obj.core_data.ptr = new_ptr;
  obj.extra_data = CLEAR(obj.extra_data, SEQ_TYPE_MASK) | ARRAY_SLICE_MASK;
  return obj;
}
