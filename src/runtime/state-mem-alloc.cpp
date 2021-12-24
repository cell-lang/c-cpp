#include "lib.h"

//    0    1    2     3     4     5      6      7    8
//   16   32   64   128   256   512   1024   2048 4096

// unsigned int v;
// unsigned r = 0;

// while (v >>= 1) {
//     r++;
// }

static uint32 subpool_index(uint32 size) {
  assert(size <= 4096);

  if (size <= 128) {
    if (size <= 32)
      return size <= 16 ? 0 : 1;
    else
      return size <= 64 ? 2 : 3;
  }
  else if (size <= 2048) {
    if (size <= 512)
      return size <=  256 ? 4 : 5;
    else
      return size <= 1024 ? 6 : 7;
  }
  else
    return 8;
}


void init_mem_pool(STATE_MEM_POOL *mem_pool) {
  memset(mem_pool, 0, sizeof(STATE_MEM_POOL));
}

void *alloc_state_mem_block(STATE_MEM_POOL *mem_pool, uint32 byte_size) {
  return malloc(byte_size);
}

void release_state_mem_block(STATE_MEM_POOL *mem_pool, void *ptr, uint32 byte_size) {
  free(ptr);
}

void release_mem_pool(STATE_MEM_POOL *mem_pool) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static uint32 round_up(uint32 mem_size) {
  return (mem_size + 7) & ~7;
}

static uint32 obj_mem_size(OBJ obj);

static uint32 objs_mem_size(OBJ *objs, uint32 count) {
  uint32 mem_size = 0;
  for (int i=0 ; i < count ; i++)
    mem_size += obj_mem_size(objs[i]);
  return mem_size;
}

static uint32 ne_int_seq_mem_size(OBJ seq) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);

  uint32 len = read_size_field_unchecked(seq);
  INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

  if (bits_tag < INT_BITS_TAG_32) {
    if (bits_tag == INT_BITS_TAG_8) {
      // Works also for unsigned bytes
      return round_up(len * sizeof(int8));
    }
    else {
      assert(bits_tag == INT_BITS_TAG_16);
      int16 *array = get_seq_elts_ptr_int16(seq);
      int16 min = 0, max = 0;
      for (uint32 i=0 ; i < len ; i++) {
        int16 value = array[i];
        min = value < min ? value : min;
        max = value > max ? value : max;
        if (!is_int8_or_uint8_range(min, max))
          return round_up(len * sizeof(int16));
      }
      return round_up(len * sizeof(int8)); // Works also for unsigned bytes
    }
  }
  else {
    if (bits_tag == INT_BITS_TAG_32) {
      int32 *array = get_seq_elts_ptr_int32(seq);
      int64 min = 0, max = 0;
      for (uint32 i=0 ; i < len ; i++) {
        int64 value = array[i];
        min = value < min ? value : min;
        max = value > max ? value : max;
        if (!is_int16_range(min, max))
          return round_up(len * sizeof(int32));
      }
      return round_up(len * (is_int8_or_uint8_range(min, max) ? sizeof(int8) : sizeof(int16)));
    }
    else {
      assert(bits_tag == INT_BITS_TAG_64);
      int64 *array = get_seq_elts_ptr_int64(seq);
      int64 min = 0, max = 0;
      for (uint32 i=0 ; i < len ; i++) {
        int64 value = array[i];
        min = value < min ? value : min;
        max = value > max ? value : max;
        if (!is_int32_range(min, max))
          return round_up(len * sizeof(int64));
      }
      uint32 byte_size = sizeof(int64);
      if (is_int32_range(min, max))
        if (is_int16_range(min, max))
          if (is_int8_or_uint8_range(min, max))
            byte_size = sizeof(int8);
          else
            byte_size = sizeof(int16);
        else
          byte_size = sizeof(int32);
      return round_up(len * byte_size);
    }
  }
  return round_up(len * (8 << bits_tag));
}

static uint32 ne_float_seq_mem_size(OBJ seq) {
  uint32 len = read_size_field_unchecked(seq);
  return round_up(len * sizeof(double));
}

static uint32 ne_bool_seq_mem_size(OBJ seq) {
  internal_fail();
}

