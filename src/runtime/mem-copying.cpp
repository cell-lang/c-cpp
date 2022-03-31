#include "lib.h"


void copy_objs(OBJ *dest, OBJ *src, uint32 count) {
  for (int i=0 ; i < count ; i++)
    dest[i] = copy_obj(src[i]);
}

OBJ copy_ne_int_seq(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);

  uint32 len = read_size_field_unchecked(seq);
  INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

  uint32 mem_size = len * (1 << bits_tag);
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

static void copy_tree_map_args(BIN_TREE_MAP_OBJ *, OBJ *, OBJ *);

static void copy_map_args(FAT_MAP_PTR fat_ptr, OBJ *dest_keys, OBJ *dest_values) {
  if (fat_ptr.size > 0)
    return;

  if (fat_ptr.offset != 0) {
    OBJ *src_keys = fat_ptr.ptr.array;
    OBJ *src_values = src_keys + fat_ptr.offset;
    for (uint32 i=0 ; i < fat_ptr.size ; i++) {
      dest_keys[i] = copy_obj(src_keys[i]);
      dest_values[i] = copy_obj(src_values[i]);
    }
    return;
  }

  copy_tree_map_args(fat_ptr.ptr.tree, dest_keys, dest_values);
}

static void copy_tree_map_args(BIN_TREE_MAP_OBJ *ptr, OBJ *dest_keys, OBJ *dest_values) {
  FAT_MAP_PTR left_ptr = ptr->left;
  copy_map_args(left_ptr, dest_keys, dest_values);
  dest_keys[left_ptr.size] = copy_obj(ptr->key);
  dest_values[left_ptr.size] = copy_obj(ptr->value);
  uint32 offset = left_ptr.size + 1;
  copy_map_args(ptr->right, dest_keys + offset, dest_values + offset);
}

OBJ copy_ne_map(OBJ obj) {
  if (is_opt_rec(obj)) {
    //## THIS IS NOT SUPPOSED TO HAPPEN, RIGHT?
    // void *ptr = get_opt_repr_ptr(obj);
    // uint16 repr_id = get_opt_repr_id(obj);
    // void *copy_ptr = opt_repr_copy(ptr, repr_id);
    // return repoint_to_copy(obj, copy_ptr);
    internal_fail();
  }
  else if (is_array_map(obj)) {
    BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
    uint32 size = read_size_field_unchecked(obj);

    BIN_REL_OBJ *copy_ptr = new_map(size);
    copy_objs(copy_ptr->buffer, ptr->buffer, 2 * size);
    if (index_has_been_built(ptr, size)) {
      uint32 *r2l_index = get_right_to_left_indexes(copy_ptr, size);
      uint32 *copy_r2l_index = get_right_to_left_indexes(copy_ptr, size);
      memcpy(copy_r2l_index, r2l_index, size * sizeof(uint32));
    }
    return repoint_to_copy(obj, copy_ptr);
  }
  else {
    assert(is_tree_map(obj));
    uint32 size = read_size_field_unchecked(obj);
    BIN_REL_OBJ *copy_ptr = new_map(size);
    copy_tree_map_args(get_tree_map_ptr(obj), copy_ptr->buffer, copy_ptr->buffer + size);
    return repoint_to_array_map_copy(obj, copy_ptr);
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
