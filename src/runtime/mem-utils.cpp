#include "lib.h"


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

__attribute__ ((noinline)) int intrl_cmp(OBJ obj1, OBJ obj2) {
  uint64 extra_data_1 = obj1.extra_data;
  uint64 extra_data_2 = obj2.extra_data;

  uint64 log_extra_data_1 = extra_data_1 << REPR_INFO_WIDTH;
  uint64 log_extra_data_2 = extra_data_2 << REPR_INFO_WIDTH;

  if (log_extra_data_1 != log_extra_data_2)
    return log_extra_data_1 < log_extra_data_2 ? 1 : -1;

  OBJ_TYPE type = (OBJ_TYPE) (log_extra_data_1 >> (REPR_INFO_WIDTH + TYPE_SHIFT));
  assert(type == get_obj_type(obj1) & type == get_obj_type(obj2));

  if (type <= MAX_INLINE_OBJ_TYPE) {
    int64 core_data_1 = obj1.core_data.int_;
    int64 core_data_2 = obj2.core_data.int_;
    return core_data_1 < core_data_2 ? 1 : (core_data_1 == core_data_2 ? 0 : -1);
  }

  extern int (*intrl_cmp_disp_table[])(OBJ, OBJ);
  return intrl_cmp_disp_table[type - MAX_INLINE_OBJ_TYPE - 1](obj1, obj2);

  // int intrl_cmp_non_inline(OBJ_TYPE type, OBJ obj1, OBJ obj2);
  // return intrl_cmp_non_inline(type, obj1, obj2);
}

////////////////////////////////////////////////////////////////////////////////
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
  static int frags[] = {5, 5, 2, 4, 16, 16, 16};
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
