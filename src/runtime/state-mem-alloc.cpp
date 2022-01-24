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

////////////////////////////////////////////////////////////////////////////////

static void *alloc_new_pages(uint32 count) {
  assert(count > 0);
  return malloc(4096 * count);
}

static void *acquire_block(STATE_MEM_POOL *mem_pool, uint32 pool_idx) {
  assert(lengthof(mem_pool->subpools) == 9);
  assert(pool_idx < lengthof(mem_pool->subpools));

  void *ptr = mem_pool->subpools[pool_idx];
  if (ptr != NULL) {
    void *next_ptr = *((void **) ptr);
    mem_pool->subpools[pool_idx] = next_ptr;
    return ptr;
  }
  else if (pool_idx < 8) {
    void *ptr = acquire_block(mem_pool, pool_idx + 1);
    uint32 size = 16 << pool_idx;
    void *next_ptr = ((uint8 *) ptr) + size;
    void **after_next_ptr = (void **) next_ptr;
    *after_next_ptr = NULL;
    mem_pool->subpools[pool_idx] = next_ptr;
    return ptr;
  }
  else {
    return alloc_new_pages(1);
  }
}

////////////////////////////////////////////////////////////////////////////////

void init_mem_pool(STATE_MEM_POOL *mem_pool) {
  memset(mem_pool, 0, sizeof(STATE_MEM_POOL));
}

void release_mem_pool(STATE_MEM_POOL *mem_pool) {

}

////////////////////////////////////////////////////////////////////////////////

void *alloc_state_mem_block(STATE_MEM_POOL *mem_pool, uint32 byte_size) {
  if (byte_size > 4096) {
    internal_fail();
  }

  uint32 pool_idx = subpool_index(byte_size);
  void *ptr = acquire_block(mem_pool, pool_idx);
  return ptr;
}

