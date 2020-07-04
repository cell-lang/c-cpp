#include "lib.h"
#include "extern.h"


// The array mustn't contain duplicates
uint32 find_obj(OBJ *sorted_array, uint32 len, OBJ obj, bool &found) {
  if (len > 0) {
    if (is_inline_obj(obj)) {
      if (len <= 12) {
        for (int i=0 ; i < len ; i++)
          if (are_shallow_eq(sorted_array[i], obj)) {
            found = true;
            return i;
          }
        found = false;
        return -1;
      }

      OBJ last_obj = sorted_array[len - 1];
      if (!is_inline_obj(last_obj))
        goto std_alg;

      int64 low_idx = 0;
      int64 high_idx = len - 1;

      while (low_idx <= high_idx) {
        int64 middle_idx = (low_idx + high_idx) / 2;
        OBJ middle_obj = sorted_array[middle_idx];

        int cr = shallow_cmp(obj, middle_obj);

        if (cr == 0) {
          found = true;
          return middle_idx;
        }

        if (cr > 0)
          high_idx = middle_idx - 1;
        else
          low_idx = middle_idx + 1;
      }

      found = false;
      return -1;
    }
  }

std_alg:
  int64 low_idx = 0;
  int64 high_idx = len - 1;

  while (low_idx <= high_idx) {
    int64 middle_idx = (low_idx + high_idx) / 2;
    OBJ middle_obj = sorted_array[middle_idx];

    int cr = comp_objs(obj, middle_obj);

    if (cr == 0) {
      found = true;
      return middle_idx;
    }

    if (cr > 0)
      high_idx = middle_idx - 1;
    else
      low_idx = middle_idx + 1;
  }

  found = false;
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

uint32 count_at_start(uint32 *sorted_idx_array, OBJ *values, uint32 len, OBJ obj) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len && are_eq(obj, values[sorted_idx_array[c]]))
    c++;
  return c;
}

uint32 count_at_end(uint32 *sorted_idx_array, OBJ *values, uint32 len, OBJ obj) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len && are_eq(obj, values[sorted_idx_array[len-1-c]]))
    c++;
  return c;
}

uint32 find_idxs_range(uint32 *sorted_idx_array, OBJ *values, uint32 len, OBJ obj, uint32 &count) {
  int64 low_idx = 0;
  int64 high_idx = len - 1;

  while (low_idx <= high_idx) {
    int64 middle_idx = (low_idx + high_idx) / 2;
    OBJ middle_obj = values[sorted_idx_array[middle_idx]];

    int cr = comp_objs(obj, middle_obj);

    if (cr == 0) {
      int count_up = count_at_start(sorted_idx_array + middle_idx + 1, values, len - middle_idx - 1, obj);
      int count_down = count_at_end(sorted_idx_array, values, middle_idx, obj);
      count = 1 + count_up + count_down;
      return middle_idx - count_down;
    }

    if (cr > 0)
      high_idx = middle_idx - 1;
    else
      low_idx = middle_idx + 1;
  }

  count = 0;
  return INVALID_INDEX;
}

////////////////////////////////////////////////////////////////////////////////

uint32 count_at_start(OBJ *sorted_array, uint32 len, OBJ obj) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len && are_eq(obj, sorted_array[c]))
    c++;
  return c;
}

uint32 count_at_end(OBJ *sorted_array, uint32 len, OBJ obj) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len && are_eq(obj, sorted_array[len-1-c]))
    c++;
  return c;
}

uint32 find_objs_range(OBJ *sorted_array, uint32 len, OBJ obj, uint32 &count) {
  int64 low_idx = 0;
  int64 high_idx = len - 1;

  while (low_idx <= high_idx) {
    int64 middle_idx = (low_idx + high_idx) / 2;
    OBJ middle_obj = sorted_array[middle_idx];

    int cr = comp_objs(obj, middle_obj);

    if (cr == 0) {
      int count_up = count_at_start(sorted_array + middle_idx + 1, len - middle_idx - 1, obj);
      int count_down = count_at_end(sorted_array, middle_idx, obj);
      count = 1 + count_up + count_down;
      return middle_idx - count_down;
    }

    if (cr > 0)
      high_idx = middle_idx - 1;
    else
      low_idx = middle_idx + 1;
  }

  count = 0;
  return INVALID_INDEX;
}

