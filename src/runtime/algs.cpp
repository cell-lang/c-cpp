#include "lib.h"


bool sorted_u32_array_contains(uint32 *array, uint32 len, uint32 value) {
  assert(len > 0);
  for (int i=1 ; i < len ; i++)
    assert(array[i - 1] <= array[i]);

  uint32 low_idx = 0;
  uint32 high_idx = len - 1;

  while (low_idx <= high_idx) {
    uint32 mid_idx = (high_idx + low_idx) / 2; //## THIS IS BUGGY
    uint32 mid_value = array[mid_idx];
    if (mid_value == value)
      return true;
    if (mid_value < value)
      low_idx = mid_idx + 1;
    else
      high_idx = mid_idx - 1;
  }

  return false;
}

bool sorted_u64_array_contains(uint64 *array, uint32 len, uint64 value) {
  assert(len > 0);
  for (int i=1 ; i < len ; i++)
    assert(array[i - 1] <= array[i]);

  uint32 low_idx = 0;
  uint32 high_idx = len - 1;

  while (low_idx <= high_idx) {
    uint32 mid_idx = (high_idx + low_idx) / 2; //## THIS IS BUGGY
    uint64 mid_value = array[mid_idx];
    if (mid_value == value)
      return true;
    if (mid_value < value)
      low_idx = mid_idx + 1;
    else
      high_idx = mid_idx - 1;
  }

  return false;
}

bool sorted_3u32_array_contains(uint32 *array, uint32 len, uint32 value1, uint32 value2, uint32 value3) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

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
  index_sort(idxs, size, keys);

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
