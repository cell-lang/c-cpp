#include "lib.h"
#include "extern.h"


const uint32 SEQ_BUFFER_FIELD_OFFSET = offsetof(SEQ_OBJ, buffer);

////////////////////////////////////////////////////////////////////////////////

#define MASK(S, W)      (((1ULL << (W)) - 1) << (S))
#define MAKE(V, S)      (((uint64) (V)) << (S))
#define GET(V, S, W)    (((V) & MASK((S), (W))) >> (S))
#define CLEAR(V, M)     ((V) & ~(M))

////////////////////////////////////////////////////////////////////////////////

const int TYPE_SHIFT          = 56;
const int TAGS_COUNT_SHIFT    = 62;

const int TYPE_WIDTH          = 6;
const int TAGS_COUNT_WIDTH    = 2;

const int SYMB_IDX_SHIFT      = 0;
const int LENGTH_SHIFT        = 0;
const int OPT_REPR_ID_SHIFT   = 0;
const int INNER_TAG_SHIFT     = 16;
const int TAG_SHIFT           = 32;

const int SYMB_IDX_WIDTH      = 16;
const int LENGTH_WIDTH        = 32;
const int OPT_REPR_ID_WIDTH   = 16;
const int TAG_WIDTH           = 16;

const uint64 TYPE_MASK        = MASK(TYPE_SHIFT, TYPE_WIDTH);
const uint64 TAGS_COUNT_MASK  = MASK(TAGS_COUNT_SHIFT, TAGS_COUNT_WIDTH);

const uint64 SYMB_IDX_MASK    = MASK(SYMB_IDX_SHIFT, SYMB_IDX_WIDTH);
const uint64 LENGTH_MASK      = MASK(LENGTH_SHIFT, LENGTH_WIDTH);
const uint64 OPT_REPR_ID_MASK = MASK(OPT_REPR_ID_SHIFT, OPT_REPR_ID_WIDTH);
const uint64 INNER_TAG_MASK   = MASK(INNER_TAG_SHIFT, TAG_WIDTH);
const uint64 TAG_MASK         = MASK(TAG_SHIFT, TAG_WIDTH);

////////////////////////////////////////////////////////////////////////////////

#define MAKE_TYPE(T)        MAKE(T, TYPE_SHIFT)
#define MAKE_LENGTH(L)      MAKE(L, LENGTH_SHIFT)
#define MAKE_OPT_REPR_ID(I) MAKE(I, OPT_REPR_ID_SHIFT)
#define MAKE_TAGS_COUNT(C)  MAKE(C, TAGS_COUNT_SHIFT)
#define MAKE_INNER_TAG(T)   MAKE(T, INNER_TAG_SHIFT)
#define MAKE_SYMB_IDX(I)    MAKE(I, SYMB_IDX_SHIFT)
#define MAKE_TAG(T)         MAKE(T, TAG_SHIFT)

////////////////////////////////////////////////////////////////////////////////