static uint32 ne_seq_mem_size(OBJ seq) {
  uint32 len = read_size_field_unchecked(seq);
  OBJ *elts = get_seq_elts_ptr(seq);

  uint32 max_size = round_up(len * sizeof(seq));

  OBJ first_elt = elts[0];

  if (is_int(first_elt)) {
    int64 min = get_int(first_elt);
    int64 max = min;

    for (uint32 i=1 ; i < len ; i++) {
      OBJ elt = elts[i];
      if (!is_int(elt))
        return max_size;

      int64 value = get_int(elt);
      min = value < min ? value : min;
      max = value > max ? value : max;
    }

    if (is_int16_range(min, max))
      if (is_int8_or_uint8_range(min, max)) {
        assert(len > 8);
        return round_up(len * sizeof(int8)); // Work also for unsigned bytes
      }
      else {
        assert(len > 4);
        return round_up(len * sizeof(int16));
      }
    else
      if (is_int32_range(min, max)) {
        assert(len > 2);
        return round_up(len * sizeof(int32));
      }
      else {
        // assert(len > 1); // Actually not true
        return round_up(len * sizeof(int64));
      }
  }

  if (is_float(first_elt)) {
    for (int i=1 ; i < len ; i++)
      if (!is_float(elts[i]))
        return max_size;
    return round_up(len * sizeof(double));
  }

  return max_size;
}

static uint32 ne_set_mem_size(OBJ obj) {
  uint32 size = read_size_field_unchecked(obj);
  SET_OBJ *ptr = (SET_OBJ *) obj.core_data.ptr;
  return set_obj_mem_size(size) + objs_mem_size(ptr->buffer, size);
}

static uint32 ne_map_mem_size(OBJ obj) {
  if (is_opt_rec(obj)) {
    internal_fail();
  }
  else if (is_array_map(obj)) {
    BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
    uint32 size = read_size_field_unchecked(obj);
    return round_up(bin_rel_obj_mem_size(size)) + objs_mem_size(ptr->buffer, 2 * size);
  }
  else if (is_bin_tree_map(obj)) {
    internal_fail();
  }
}

static uint32 ne_bin_rel_mem_size(OBJ obj) {
  BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);
  return round_up(bin_rel_obj_mem_size(size)) + objs_mem_size(ptr->buffer, 2 * size);
}

static uint32 ne_tern_rel_mem_size(OBJ obj) {
  BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);
  return round_up(tern_rel_obj_mem_size(size)) + objs_mem_size(ptr->buffer, 3 * size);
}

static uint32 ad_hoc_tag_rec_mem_size(OBJ obj) {
  void *ptr = get_opt_repr_ptr(obj);
  uint16 repr_id = get_opt_repr_id(obj);
  return opt_repr_mem_size(ptr, repr_id);
}

static uint32 boxed_obj_mem_size(OBJ obj) {
  BOXED_OBJ *ptr = get_boxed_obj_ptr(obj);
  return round_up(boxed_obj_mem_size()) + obj_mem_size(ptr->obj);
}

////////////////////////////////////////////////////////////////////////////////

static uint32 obj_mem_size(OBJ obj) {
  if (is_inline_obj(obj))
    return sizeof(obj);

  switch (get_obj_type(obj)) {
    case TYPE_NE_INT_SEQ:
      return ne_int_seq_mem_size(obj);

    case TYPE_NE_FLOAT_SEQ:
      return ne_float_seq_mem_size(obj);

    case TYPE_NE_BOOL_SEQ:
      return ne_bool_seq_mem_size(obj);

    case TYPE_NE_SEQ:
      return ne_seq_mem_size(obj);

    case TYPE_NE_SET:
      return ne_set_mem_size(obj);

    case TYPE_NE_MAP:
      return ne_map_mem_size(obj);

    case TYPE_NE_BIN_REL:
      return ne_bin_rel_mem_size(obj);

    case TYPE_NE_TERN_REL:
      return ne_tern_rel_mem_size(obj);

    case TYPE_AD_HOC_TAG_REC:
      return ad_hoc_tag_rec_mem_size(obj);

    case TYPE_BOXED_OBJ:
      return boxed_obj_mem_size(obj);

    default:
      internal_fail();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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

  // if (bits_tag < INT_BITS_TAG_32) {
  //   if (bits_tag == INT_BITS_TAG_8) {
  //     // Works also for unsigned bytes
  //     int8 *copy_elts = new_int8_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int8));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  //   else {
  //     assert(bits_tag == INT_BITS_TAG_16);
  //     int16 *copy_elts = new_int16_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int16));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  // }
  // else {
  //   if (bits_tag == INT_BITS_TAG_32) {
  //     int32 *copy_elts = new_int32_array(len);
  //     memcpy(copy_elts, seq.core_data.ptr, size * sizeof(int32));
  //     return repoint_to_sliced_copy(seq, copy_elts);
  //   }
  //   else {
  //     assert(bits_tag == INT_BITS_TAG_64);
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
