#include "lib.h"


void copy_objs(OBJ *dest, OBJ *src, uint32 count) {
  for (int i=0 ; i < count ; i++)
    dest[i] = copy_obj(src[i]);
}

OBJ copy_ne_int_seq(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);

  uint32 len = read_size_field_unchecked(seq);
  INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

  uint32 mem_size = len * (8 << bits_tag);
  void *copy_elts = new_obj((mem_size + 7) & ~7);
  memcpy(copy_elts, seq.core_data.ptr, mem_size);
  return repoint_to_sliced_copy(seq, copy_elts);

  // if (bits_tag < INT_BITS_32_TAG) {
  //   if (bits_tag == INT_BITS_8_TAG) {
  //     // Works also for unsigned bytes
  //     int8 *copy_elts = new_int8_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int8));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  //   else {
  //     assert(bits_tag == INT_BITS_16_TAG);
  //     int16 *copy_elts = new_int16_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int16));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  // }
  // else {
  //   if (bits_tag == INT_BITS_32_TAG) {
  //     int32 *copy_elts = new_int32_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int32));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  //   else {
  //     assert(bits_tag == INT_BITS_64_TAG);
  //     int64 *copy_elts = new_int64_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int64));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  // }
}

OBJ copy_ne_float_seq(OBJ obj) {
  double *elts = (double *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  double *copy_elts = new_float_array(size);
  memcpy(copy_elts, elts, size * sizeof(double));
  return repoint_to_sliced_copy(obj, copy_elts);
}

OBJ copy_ne_bool_seq(OBJ obj) {
  internal_fail();
}

OBJ copy_ne_seq(OBJ obj) {
  OBJ *elts = (OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  OBJ *copy_elts = new_obj_array(size);
  copy_objs(copy_elts, elts, size);
  return repoint_to_sliced_copy(obj, copy_elts);
}

OBJ copy_ne_set(OBJ obj) {
  SET_OBJ *ptr = (SET_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  SET_OBJ *copy_ptr = new_set(size);
  copy_objs(copy_ptr->buffer, ptr->buffer, size);
  return repoint_to_copy(obj, copy_ptr);
}

OBJ copy_ne_map(OBJ obj) {
  if (is_opt_rec(obj)) {
    void *ptr = get_opt_repr_ptr(obj);
    uint16 repr_id = get_opt_repr_id(obj);
    void *copy_ptr = opt_repr_copy(ptr, repr_id);
    return repoint_to_copy(obj, copy_ptr);
  }
  else if (is_array_map(obj)) {
    BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
    uint32 size = read_size_field_unchecked(obj);

    BIN_REL_OBJ *copy_ptr = new_map(size);
    copy_objs(copy_ptr->buffer, ptr->buffer, 2 * size);
    if (index_has_been_build(ptr, size)) {
      uint32 *r2l_index = get_right_to_left_indexes(copy_ptr, size);
      uint32 *copy_r2l_index = get_right_to_left_indexes(copy_ptr, size);
      memcpy(copy_r2l_index, r2l_index, size * sizeof(uint32));
    }
    return repoint_to_copy(obj, copy_ptr);
  }
  else if (is_bin_tree_map(obj)) {
    internal_fail();
  }
}

OBJ copy_ne_bin_rel(OBJ obj) {
  BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  BIN_REL_OBJ *copy_ptr = new_bin_rel(size);
  copy_objs(copy_ptr->buffer, ptr->buffer, 2 * size);
  memcpy(get_right_to_left_indexes(copy_ptr, size), get_right_to_left_indexes(ptr, size), size * sizeof(uint32));
  return repoint_to_copy(obj, copy_ptr);
}

OBJ copy_ne_tern_rel(OBJ obj) {
  TERN_REL_OBJ *ptr = (TERN_REL_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  TERN_REL_OBJ *copy_ptr = new_tern_rel(size);
  copy_objs(copy_ptr->buffer, ptr->buffer, 3 * size);
  memcpy(get_rotated_index(copy_ptr, size, 1), get_rotated_index(ptr, size, 1), 2 * size * sizeof(uint32));
  return repoint_to_copy(obj, copy_ptr);
}

OBJ copy_ad_hoc_tag_rec(OBJ obj) {
  void *ptr = get_opt_repr_ptr(obj);
  uint16 repr_id = get_opt_repr_id(obj);
  void *copy = opt_repr_copy(ptr, repr_id);
  return repoint_to_copy(obj, copy);
}

OBJ copy_boxed_obj(OBJ obj) {
  BOXED_OBJ *copy_ptr = new_boxed_obj();
  copy_ptr->obj = copy_obj(get_boxed_obj_ptr(obj)->obj);
  return repoint_to_copy(obj, copy_ptr);
}

////////////////////////////////////////////////////////////////////////////////

OBJ copy_obj(OBJ obj) {
  if (is_inline_obj(obj) || !needs_copying(obj.core_data.ptr))
    return obj;

  switch (get_obj_type(obj)) {
    case TYPE_NE_INT_SEQ:
      return copy_ne_int_seq(obj);

    case TYPE_NE_FLOAT_SEQ:
      return copy_ne_float_seq(obj);

    case TYPE_NE_BOOL_SEQ:
      return copy_ne_bool_seq(obj);

    case TYPE_NE_SEQ:
      return copy_ne_seq(obj);

    case TYPE_NE_SET:
      return copy_ne_set(obj);

    case TYPE_NE_MAP:
      return copy_ne_map(obj);

    case TYPE_NE_BIN_REL:
      return copy_ne_bin_rel(obj);

    case TYPE_NE_TERN_REL:
      return copy_ne_tern_rel(obj);

    case TYPE_AD_HOC_TAG_REC:
      return copy_ad_hoc_tag_rec(obj);

    case TYPE_BOXED_OBJ:
      return copy_boxed_obj(obj);

    default:
      internal_fail();
  }
}