const uint64 BLANK_OBJ_MASK                 = MAKE_TYPE(TYPE_BLANK_OBJ);
const uint64 NULL_OBJ_MASK                  = MAKE_TYPE(TYPE_NULL_OBJ);
const uint64 SYMBOL_BASE_MASK               = MAKE_TYPE(TYPE_SYMBOL);
const uint64 INTEGER_MASK                   = MAKE_TYPE(TYPE_INTEGER);
const uint64 FLOAT_MASK                     = MAKE_TYPE(TYPE_FLOAT);
const uint64 EMPTY_SEQ_MASK                 = MAKE_TYPE(TYPE_EMPTY_SEQ);
const uint64 EMPTY_REL_MASK                 = MAKE_TYPE(TYPE_EMPTY_REL);
const uint64 NE_SEQ_BASE_MASK               = MAKE_TYPE(TYPE_NE_SEQ);
const uint64 NE_SLICE_BASE_MASK             = MAKE_TYPE(TYPE_NE_SLICE);
const uint64 NE_SET_BASE_MASK               = MAKE_TYPE(TYPE_NE_SET);
const uint64 NE_BIN_REL_BASE_MASK           = MAKE_TYPE(TYPE_NE_BIN_REL);
const uint64 NE_MAP_BASE_MASK               = MAKE_TYPE(TYPE_NE_MAP);
const uint64 NE_LOG_MAP_BASE_MASK           = MAKE_TYPE(TYPE_NE_LOG_MAP);
const uint64 NE_TERN_REL_BASE_MASK          = MAKE_TYPE(TYPE_NE_TERN_REL);
const uint64 TAG_OBJ_BASE_MASK              = MAKE_TYPE(TYPE_TAG_OBJ);
const uint64 OPT_REC_BASE_MASK              = MAKE_TYPE(TYPE_OPT_REC);
const uint64 OPT_TAG_REC_BASE_MASK          = MAKE_TYPE(TYPE_OPT_TAG_REC);
const uint64 NE_SEQ_UINT8_BASE_MASK         = MAKE_TYPE(TYPE_NE_SEQ_UINT8);
const uint64 NE_SLICE_UINT8_BASE_MASK       = MAKE_TYPE(TYPE_NE_SLICE_UINT8);
const uint64 NE_SEQ_INT16_BASE_MASK         = MAKE_TYPE(TYPE_NE_SEQ_INT16);
const uint64 NE_SLICE_INT16_BASE_MASK       = MAKE_TYPE(TYPE_NE_SLICE_INT16);
const uint64 NE_SEQ_UINT8_INLINE_BASE_MASK  = MAKE_TYPE(TYPE_NE_SEQ_UINT8_INLINE);
const uint64 NE_SEQ_INT16_INLINE_BASE_MASK  = MAKE_TYPE(TYPE_NE_SEQ_INT16_INLINE);

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
  static int frags[] = {2, 6, 8, 16, 16, 16};
  static const char *type_names[] = {
    "TYPE_BLANK_OBJ",
    "TYPE_NULL_OBJ",
    "TYPE_SYMBOL",
    "TYPE_INTEGER",
    "TYPE_FLOAT",
    "TYPE_EMPTY_SEQ",
    "TYPE_EMPTY_REL",
    "TYPE_NE_SEQ",
    "TYPE_NE_SET",
    "TYPE_NE_BIN_REL",
    "TYPE_NE_TERN_REL",
    "TYPE_TAG_OBJ",
    "TYPE_NE_SLICE",
    "TYPE_NE_MAP",
    "TYPE_NE_LOG_MAP",
    "TYPE_OPT_REC",
    "TYPE_OPT_TAG_REC",
    "TYPE_NE_SEQ_UINT8",
    "TYPE_NE_SLICE_UINT8",
    "TYPE_NE_SEQ_INT16",
    "TYPE_NE_SLICE_INT16",
    "TYPE_NE_SEQ_UINT8_INLINE",
    "TYPE_NE_SEQ_INT16_INLINE"
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

  OBJ_TYPE phys_type = get_physical_type(obj);
  const char *type_name = phys_type < sizeof(type_names)/sizeof(char*) ? type_names[phys_type] : "INVALID_TYPE";

  printf("%s - (%s)\n", buffer, type_name);
  fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////

OBJ_TYPE get_physical_type(OBJ obj) {
  return (OBJ_TYPE) GET(obj.extra_data, TYPE_SHIFT, TYPE_WIDTH);
}

uint32 get_tags_count(OBJ obj) {
  return obj.extra_data >> TAGS_COUNT_SHIFT;
}

uint32 get_ex_type(OBJ obj) {
  return obj.extra_data >> TYPE_SHIFT;
}

////////////////////////////////////////////////////////////////////////////////

OBJ_TYPE physical_to_logical_type(OBJ_TYPE type) {
  static OBJ_TYPE log_types[] = {
    TYPE_NE_SEQ,      // TYPE_NE_SLICE
    TYPE_NE_BIN_REL,  // TYPE_NE_MAP
    TYPE_NE_BIN_REL,  // TYPE_NE_LOG_MAP
    TYPE_NE_BIN_REL,  // TYPE_OPT_REC
    TYPE_TAG_OBJ,     // TYPE_OPT_TAG_REC
    TYPE_NE_SEQ,      // TYPE_NE_SEQ_UINT8
    TYPE_NE_SEQ,      // TYPE_NE_SLICE_UINT8
    TYPE_NE_SEQ,      // TYPE_NE_SEQ_INT16
    TYPE_NE_SEQ,      // TYPE_NE_SLICE_INT16
    TYPE_NE_SEQ,      // TYPE_NE_SEQ_UINT8_INLINE
    TYPE_NE_SEQ       // TYPE_NE_SEQ_INT16_INLINE
  };

  if (type <= MAX_LOGICAL_TYPE)
    return type;

  int idx = type - MAX_LOGICAL_TYPE - 1;
  assert(idx < sizeof(log_types) / sizeof(OBJ_TYPE));
  return log_types[idx];
}

OBJ_TYPE get_logical_type(OBJ obj) {
  return get_tags_count(obj) != 0 ? TYPE_TAG_OBJ : physical_to_logical_type(get_physical_type(obj));
}

////////////////////////////////////////////////////////////////////////////////

bool is_single_tag_type(OBJ_TYPE type) {
  const uint32 BIT_MAP =
    (1 << TYPE_NE_SEQ) |
    (1 << TYPE_NE_SET) |
    (1 << TYPE_NE_BIN_REL) |
    (1 << TYPE_NE_TERN_REL)|
    (1 << TYPE_NE_MAP) |
    (1 << TYPE_NE_LOG_MAP) |
    (1 << TYPE_NE_SLICE) |
    (1 << TYPE_NE_SEQ_UINT8) |
    (1 << TYPE_NE_SLICE_UINT8) |
    (1 << TYPE_NE_SEQ_INT16) |
    (1 << TYPE_NE_SLICE_INT16) |
    (1 << TYPE_NE_SEQ_UINT8_INLINE) |
    (1 << TYPE_NE_SEQ_INT16_INLINE);

  return ((BIT_MAP >> type) & 1) == 1;
}

////////////////////////////////////////////////////////////////////////////////

OBJ make_blank_obj() {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = BLANK_OBJ_MASK;
  return obj;
}

OBJ make_null_obj() {
  OBJ obj;
  obj.core_data.int_ = 0;
  obj.extra_data = NULL_OBJ_MASK;
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

// static bool is_fake(OBJ *elts, uint32 length) {
//   for (int i=0 ; i < length ; i++) {
//     OBJ elt = elts[i];
//     if (!is_int(elt))
//       return false;
//     int64 value = get_int(elt);
//     if (value < 0 | value > 255)
//       return false;
//   }
//   return true;
// }

OBJ make_seq(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

// #ifndef NDEBUG
//   if (is_fake(ptr->buffer.obj, length))
//     printf("make_seq(..)\n");
// #endif

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.obj;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_BASE_MASK;
  return obj;
}

OBJ make_slice(OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 0);

// #ifndef NDEBUG
//   if (is_fake(ptr, length))
//     printf("make_slice(..)\n");
// #endif

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_SLICE_BASE_MASK;
  return obj;
}

OBJ make_seq_uint8(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.uint8_;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_UINT8_BASE_MASK;
  return obj;
}

OBJ make_slice_uint8(uint8 *ptr, uint32 length) {
  assert(ptr != NULL & length > 8);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_SLICE_UINT8_BASE_MASK;
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

OBJ make_seq_int16(SEQ_OBJ *ptr, uint32 length) {
  assert(ptr != NULL & length > 4);

  OBJ obj;
  obj.core_data.ptr = ptr->buffer.int16_;
  obj.extra_data = MAKE_LENGTH(length) | NE_SEQ_INT16_BASE_MASK;
  return obj;
}

OBJ make_slice_int16(int16 *ptr, uint32 length) {
  assert(ptr != NULL & length > 4);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(length) | NE_SLICE_INT16_BASE_MASK;
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

OBJ make_empty_seq() {
  OBJ obj;
  obj.core_data.ptr = NULL;
  obj.extra_data = EMPTY_SEQ_MASK;
  return obj;
}

OBJ make_set(SET_OBJ *ptr, uint32 size) {
  assert(ptr != NULL & size > 0);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_SET_BASE_MASK;
  return obj;
}

OBJ make_empty_rel() {
  OBJ obj;
  obj.core_data.ptr = NULL;
  obj.extra_data = EMPTY_REL_MASK;
  return obj;
}

OBJ make_bin_rel(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_BIN_REL_BASE_MASK;
  return obj;
}

OBJ make_log_map(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_LOG_MAP_BASE_MASK;
  return obj;
}

OBJ make_map(BIN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_MAP_BASE_MASK;
  return obj;
}

OBJ make_tern_rel(TERN_REL_OBJ *ptr, uint32 size) {
  assert(ptr != NULL);

  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_LENGTH(size) | NE_TERN_REL_BASE_MASK;
  return obj;
}

OBJ make_tag_obj_(uint16 tag_id, OBJ obj) {
  OBJ_TYPE type = get_physical_type(obj);
  uint8 tags_count = get_tags_count(obj);

  if (tags_count == 0) {
    if (type == TYPE_OPT_REC) {
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

  OBJ tag_obj;
  TAG_OBJ *ptr = new_tag_obj();
  ptr->obj = obj;
  tag_obj.core_data.ptr = ptr;
  tag_obj.extra_data = TAG_OBJ_BASE_MASK | MAKE_TAG(tag_id) | MAKE_TAGS_COUNT(1);
  return tag_obj;
}


OBJ make_tag_obj(uint16 tag_id, OBJ obj) {
  OBJ tag_obj = make_tag_obj_(tag_id, obj);
  assert(!(get_physical_type(tag_obj) == TYPE_NE_SET & get_tags_count(tag_obj) == 2));
  assert(get_tag_id(tag_obj) == tag_id);
  assert(are_shallow_eq(get_inner_obj(tag_obj), obj));
  return tag_obj;
}


OBJ make_opt_tag_rec(void *ptr, uint16 repr_id) {
  OBJ obj;
  obj.core_data.ptr = ptr;
  obj.extra_data = MAKE_OPT_REPR_ID(repr_id) | OPT_TAG_REC_BASE_MASK;
  return obj;
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

uint32 read_size_field(OBJ obj) {
  return GET(obj.extra_data, LENGTH_SHIFT, LENGTH_WIDTH);
}

uint32 get_seq_length(OBJ seq) {
  assert(is_seq(seq));
  return read_size_field(seq);
}

uint32 get_rel_size(OBJ rel) {
  assert(is_set(rel) | is_bin_rel(rel) | is_tern_rel(rel));
  assert(get_physical_type(rel) != TYPE_OPT_REC);
  return read_size_field(rel);
}

uint16 get_tag_id(OBJ obj) {
  assert(is_tag_obj(obj));
  assert(get_tags_count(obj) != 0 || get_physical_type(obj) == TYPE_OPT_TAG_REC);

  if (get_tags_count(obj) != 0)
    return GET(obj.extra_data, TAG_SHIFT, TAG_WIDTH);
  else
    return opt_repr_get_tag_id(get_opt_repr_id(obj));
}

OBJ ref_get_inner_obj(OBJ obj) {
  assert(is_tag_obj(obj));

  OBJ_TYPE type = get_physical_type(obj);

  if (is_single_tag_type(type)) {
    assert(get_tags_count(obj) == 1);
    obj.extra_data = CLEAR(obj.extra_data, TAG_MASK | TAGS_COUNT_MASK);
    return obj;
  }

  uint8 tags_count = get_tags_count(obj);

  if (type == TYPE_TAG_OBJ & tags_count == 1)
    return ((TAG_OBJ *) obj.core_data.ptr)->obj;

  if (tags_count == 0) {
    assert(type == TYPE_OPT_TAG_REC);
    obj.extra_data = CLEAR(obj.extra_data, TYPE_MASK) | OPT_REC_BASE_MASK;
    return obj;
  }

  uint16 inner_tag = GET(obj.extra_data, INNER_TAG_SHIFT, TAG_WIDTH);
  uint64 cleared_extra_data = CLEAR(obj.extra_data, INNER_TAG_MASK | TAG_MASK | TAGS_COUNT_MASK);
  obj.extra_data = cleared_extra_data | MAKE_TAG(inner_tag) | MAKE_TAGS_COUNT(tags_count-1);
  return obj;
}

OBJ new_get_inner_obj(OBJ obj) {
  assert(is_tag_obj(obj));

  uint32 tags_count = get_tags_count(obj);

  if (tags_count == 1) {
    OBJ_TYPE type = get_physical_type(obj);
    if (type != TYPE_TAG_OBJ) {
      obj.extra_data = CLEAR(obj.extra_data, TAG_MASK | TAGS_COUNT_MASK);
      return obj;
    }
    else
      return ((TAG_OBJ *) obj.core_data.ptr)->obj;
  }
  else if (tags_count == 2) {
    uint16 inner_tag = GET(obj.extra_data, INNER_TAG_SHIFT, TAG_WIDTH);
    uint64 cleared_extra_data = CLEAR(obj.extra_data, INNER_TAG_MASK | TAG_MASK | TAGS_COUNT_MASK);
    obj.extra_data = cleared_extra_data | MAKE_TAG(inner_tag) | MAKE_TAGS_COUNT(1);
    return obj;
  }
  else {
    assert(tags_count == 0);
    assert(get_physical_type(obj) == TYPE_OPT_TAG_REC);

    obj.extra_data = CLEAR(obj.extra_data, TYPE_MASK) | OPT_REC_BASE_MASK;
    return obj;
  }
}

OBJ get_inner_obj(OBJ obj) {
  assert(are_shallow_eq(new_get_inner_obj(obj), ref_get_inner_obj(obj)));
  return new_get_inner_obj(obj);
}

////////////////////////////////////////////////////////////////////////////////

OBJ *get_seq_elts_ptr(OBJ obj) {
  assert(is_ne_seq(obj));
  assert(get_physical_type(obj) == TYPE_NE_SEQ | get_physical_type(obj) == TYPE_NE_SLICE);
  return (OBJ *) obj.core_data.ptr;
}

uint8 *get_seq_elts_ptr_uint8(OBJ obj) {
  assert(is_ne_seq(obj));
  assert(get_physical_type(obj) == TYPE_NE_SLICE_UINT8 | get_physical_type(obj) == TYPE_NE_SEQ_UINT8);
  return (uint8 *) obj.core_data.ptr;
}

int16 *get_seq_elts_ptr_int16(OBJ obj) {
  assert(is_ne_seq(obj));
  assert(get_physical_type(obj) == TYPE_NE_SLICE_INT16 | get_physical_type(obj) == TYPE_NE_SEQ_INT16);
  return (int16 *) obj.core_data.ptr;
}

OBJ *get_set_elts_ptr(OBJ obj) {
  assert(is_ne_set(obj));
  return (OBJ *) obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ* get_seq_ptr(OBJ obj) {
  assert(
    get_physical_type(obj) == TYPE_NE_SEQ |
    get_physical_type(obj) == TYPE_NE_SEQ_UINT8 |
    get_physical_type(obj) == TYPE_NE_SEQ_INT16
  );
  return (SEQ_OBJ *) (((char *) obj.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET);
}

// SET_OBJ* get_set_ptr(OBJ obj) {
//   assert(get_physical_type(obj) == TYPE_NE_SET & obj.core_data.ptr != NULL);
//   return (SET_OBJ *) obj.core_data.ptr;
// }

BIN_REL_OBJ *get_bin_rel_ptr(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_NE_BIN_REL | get_physical_type(obj) == TYPE_NE_LOG_MAP | get_physical_type(obj) == TYPE_NE_MAP);
  assert(obj.core_data.ptr != NULL);
  return (BIN_REL_OBJ *) obj.core_data.ptr;
}

TERN_REL_OBJ *get_tern_rel_ptr(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_NE_TERN_REL & obj.core_data.ptr != NULL);
  return (TERN_REL_OBJ *) obj.core_data.ptr;
}

TAG_OBJ *get_tag_obj_ptr(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_TAG_OBJ & obj.core_data.ptr != NULL);
  return (TAG_OBJ *) obj.core_data.ptr;
}

void* get_opt_tag_rec_ptr(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_OPT_TAG_REC);
  return obj.core_data.ptr;
}

void* get_opt_repr_ptr(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_OPT_REC | get_physical_type(obj) == TYPE_OPT_TAG_REC);
  return obj.core_data.ptr;
}

uint16 get_opt_repr_id(OBJ obj) {
  return GET(obj.extra_data, OPT_REPR_ID_SHIFT, OPT_REPR_ID_WIDTH);
}

////////////////////////////////////////////////////////////////////////////////

bool is_blank_obj(OBJ obj) {
  return get_physical_type(obj) == TYPE_BLANK_OBJ;
}

bool is_null_obj(OBJ obj) {
  return get_physical_type(obj) == TYPE_NULL_OBJ;
}

bool is_symb(OBJ obj) {
  assert(
    (get_ex_type(obj) == TYPE_SYMBOL)
      ==
    (get_tags_count(obj) == 0 & get_physical_type(obj) == TYPE_SYMBOL)
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
  return ex_type == TYPE_EMPTY_SEQ | ex_type == TYPE_NE_SEQ | ex_type == TYPE_NE_SLICE |
         (ex_type >= MIN_OPT_SEQ_TYPE & ex_type <= MAX_OPT_SEQ_TYPE);
}

bool is_empty_seq(OBJ obj) {
  return obj.extra_data == EMPTY_SEQ_MASK;
}

bool is_ne_seq(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_SEQ | ex_type == TYPE_NE_SLICE |
         (ex_type >= MIN_OPT_SEQ_TYPE & ex_type <= MAX_OPT_SEQ_TYPE);
}

bool is_empty_rel(OBJ obj) {
  return obj.extra_data == EMPTY_REL_MASK;
}

bool is_ne_set(OBJ obj) {
  assert(
    (get_ex_type(obj) == TYPE_NE_SET) ==
    ((get_physical_type(obj) == TYPE_NE_SET) & get_tags_count(obj) == 0)
  );
  return get_ex_type(obj) == TYPE_NE_SET;
}

bool is_set(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_SET | ex_type == TYPE_EMPTY_REL;
}

bool is_ne_bin_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_BIN_REL | ex_type == TYPE_NE_MAP |
         ex_type == TYPE_NE_LOG_MAP | ex_type == TYPE_OPT_REC;
}

bool is_ne_map(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_NE_MAP | ex_type == TYPE_NE_LOG_MAP | ex_type == TYPE_OPT_REC;
}

bool is_bin_rel(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_EMPTY_REL | ex_type == TYPE_NE_BIN_REL | ex_type == TYPE_NE_MAP |
         ex_type == TYPE_NE_LOG_MAP | ex_type == TYPE_OPT_REC;
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
  return ex_type == TYPE_OPT_TAG_REC | ex_type > MAX_PHYSICAL_TYPE;
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
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_OPT_REC;
}

bool is_opt_rec_or_tag_rec(OBJ obj) {
  uint32 ex_type = get_ex_type(obj);
  return ex_type == TYPE_OPT_REC | ex_type == TYPE_OPT_TAG_REC;
}

////////////////////////////////////////////////////////////////////////////////

bool is_always_inline(OBJ obj) {
  return get_physical_type(obj) <= MAX_ALWAYS_INLINE_OBJ_TYPE;
}

bool is_inline_obj(OBJ obj) {
  OBJ_TYPE type = get_physical_type(obj);
  assert(type <= MAX_ALWAYS_INLINE_OBJ_TYPE | type >= MIN_INLINE_PHYSICAL_OBJ_TYPE | obj.core_data.ptr != NULL);
  return type <= MAX_ALWAYS_INLINE_OBJ_TYPE | type >= MIN_INLINE_PHYSICAL_OBJ_TYPE;
}

////////////////////////////////////////////////////////////////////////////////

void *get_ref_obj_ptr(OBJ obj) {
  OBJ_TYPE type = get_physical_type(obj);
  assert(
    type == TYPE_NE_SEQ | type == TYPE_NE_SET      | type == TYPE_NE_BIN_REL | type == TYPE_NE_LOG_MAP |
    type == TYPE_NE_MAP | type == TYPE_NE_TERN_REL | type == TYPE_TAG_OBJ    | type == TYPE_OPT_REC |
    type == TYPE_OPT_TAG_REC
  );

  if (type == TYPE_NE_SEQ)
    return ((char *) obj.core_data.ptr) - SEQ_BUFFER_FIELD_OFFSET;

  return obj.core_data.ptr;
}

////////////////////////////////////////////////////////////////////////////////

bool are_shallow_eq(OBJ obj1, OBJ obj2) {
  return obj1.core_data.int_ == obj2.core_data.int_ && obj1.extra_data == obj2.extra_data;
}

int shallow_cmp(OBJ obj1, OBJ obj2) {
  assert(is_always_inline(obj1) & is_always_inline(obj2));

  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  if (extra_data_1 < extra_data_2)
    return 1;

  if (extra_data_1 > extra_data_2)
    return -1;

  int64 core_data_1 = obj1.core_data.int_;
  int64 core_data_2 = obj2.core_data.int_;

  if (core_data_1 < core_data_2)
    return 1;

  if (core_data_1 > core_data_2)
    return -1;

  return 0;
}

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

//## IF WE DECIDE TO OPTIMIZE THE COPYING OF SEQUENCES AND/OR SLICES, THIS WILL STOP WORKING
OBJ repoint_to_copy(OBJ obj, void *new_ptr) {
  assert(!is_inline_obj(obj));
  assert(get_physical_type(obj) != TYPE_NE_SLICE);

  obj.core_data.ptr = new_ptr;
  return obj;
}

OBJ repoint_slice_to_seq(OBJ obj, SEQ_OBJ *ptr) {
  assert(get_physical_type(obj) == TYPE_NE_SLICE);
  assert(get_tags_count(obj) == 0 | get_tags_count(obj) == 1);

  OBJ new_obj = make_seq(ptr, ptr->used);
  if (get_tags_count(obj) != 0)
    new_obj = make_tag_obj(get_tag_id(obj), new_obj);
  return new_obj;
}

OBJ repoint_uint8_seq_to_slice(OBJ obj, uint8 *elts) {
  assert(get_physical_type(obj) == TYPE_NE_SEQ_UINT8);
  assert(get_tags_count(obj) == 0 | get_tags_count(obj) == 1);

  OBJ new_obj = make_slice_uint8(elts, read_size_field(obj));
  if (get_tags_count(obj) != 0)
    new_obj = make_tag_obj(get_tag_id(obj), new_obj);
  return new_obj;
}

OBJ repoint_int16_seq_to_slice(OBJ obj, int16 *elts) {
  assert(get_physical_type(obj) == TYPE_NE_SEQ_INT16);
  assert(get_tags_count(obj) == 0 | get_tags_count(obj) == 1);

  OBJ new_obj = make_slice_int16(elts, read_size_field(obj));
  if (get_tags_count(obj) != 0)
    new_obj = make_tag_obj(get_tag_id(obj), new_obj);
  return new_obj;
}

////////////////////////////////////////////////////////////////////////////////

uint16 get_inline_tag_id(OBJ obj) {
  assert(get_tags_count(obj) > 0);
  return GET(obj.extra_data, TAG_SHIFT, TAG_WIDTH);
}

uint16 get_nested_inline_tag_id(OBJ obj) {
  assert(get_tags_count(obj) == 2);
  return GET(obj.extra_data, INNER_TAG_SHIFT, TAG_WIDTH);
}

OBJ untag_opt_tag_rec(OBJ obj) {
  assert(get_physical_type(obj) == TYPE_OPT_TAG_REC);

  obj.extra_data = CLEAR(obj.extra_data, TYPE_MASK) | OPT_REC_BASE_MASK;
  return obj;
}

OBJ clear_both_inline_tags(OBJ obj) {
  assert(get_tags_count(obj) == 2);
  assert(!is_single_tag_type(get_physical_type(obj)));
  return get_inner_obj(get_inner_obj(obj)); //## IMPLEMENT FOR REAL
}

OBJ clear_all_inline_tags(OBJ obj) {
  assert(!is_single_tag_type(get_physical_type(obj)));
  obj.extra_data = CLEAR(obj.extra_data, TAG_MASK | INNER_TAG_MASK);
  return obj;
}