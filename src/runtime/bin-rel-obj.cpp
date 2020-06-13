#include "lib.h"
#include "extern.h"


void build_map_right_to_left_sorted_idx_array(OBJ map) {
  assert(get_physical_type(map) == TYPE_NE_MAP);

  BIN_REL_OBJ *ptr = get_bin_rel_ptr(map);
  uint32 *rev_idxs = get_right_to_left_indexes(ptr);
  if (rev_idxs[0] != INVALID_INDEX)
    return;
  stable_index_sort(rev_idxs, get_right_col_array_ptr(ptr), ptr->size);

#ifndef NDEBUG
  uint32 size = ptr->size;
  OBJ *left_col = get_left_col_array_ptr(ptr);
  OBJ *right_col = get_right_col_array_ptr(ptr);

  for (uint32 i=1 ; i < size ; i++) {
    int cr_M = comp_objs(left_col[i-1], left_col[i]);
    int cr_m = comp_objs(right_col[i-1], right_col[i]);
    assert(cr_M > 0 | (cr_M == 0 & cr_m > 0));
  }

  for (uint32 i=1 ; i < size ; i++) {
    uint32 curr_idx = rev_idxs[i];
    uint32 prev_idx = rev_idxs[i-1];
    int cr_M = comp_objs(right_col[prev_idx], right_col[curr_idx]);
    int cr_m = comp_objs(left_col[prev_idx], left_col[curr_idx]);
    assert(cr_M > 0 | (cr_M == 0 & cr_m > 0));
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_bin_rel(OBJ *vals1, OBJ *vals2, uint32 size) {
  if (size == 0)
    return make_empty_rel();

    // Creating the array of indexes sorted by column 1, column 2, index
  uint32 *index = new_uint32_array(size);
  index_sort(index, vals1, vals2, size);

  // Counting the number of unique tuples and unique values in the left column,
  // and releasing unnecessary objects
  uint32 unique_tuples = 1;
  uint32 prev_idx = index[0];
  bool left_col_is_unique = true;
  for (uint32 i=1 ; i < size ; i++) {
    uint32 idx = index[i];
    if (comp_objs(vals1[idx], vals1[prev_idx]) != 0) {
      // The current left column value is new, so the tuple is new too.
      unique_tuples++;
      prev_idx = idx;
    }
    else if (comp_objs(vals2[idx], vals2[prev_idx]) != 0) {
      // The current left column value is unchanged, but the value in the right column is new, so the tuple is new too
      unique_tuples++;
      prev_idx = idx;
      left_col_is_unique = false;
    }
    else {
      // Duplicate tuple, marking it as such
      index[i] = INVALID_INDEX;
    }
  }

  // Creating the new binary relation object
  BIN_REL_OBJ *rel = new_bin_rel(unique_tuples);

  OBJ *left_col = get_left_col_array_ptr(rel);
  OBJ *right_col = get_right_col_array_ptr(rel);

  // Copying the sorted, non-duplicate tuples into their final destination
  uint32 count = 0;
  for (uint32 i=0 ; i < size ; i++) {
    uint32 idx = index[i];
    if (idx != INVALID_INDEX) {
      left_col[count] = vals1[idx];
      right_col[count] = vals2[idx];
      count++;
    }
  }
  assert(count == unique_tuples);

  // Creating the reverse index
  uint32 *rev_index = get_right_to_left_indexes(rel);
  stable_index_sort(rev_index, right_col, count);

#ifndef NDEBUG
  for (uint32 i=1 ; i < count ; i++) {
    int cr_M = comp_objs(left_col[i-1], left_col[i]);
    int cr_m = comp_objs(right_col[i-1], right_col[i]);
    assert(cr_M > 0 | (cr_M == 0 & cr_m > 0));
  }

  for (uint32 i=1 ; i < count ; i++) {
    uint32 curr_idx = rev_index[i];
    uint32 prev_idx = rev_index[i-1];
    int cr_M = comp_objs(right_col[prev_idx], right_col[curr_idx]);
    int cr_m = comp_objs(left_col[prev_idx], left_col[curr_idx]);
    assert(cr_M > 0 | (cr_M == 0 & cr_m > 0));
  }
#endif

  return left_col_is_unique ? make_log_map(rel) : make_bin_rel(rel);
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_bin_rel(STREAM &stream1, STREAM &stream2) {
  assert(stream1.count == stream2.count);

  if (stream1.count == 0)
    return make_empty_rel();

  return build_bin_rel(stream1.buffer, stream2.buffer, stream1.count);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ build_map(OBJ *keys, OBJ *values, uint32 size) {
  if (size == 0)
    return make_empty_rel();

  uint32 actual_size = sort_and_check_no_dups(keys, values, size);

  BIN_REL_OBJ *map = new_map(actual_size);
  OBJ *ks  = map->buffer;
  OBJ *vs  = ks + map->size;

  for (uint32 i=0 ; i < actual_size ; i++) {
    ks[i] = keys[i];
    vs[i] = values[i];
  }

  return make_map(map);
}

OBJ build_map(STREAM &key_stream, STREAM &value_stream) {
  assert(key_stream.count == value_stream.count);

  if (key_stream.count == 0)
    return make_empty_rel();

  return build_map(key_stream.buffer, value_stream.buffer, key_stream.count);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void get_bin_rel_null_iter(BIN_REL_ITER &it) {
  it.iter.bin_rel.left_col = NULL;   // Not strictly necessary
  it.iter.bin_rel.right_col = NULL;  // Not strictly necessary
  it.iter.bin_rel.rev_idxs = NULL;
  it.idx = 0;
  it.end = 0;
  it.type = BIN_REL_ITER::BRIT_BIN_REL;
}

void get_bin_rel_iter(BIN_REL_ITER &it, OBJ rel) {
  assert(is_bin_rel(rel));

  if (is_opt_rec(rel)) {
    void *ptr = get_opt_repr_ptr(rel);
    uint16 repr_id = get_opt_repr_id(rel);

    uint32 count;
    uint16 *fields = opt_repr_get_fields(repr_id, count);

    uint32 idx = 0;
    while (!opt_repr_has_field(ptr, repr_id, fields[idx]))
      idx++;

    it.iter.opt_rec.fields = fields;
    it.iter.opt_rec.ptr = ptr;
    it.iter.opt_rec.repr_id = repr_id;
    it.idx = idx;
    it.end = count;
    it.type = BIN_REL_ITER::BRIT_OPT_REC;
  }
  else if (!is_empty_rel(rel)) {
    BIN_REL_OBJ *ptr = get_bin_rel_ptr(rel);
    it.iter.bin_rel.left_col = get_left_col_array_ptr(ptr);
    it.iter.bin_rel.right_col = get_right_col_array_ptr(ptr);
    it.iter.bin_rel.rev_idxs = NULL;
    it.idx = 0;
    it.end = ptr->size;
    it.type = BIN_REL_ITER::BRIT_BIN_REL;
  }
  else
    get_bin_rel_null_iter(it);
}

void get_bin_rel_iter_1(BIN_REL_ITER &it, OBJ rel, OBJ arg1) {
  assert(is_bin_rel(rel));

  if (is_opt_rec(rel)) {
    if (is_symb(arg1)) {
      uint16 symb_id = get_symb_id(arg1);

      void *ptr = get_opt_repr_ptr(rel);
      uint16 repr_id = get_opt_repr_id(rel);

      if (opt_repr_has_field(ptr, repr_id, symb_id)) {
        uint32 count;
        uint16 *fields = opt_repr_get_fields(repr_id, count);
        while (*fields != symb_id)
          fields++;

        it.iter.opt_rec.fields = fields;
        it.iter.opt_rec.ptr = ptr;
        it.iter.opt_rec.repr_id = repr_id;
        it.idx = 0;
        it.end = 1;
        it.type = BIN_REL_ITER::BRIT_OPT_REC;
      }
    }
  }
  else if (is_ne_bin_rel(rel)) {
    BIN_REL_OBJ *ptr = get_bin_rel_ptr(rel);
    uint32 size = ptr->size;
    OBJ *left_col = get_left_col_array_ptr(ptr);

    uint32 count;
    uint32 first = find_objs_range(left_col, size, arg1, count);

    if (count > 0) {
      it.iter.bin_rel.left_col = left_col;
      it.iter.bin_rel.right_col = get_right_col_array_ptr(ptr);
      it.iter.bin_rel.rev_idxs = NULL;
      it.idx = first;
      it.end = first + count;
      it.type = BIN_REL_ITER::BRIT_BIN_REL;
      return;
    }
  }

  get_bin_rel_null_iter(it);
}

void get_bin_rel_iter_2(BIN_REL_ITER &it, OBJ rel, OBJ arg2) {
  assert(is_bin_rel(rel));

  if (is_opt_rec(rel)) {
    void *ptr = get_opt_repr_ptr(rel);
    uint16 repr_id = get_opt_repr_id(rel);

    uint32 count;
    uint16 *fields = opt_repr_get_fields(repr_id, count);

    uint32 buffer[1024];
    uint32 *idxs = count > 1024 ? new_uint32_array(count) : buffer;

    uint32 found = 0;
    for (int i=0 ; i < count ; i++) {
      uint16 field = fields[i];
      if (opt_repr_has_field(ptr, repr_id, field)) {
        OBJ value = opt_repr_lookup_field(ptr, repr_id, field);
        if (are_eq(value, arg2))
          idxs[found++] = i;
      }
    }

    if (found > 0) {
      if (found > 1) {
        uint16 *fields_subset = new_uint16_array(found);
        for (int i=0 ; i < found ; i++)
          fields_subset[i] = fields[idxs[i]];
        it.iter.opt_rec.fields = fields_subset;
      }
      else
        it.iter.opt_rec.fields = fields + idxs[0];

      it.iter.opt_rec.ptr = ptr;
      it.iter.opt_rec.repr_id = repr_id;
      it.idx = 0;
      it.end = found;
      it.type = BIN_REL_ITER::BRIT_OPT_REC;
    }
  }
  else if (is_ne_bin_rel(rel)) {
    if (get_physical_type(rel) == TYPE_NE_MAP)
      build_map_right_to_left_sorted_idx_array(rel);

    BIN_REL_OBJ *ptr = get_bin_rel_ptr(rel);
    uint32 size = ptr->size;
    OBJ *right_col = get_right_col_array_ptr(ptr);
    uint32 *rev_idxs = get_right_to_left_indexes(ptr);

    uint32 count;
    uint32 first = find_idxs_range(rev_idxs, right_col, size, arg2, count);

    if (count > 0) {
      it.iter.bin_rel.left_col = get_left_col_array_ptr(ptr);
      it.iter.bin_rel.right_col = right_col;
      it.iter.bin_rel.rev_idxs = rev_idxs;
      it.idx = first;
      it.end = first + count;
      it.type = BIN_REL_ITER::BRIT_BIN_REL;
      return;
    }
  }

  get_bin_rel_null_iter(it);
}