////////////////////////////////////////////////////////////////////////////////

uint32 count_at_start(OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len && are_eq(major_arg, major_col[c]) && are_eq(minor_arg, minor_col[c]))
    c++;
  return c;
}

uint32 count_at_end(OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len && are_eq(major_arg, major_col[len-1-c]) && are_eq(minor_arg, minor_col[len-1-c]))
    c++;
  return c;
}

uint32 find_objs_range(OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg, uint32 &count) {
  int64 low_idx = 0;
  int64 high_idx = len - 1;

  while (low_idx <= high_idx) {
    int64 idx = (low_idx + high_idx) / 2;

    int cr = comp_objs(major_arg, major_col[idx]);
    if (cr == 0)
      cr = comp_objs(minor_arg, minor_col[idx]);

    if (cr == 0) {
      int count_up = count_at_start(major_col+idx+1, minor_col+idx+1, len-idx-1, major_arg, minor_arg);
      int count_down = count_at_end(major_col, minor_col, idx, major_arg, minor_arg);
      count = 1 + count_up + count_down;
      return idx - count_down;
    }

    if (cr > 0)
      high_idx = idx - 1;
    else
      low_idx = idx + 1;
  }

  count = 0;
  return INVALID_INDEX;
}

////////////////////////////////////////////////////////////////////////////////

uint32 count_at_start(uint32 *index, OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len) {
    uint32 idx = index[c];
    if (are_eq(major_arg, major_col[idx]) && are_eq(minor_arg, minor_col[idx]))
      c++;
    else
      break;
  }
  return c;
}

uint32 count_at_end(uint32 *index, OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg) {
  //## IMPLEMENT FOR REAL...
  int c = 0;
  while (c < len) {
    uint32 idx = index[len-c-1];
    if (are_eq(major_arg, major_col[idx]) and are_eq(minor_arg, minor_col[idx]))
      c++;
    else
      break;
  }
  return c;
}

uint32 find_idxs_range(uint32 *index, OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg, uint32 &count) {
  int64 low_idx = 0;
  int64 high_idx = len - 1;

  while (low_idx <= high_idx) {
    int64 idx = (low_idx + high_idx) / 2;
    uint32 dr_idx = index[idx];

    int cr = comp_objs(major_arg, major_col[dr_idx]);
    if (cr == 0)
      cr = comp_objs(minor_arg, minor_col[dr_idx]);

    if (cr == 0) {
      int count_up = count_at_start(index+idx+1, major_col, minor_col, len-idx-1, major_arg, minor_arg);
      int count_down = count_at_end(index, major_col, minor_col, idx, major_arg, minor_arg);
      count = 1 + count_up + count_down;
      return idx - count_down;
    }

    if (cr > 0)
      high_idx = idx - 1;
    else
      low_idx = idx + 1;
  }

  count = 0;
  return INVALID_INDEX;
}

////////////////////////////////////////////////////////////////////////////////

uint32 adjust_map_with_duplicate_keys(OBJ *keys, OBJ *values, uint32 size) {
  assert(size >= 2);

  OBJ prev_key = keys[0];
  OBJ prev_val = values[0];

  uint32 next_slot_idx = 1;
  uint32 i = 1;
  do {
    OBJ curr_key = keys[i];
    OBJ curr_val = values[i];

    if (are_eq(curr_key, prev_key)) {
      if (comp_objs(curr_val, prev_val) != 0)
        soft_fail("Map contains duplicate keys");
    }
    else {
      keys[next_slot_idx] = curr_key;
      values[next_slot_idx] = curr_val;
      next_slot_idx++;
      prev_key = curr_key;
      prev_val = curr_val;
    }
  } while (++i < size);

  return next_slot_idx;
}