void release_state_mem_block(STATE_MEM_POOL *mem_pool, void *ptr, uint32 byte_size) {
  if (byte_size > 4096) {
    internal_fail();
  }

  uint32 pool_idx = subpool_index(byte_size);
  void *next_ptr = mem_pool->subpools[pool_idx];
  *((void **) ptr) = next_ptr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static uint32 round_up(uint32 mem_size) {
  return (mem_size + 7) & ~7;
}

static uint32 null_round_up(uint32 mem_size) {
  assert(mem_size % 8 == 0);
  return mem_size;
}

////////////////////////////////////////////////////////////////////////////////

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
      uint32 elt_mem_size = sizeof(int64);
      if (is_int32_range(min, max))
        if (is_int16_range(min, max))
          if (is_int8_or_uint8_range(min, max))
            elt_mem_size = sizeof(int8);
          else
            elt_mem_size = sizeof(int16);
        else
          elt_mem_size = sizeof(int32);
      return round_up(len * elt_mem_size);
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

  OBJ first_elt = elts[0];

  if (is_int(first_elt)) {
    int64 min = get_int(first_elt);
    int64 max = min;

    for (uint32 i=1 ; i < len ; i++) {
      OBJ elt = elts[i];
      if (!is_int(elt))
        goto obj_seq;

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
        goto obj_seq;
    return round_up(len * sizeof(double));
  }

obj_seq:
  return round_up(len * sizeof(OBJ)) + objs_mem_size(elts, len);
}

static uint32 ne_set_mem_size(OBJ obj) {
  uint32 size = read_size_field_unchecked(obj);
  SET_OBJ *ptr = (SET_OBJ *) obj.core_data.ptr;
  return round_up(set_obj_mem_size(size)) + objs_mem_size(ptr->buffer, size);
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

uint32 obj_mem_size(OBJ obj) {
  if (is_inline_obj(obj))
    return 0;

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

inline void *grab_mem(void **mem_var, uint32 byte_size) {
  assert(byte_size % 8 == 0);
  uint8 *mem = (uint8 *) *mem_var;
  *mem_var = mem + byte_size;
  return mem;
}

////////////////////////////////////////////////////////////////////////////////

//## THIS COULD BE MADE MORE EFFICIENT BY ALSO PROVIDING ACCESS TO THE MEMORY THE OBJECTS ARE TO BE COPIED TO
static void copy_objs_to(OBJ *src_array, uint32 size, OBJ *dest_array, void **dest_var) {
  for (int i=0 ; i < size ; i++)
    dest_array[i] = copy_obj_to(src_array[i], dest_var);
}

static OBJ copy_ne_int_seq_to(OBJ seq, void **dest_var) {
  assert(get_obj_type(seq) == TYPE_NE_INT_SEQ);

  uint32 len = read_size_field_unchecked(seq);
  INT_BITS_TAG bits_tag = get_int_bits_tag(seq);

  // uint32 mem_size = len * (8 << bits_tag);
  // void *copy_elts = new_obj((mem_size + 7) & ~7);
  // memcpy(copy_elts, seq.core_data.ptr, mem_size);
  // return repoint_to_sliced_copy(seq, copy_elts);


  if (bits_tag < INT_BITS_TAG_32) {
    if (bits_tag == INT_BITS_TAG_8) {
      // Works also for unsigned bytes
      void *src = get_seq_elts_ptr_int8_or_uint8(seq);
      uint32 mem_size = round_up(len * sizeof(int8));
      void *dest = grab_mem(dest_var, mem_size);
      memcpy(dest, src, mem_size);
      return repoint_to_sliced_copy(seq, dest);
    }
    else {
      assert(bits_tag == INT_BITS_TAG_16);
      int16 *array = get_seq_elts_ptr_int16(seq);
      int16 min = 0, max = 0;
      for (uint32 i=0 ; i < len ; i++) {
        int16 value = array[i];
        min = value < min ? value : min;
        max = value > max ? value : max;
        if (!is_int8_or_uint8_range(min, max)) {
          uint32 mem_size = round_up(len * sizeof(int16));
          void *dest = grab_mem(dest_var, mem_size);
          memcpy(dest, array, mem_size);
          return repoint_to_sliced_copy(seq, dest);
        }
      }

      uint32 mem_size = round_up(len * sizeof(int8)); // Works also for unsigned bytes
      if (is_uint8_range(min, max)) {
        uint8 *dest = (uint8 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_uint8_sliced_copy(seq, dest);
      }
      else {
        assert(is_int8_range(min, max));
        int8 *dest = (int8 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_int8_sliced_copy(seq, dest);
      }
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
        if (!is_int16_range(min, max)) {
          uint32 mem_size = round_up(len * sizeof(int32));
          void *dest = grab_mem(dest_var, mem_size);
          memcpy(dest, array, mem_size);
          return repoint_to_sliced_copy(seq, dest);
        }
      }

      if (is_uint8_range(min, max)) {
        uint32 mem_size = round_up(len * sizeof(uint8));
        uint8 *dest = (uint8 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_uint8_sliced_copy(seq, dest);
      }
      else if (is_int8_range(min, max)) {
        uint32 mem_size = round_up(len * sizeof(int8));
        int8 *dest = (int8 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_int8_sliced_copy(seq, dest);
      }
      else {
        assert(is_int16_range(min, max));
        uint32 mem_size = round_up(len * sizeof(int16));
        int16 *dest = (int16 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_int16_sliced_copy(seq, dest);

      }
    }
    else {
      assert(bits_tag == INT_BITS_TAG_64);
      int64 *array = get_seq_elts_ptr_int64(seq);
      int64 min = 0, max = 0;
      for (uint32 i=0 ; i < len ; i++) {
        int64 value = array[i];
        min = value < min ? value : min;
        max = value > max ? value : max;
        if (!is_int32_range(min, max)) {
          uint32 mem_size = null_round_up(len * sizeof(int64));
          void *dest = grab_mem(dest_var, mem_size);
          memcpy(dest, array, mem_size);
          return repoint_to_sliced_copy(seq, dest);
        }
      }

      if (is_uint8_range(min, max)) {
        uint32 mem_size = round_up(len * sizeof(uint8));
        uint8 *dest = (uint8 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_uint8_sliced_copy(seq, dest);
      }
      else if (is_int8_range(min, max)) {
        uint32 mem_size = round_up(len * sizeof(int8));
        int8 *dest = (int8 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_int8_sliced_copy(seq, dest);
      }
      else if (is_int16_range(min, max)) {
        uint32 mem_size = round_up(len * sizeof(int16));
        int16 *dest = (int16 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_int16_sliced_copy(seq, dest);
      }
      else {
        assert(is_int32_range(min, max));
        uint32 mem_size = round_up(len * sizeof(int32));
        int32 *dest = (int32 *) grab_mem(dest_var, mem_size);
        for (int i=0 ; i < len ; i++)
          dest[i] = array[i];
        return repoint_to_int32_sliced_copy(seq, dest);
      }
    }
  }

  internal_fail();
}

static OBJ copy_ne_float_seq_to(OBJ seq, void **dest_var) {
  double *elts = (double *) seq.core_data.ptr;
  uint32 len = read_size_field_unchecked(seq);
  uint32 mem_size = round_up(len * sizeof(double));
  void *dest = grab_mem(dest_var, mem_size);
  memcpy(dest, elts, mem_size);
  return repoint_to_sliced_copy(seq, dest);
}

static OBJ copy_ne_bool_seq_to(OBJ seq, void **dest_var) {
  internal_fail();
}

static OBJ copy_ne_seq_to(OBJ seq, void **dest_var) {
  OBJ *elts = (OBJ *) seq.core_data.ptr;
  uint32 len = read_size_field_unchecked(seq);

#ifndef NDEBUG
  bool non_int_found = false;
  bool non_float_found = false;

  for (int i=0 ; i < len ; i++) {
    OBJ elt = elts[i];
    non_int_found |= !is_int(elt);
    non_float_found |= !is_float(elt);
  }

  assert(non_int_found && non_float_found);
#endif

  uint32 mem_size = null_round_up(len * sizeof(OBJ));
  OBJ *dest = (OBJ *) grab_mem(dest_var, mem_size);
  copy_objs_to(elts, len, dest, dest_var);
  return repoint_to_sliced_copy(seq, dest);
}

static OBJ copy_ne_set_to(OBJ obj, void **dest_var) {
  SET_OBJ *ptr = (SET_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  uint32 mem_size = round_up(set_obj_mem_size(size));
  SET_OBJ *copy_ptr = (SET_OBJ *) grab_mem(dest_var, mem_size);
  copy_objs_to(ptr->buffer, size, copy_ptr->buffer, dest_var);
  return repoint_to_copy(obj, copy_ptr);
}

static OBJ copy_ne_map_to(OBJ obj, void **dest_var) {
  if (is_opt_rec(obj)) {
    // void *ptr = get_opt_repr_ptr(obj);
    // uint16 repr_id = get_opt_repr_id(obj);
    // void *copy_ptr = opt_repr_copy(ptr, repr_id);
    // return repoint_to_copy(obj, copy_ptr);
    internal_fail();
  }
  else if (is_array_map(obj)) {
    BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
    uint32 size = read_size_field_unchecked(obj);

    uint32 mem_size = round_up(bin_rel_obj_mem_size(size));
    BIN_REL_OBJ *copy_ptr = (BIN_REL_OBJ *) grab_mem(dest_var, mem_size);

    copy_objs_to(ptr->buffer, 2 * size, copy_ptr->buffer, dest_var);

    uint32 *copy_r2l_index = get_right_to_left_indexes(copy_ptr, size);
    if (index_has_been_built(ptr, size)) {
      uint32 *r2l_index = get_right_to_left_indexes(copy_ptr, size);
      memcpy(copy_r2l_index, r2l_index, size * sizeof(uint32));
    }
    else {
      copy_r2l_index[0] = 0;
      if (size > 1) {
        copy_r2l_index[1] = 0;
        OBJ copy = repoint_to_copy(obj, copy_ptr);
        build_map_right_to_left_sorted_idx_array(copy);
        return copy;
      }
    }

    return repoint_to_copy(obj, copy_ptr);
  }
  else if (is_bin_tree_map(obj)) {
    internal_fail();
  }
}

static OBJ copy_ne_bin_rel_to(OBJ obj, void **dest_var) {
  BIN_REL_OBJ *ptr = (BIN_REL_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  uint32 mem_size = round_up(bin_rel_obj_mem_size(size));
  BIN_REL_OBJ *copy_ptr = (BIN_REL_OBJ *) grab_mem(dest_var, mem_size);

  copy_objs_to(ptr->buffer, 2 * size, copy_ptr->buffer, dest_var);
  memcpy(get_right_to_left_indexes(copy_ptr, size), get_right_to_left_indexes(ptr, size), size * sizeof(uint32));
  return repoint_to_copy(obj, copy_ptr);
}

static OBJ copy_ne_tern_rel_to(OBJ obj, void **dest_var) {
  TERN_REL_OBJ *ptr = (TERN_REL_OBJ *) obj.core_data.ptr;
  uint32 size = read_size_field_unchecked(obj);

  uint32 mem_size = round_up(tern_rel_obj_mem_size(size));
  TERN_REL_OBJ *copy_ptr = (TERN_REL_OBJ *) grab_mem(dest_var, mem_size);

  copy_objs_to(ptr->buffer, 3 * size, copy_ptr->buffer, dest_var);
  memcpy(get_rotated_index(copy_ptr, size, 1), get_rotated_index(ptr, size, 1), 2 * size * sizeof(uint32));
  return repoint_to_copy(obj, copy_ptr);
}

static OBJ copy_ad_hoc_tag_rec_to(OBJ obj, void **dest_var) {
  void *ptr = get_opt_repr_ptr(obj);
  uint16 repr_id = get_opt_repr_id(obj);
  void *copy = *dest_var;
  opt_repr_copy_to_pool(ptr, repr_id, dest_var);
  return repoint_to_copy(obj, copy);
}

static OBJ copy_boxed_obj_to(OBJ obj, void **dest_var) {
  uint32 mem_size = null_round_up(boxed_obj_mem_size());
  BOXED_OBJ *copy_ptr = (BOXED_OBJ *) grab_mem(dest_var, mem_size);

  copy_ptr->obj = copy_obj_to(get_boxed_obj_ptr(obj)->obj, dest_var);
  return repoint_to_copy(obj, copy_ptr);
}

////////////////////////////////////////////////////////////////////////////////

OBJ copy_obj_to(OBJ obj, void **dest_var) {
  if (is_inline_obj(obj))
    return obj;

  switch (get_obj_type(obj)) {
    case TYPE_NE_INT_SEQ:
      return copy_ne_int_seq_to(obj, dest_var);

    case TYPE_NE_FLOAT_SEQ:
      return copy_ne_float_seq_to(obj, dest_var);

    case TYPE_NE_BOOL_SEQ:
      return copy_ne_bool_seq_to(obj, dest_var);

    case TYPE_NE_SEQ:
      return copy_ne_seq_to(obj, dest_var);

    case TYPE_NE_SET:
      return copy_ne_set_to(obj, dest_var);

    case TYPE_NE_MAP:
      return copy_ne_map_to(obj, dest_var);

    case TYPE_NE_BIN_REL:
      return copy_ne_bin_rel_to(obj, dest_var);

    case TYPE_NE_TERN_REL:
      return copy_ne_tern_rel_to(obj, dest_var);

    case TYPE_AD_HOC_TAG_REC:
      return copy_ad_hoc_tag_rec_to(obj, dest_var);

    case TYPE_BOXED_OBJ:
      return copy_boxed_obj_to(obj, dest_var);

    default:
      internal_fail();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ copy_to_pool(STATE_MEM_POOL *mem_pool, OBJ obj) {
  if (is_inline_obj(obj))
    return obj;

  uint32 total_mem_size = obj_mem_size(obj);
  assert(total_mem_size % 8 == 0);

#ifndef NDEBUG
  if (total_mem_size < 64 * 1024) {
    uint8 test_mem_0[128 * 1024];
    uint8 test_mem_1[128 * 1024];

    uint32 start_idx = 32 * 1024;
    uint32 end_idx = start_idx + total_mem_size;

    for (int i=0 ; i < 128 * 1024 ; i++) {
      test_mem_0[i] = 0;
      test_mem_1[i] = 0xFF;
    }

    void *test_mem_var = test_mem_0 + start_idx;
    OBJ test_copy_0 = copy_obj_to(obj, &test_mem_var);
    assert(test_mem_var == test_mem_0 + end_idx);
    assert(are_eq(obj, test_copy_0));

    test_mem_var = test_mem_1 + start_idx;
    OBJ test_copy_1 = copy_obj_to(obj, &test_mem_var);
    assert(test_mem_var == test_mem_1 + end_idx);
    assert(are_eq(obj, test_copy_1));

    assert(are_eq(test_copy_0, test_copy_1));

    for (int i=0 ; i < 128 * 1024 ; i++)
      if (i < start_idx || i >= end_idx) {
        assert(test_mem_0[i] == 0);
        assert(test_mem_1[i] == 0xFF);
      }
  }
#endif

  void *mem_start = alloc_state_mem_block(mem_pool, total_mem_size);
  void *mem_var = mem_start;
  OBJ copy = copy_obj_to(obj, &mem_var);
  assert(mem_var == ((uint8 *) mem_start) + total_mem_size);
  assert(obj_mem_size(copy) == total_mem_size);
  return copy;
}

void remove_from_pool(STATE_MEM_POOL *mem_pool, OBJ obj) {
  if (!is_inline_obj(obj)) {
    uint32 total_mem_size = obj_mem_size(obj);
    assert(total_mem_size % 8 == 0);

    release_state_mem_block(mem_pool, obj.core_data.ptr, total_mem_size);
  }
}