uint32 sort_and_check_no_dups(OBJ *keys, OBJ *values, uint32 size) {
  if (size < 2)
    return size;

  uint32 *idxs = new_uint32_array(size);
  index_sort(idxs, keys, size);

  for (uint32 i=0 ; i < size ; i++)
    if (idxs[i] != i) {
      OBJ key = keys[i];
      OBJ value = values[i];

      for (uint32 j = i ; ; ) {
        uint32 k = idxs[j];
        idxs[j] = j;

        if (k == i) {
          keys[j]   = key;
          values[j] = value;
          break;
        }
        else {
          keys[j]   = keys[k];
          values[j] = values[k];
          j = k;
        }
      }
    }

  OBJ prev_key = keys[0];
  for (uint32 i=1 ; i < size ; i++) {
    OBJ curr_key = keys[i];
    if (are_eq(curr_key, prev_key)) {
      uint32 offset = i - 1;
      return offset + adjust_map_with_duplicate_keys(keys+offset, values+offset, size-offset);
    }
    prev_key = curr_key;
  }

  return size;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int slow_cmp_objs(OBJ obj1, OBJ obj2);


__attribute__ ((noinline)) int slow_cmp_same_type_opt_recs(void *ptr1, void *ptr2, uint16 repr_id, uint32 size) {
  uint32 count;
  uint16 *labels = opt_repr_get_fields(repr_id, count);
  assert(count == size);

  for (int i=0 ; i < size ; i++) {
    uint16 label = labels[i];
    OBJ value1 = opt_repr_lookup_field(ptr1, repr_id, label);
    OBJ value2 = opt_repr_lookup_field(ptr2, repr_id, label);

    int res = slow_cmp_objs(value1, value2);
    if (res != 0)
      return res;
  }

  return 0;
}


int slow_cmp_bin_rels_slow(OBJ rel1, OBJ rel2) {
  assert(get_size(rel1) == get_size(rel2));

  BIN_REL_ITER key_it1, key_it2, value_it1, value_it2;

  get_bin_rel_iter(key_it1, rel1);
  get_bin_rel_iter(key_it2, rel2);

  value_it1 = key_it1;
  value_it2 = key_it2;

  while (!is_out_of_range(key_it1)) {
    assert(!is_out_of_range(key_it2));

    OBJ left_arg_1 = get_curr_left_arg(key_it1);
    OBJ left_arg_2 = get_curr_left_arg(key_it2);

    int res = slow_cmp_objs(left_arg_1, left_arg_2);
    if (res != 0)
      return res;

    move_forward(key_it1);
    move_forward(key_it2);
  }

  assert(is_out_of_range(key_it2));

  while (!is_out_of_range(value_it1)) {
    assert(!is_out_of_range(value_it2));

    OBJ right_arg_1 = get_curr_right_arg(value_it1);
    OBJ right_arg_2 = get_curr_right_arg(value_it2);

    int res = slow_cmp_objs(right_arg_1, right_arg_2);
    if (res != 0)
      return res;

    move_forward(value_it1);
    move_forward(value_it2);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Returns:   > 0     if obj1 < obj2
//              0     if obj1 = obj2
//            < 0     if obj1 > obj2

int slow_cmp_objs(OBJ obj1, OBJ obj2) {
  if (are_shallow_eq(obj1, obj2))
    return 0;

  bool is_inline_1 = is_inline_obj(obj1);
  bool is_inline_2 = is_inline_obj(obj2);

  if (is_inline_1)
    if (is_inline_2)
      return shallow_cmp(obj1, obj2);
    else
      return 1;
  else if (is_inline_2)
    return -1;

  OBJ_TYPE type1 = get_logical_type(obj1);
  OBJ_TYPE type2 = get_logical_type(obj2);

  if (type1 != type2)
    return type2 - type1;

  uint32 count = 0;
  OBJ *elems1 = 0;
  OBJ *elems2 = 0;

  switch (type1) {
    case TYPE_NE_SEQ: {
      uint32 len1 = get_seq_length(obj1);
      uint32 len2 = get_seq_length(obj2);
      if (len1 != len2)
        return len2 - len1; //## BUG BUG BUG
      count = len1;
      elems1 = get_seq_buffer_ptr(obj1);
      elems2 = get_seq_buffer_ptr(obj2);
      break;
    }

    case TYPE_NE_SET: {
      uint32 size1 = get_set_size(obj1);
      uint32 size2 = get_set_size(obj2);
      if (size1 != size2)
        return size2 - size1; //## BUG BUG BUG
      SET_OBJ *set1 = get_set_ptr(obj1);
      SET_OBJ *set2 = get_set_ptr(obj2);
      count = size1;
      elems1 = set1->buffer;
      elems2 = set2->buffer;
      break;
    }

    case TYPE_NE_BIN_REL: {
      if (is_opt_rec(obj1)) {
        void *ptr1 = get_opt_repr_ptr(obj1);
        uint16 repr_id_1 = get_opt_repr_id(obj1);
        uint32 size1 = opt_repr_get_fields_count(ptr1, repr_id_1);

        if (is_opt_rec(obj2)) {
          void *ptr2 = get_opt_repr_ptr(obj2);
          uint16 repr_id_2 = get_opt_repr_id(obj2);
          uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);
          if (size1 != size2)
            return size2 - size1; //## BUG BUG BUG
          else if (repr_id_1 == repr_id_2 && !opt_repr_may_have_opt_fields(repr_id_1))
            return slow_cmp_same_type_opt_recs(ptr1, ptr2, repr_id_1, size1);
          else
            // return cmp_opt_recs(ptr1, repr_id_1, ptr2, repr_id_2, size1);
            return slow_cmp_bin_rels_slow(obj1, obj2);
        }
        else {
          BIN_REL_OBJ *rel2 = get_bin_rel_ptr(obj2);
          uint32 size2 = rel2->size;
          if (size1 != size2)
            return size2 - size1; //## BUG BUG BUG
          else
            // return cmp_opt_rec_bin_rel(ptr1, repr_id_1, rel2);
            return slow_cmp_bin_rels_slow(obj1, obj2);
        }
      }
      else if (is_opt_rec(obj2)) {
        BIN_REL_OBJ *rel1 = get_bin_rel_ptr(obj1);
        uint32 size1 = rel1->size;

        void *ptr2 = get_opt_repr_ptr(obj2);
        uint16 repr_id_2 = get_opt_repr_id(obj2);
        uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);

        if (size1 != size2)
          return size2 - size1; //## BUG BUG BUG
        else
          // return -cmp_opt_rec_bin_rel(ptr2, repr_id_2, rel1);
          return slow_cmp_bin_rels_slow(obj1, obj2);
      }

      BIN_REL_OBJ *rel1 = get_bin_rel_ptr(obj1);
      BIN_REL_OBJ *rel2 = get_bin_rel_ptr(obj2);
      uint32 size1 = rel1->size;
      uint32 size2 = rel2->size;
      if (size1 != size2)
        return size2 - size1; //## BUG BUG BUG
      count = 2 * size1;
      elems1 = rel1->buffer;
      elems2 = rel2->buffer;
      break;
    }

    case TYPE_NE_TERN_REL: {
      TERN_REL_OBJ *rel1 = get_tern_rel_ptr(obj1);
      TERN_REL_OBJ *rel2 = get_tern_rel_ptr(obj2);
      uint32 size1 = rel1->size;
      uint32 size2 = rel2->size;
      if (size1 != size2)
        return size2 - size1; //## BUG BUG BUG
      count = 3 * size1;
      elems1 = rel1->buffer;
      elems2 = rel2->buffer;
      break;
    }

    case TYPE_TAG_OBJ: {
      uint16 tag_id_1 = get_tag_id(obj1);
      uint16 tag_id_2 = get_tag_id(obj2);
      if (tag_id_1 != tag_id_2)
        return tag_id_2 - tag_id_1;
      return slow_cmp_objs(get_inner_obj(obj1), get_inner_obj(obj2));
    }

    default:
      internal_fail();
  }

  for (uint32 i=0 ; i < count ; i++) {
    int cr = slow_cmp_objs(elems1[i], elems2[i]);
    if (cr != 0)
      return cr;
  }

  return 0;
}
