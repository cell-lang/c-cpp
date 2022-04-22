#include "lib.h"


bool contains(OBJ set, OBJ elt) {
  if (is_empty_rel(set))
    return false;

  uint32 size = read_size_field(set);

  bool found;
  if (is_array_set(set)) {
    OBJ *elts = get_set_elts_ptr(set);
    find_obj(elts, size, elt, found);
  }
  else {
    MIXED_REPR_SET_OBJ *mixed_repr_ptr = get_mixed_repr_set_ptr(set);
    SET_OBJ *ptr = mixed_repr_ptr->array_repr;
    if (ptr != NULL)
      find_obj(ptr->buffer, size, elt, found);
    else
      found = tree_set_contains(mixed_repr_ptr->tree_repr, elt);
  }
  return found;
}

////////////////////////////////////////////////////////////////////////////////

bool contains_br(OBJ rel, OBJ arg0, OBJ arg1) {
  if (is_empty_rel(rel))
    return false;

  if (is_opt_rec(rel)) {
    if (!is_symb(arg0))
      return false;

    uint16 field_id = get_symb_id(arg0);

    void *ptr = get_opt_repr_ptr(rel);
    uint16 repr_id = get_opt_repr_id(rel);

    if (!opt_repr_has_field(ptr, repr_id, field_id))
      return false;

    OBJ value = opt_repr_lookup_field(ptr, repr_id, field_id);
    return are_eq(arg1, value);
  }

  BIN_REL_OBJ *ptr;
  if (is_ne_map(rel) && is_mixed_repr_map(rel)) {
    MIXED_REPR_MAP_OBJ *mixed_repr_ptr = get_mixed_repr_map_ptr(rel);
    ptr = mixed_repr_ptr->array_repr;
    if (ptr == NULL)
      return tree_map_contains(mixed_repr_ptr->tree_repr, arg0, arg1);
  }
  else
    ptr = get_bin_rel_ptr(rel);

  uint32 size = read_size_field(rel);
  OBJ *left_col = get_left_col_array_ptr(ptr);
  OBJ *right_col = get_right_col_array_ptr(ptr, size);

  if (is_ne_map(rel)) {
    bool found;
    uint32 idx = find_obj(left_col, size, arg0, found);
    if (!found)
      return false;
    return are_eq(right_col[idx], arg1);
  }

  uint32 count;
  uint32 idx = find_objs_range(left_col, size, arg0, count);
  if (count == 0)
    return false;
  bool found;
  find_obj(right_col+idx, count, arg1, found);
  return found;
}

bool contains_br_1(OBJ rel, OBJ arg1) {
  if (is_empty_rel(rel))
    return false;

  if (is_opt_rec(rel)) {
    if (is_symb(arg1))
      return opt_repr_has_field(get_opt_repr_ptr(rel), get_opt_repr_id(rel), get_symb_id(arg1));
    else
      return false;
  }

  BIN_REL_OBJ *ptr;
  if (is_ne_map(rel) && is_mixed_repr_map(rel)) {
    MIXED_REPR_MAP_OBJ *mixed_repr_ptr = get_mixed_repr_map_ptr(rel);
    ptr = mixed_repr_ptr->array_repr;
    if (ptr == NULL)
      return tree_map_contains_key(mixed_repr_ptr->tree_repr, arg1);
  }
  else
    ptr = get_bin_rel_ptr(rel);

  uint32 size = read_size_field(rel);
  OBJ *left_col = get_left_col_array_ptr(ptr);

  if (is_ne_map(rel)) {
    bool found;
    uint32 idx = find_obj(left_col, size, arg1, found);
    return found;
  }

  uint32 count;
  uint32 idx = find_objs_range(left_col, size, arg1, count);
  return count > 0;
}

bool contains_br_2(OBJ rel, OBJ arg2) {
  if (is_opt_rec(rel)) {
    void *ptr = get_opt_repr_ptr(rel);
    uint16 repr_id = get_opt_repr_id(rel);

    uint32 count;
    uint16 *symbs = opt_repr_get_fields(repr_id, count);

    for (int i=0 ; i < count ; i++)
      if (opt_repr_has_field(ptr, repr_id, symbs[i])) {
        OBJ value = opt_repr_lookup_field(ptr, repr_id, symbs[i]);
        if (are_eq(value, arg2))
          return true;
      }

    return false;
  }

  BIN_REL_ITER it;
  get_bin_rel_iter_2(it, rel, arg2);
  return !is_out_of_range(it);
}

////////////////////////////////////////////////////////////////////////////////

bool contains_tr(OBJ rel, OBJ arg1, OBJ arg2, OBJ arg3) {
  assert(is_tern_rel(rel));

  if (is_empty_rel(rel))
    return false;

  TERN_REL_OBJ *ptr = get_tern_rel_ptr(rel);
  uint32 size = read_size_field(rel);
  OBJ *col1 = get_col_array_ptr(ptr, size, 0);

  uint32 count;
  uint32 first = find_objs_range(col1, size, arg1, count);
  if (count == 0)
    return false;

  OBJ *col2 = get_col_array_ptr(ptr, size, 1);

  first = first + find_objs_range(col2+first, count, arg2, count);
  if (count == 0)
    return false;

  OBJ *col3 = get_col_array_ptr(ptr, size, 2);

  bool found;
  find_obj(col3+first, count, arg3, found);
  return found;
}

bool contains_tr_1(OBJ rel, OBJ arg1) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 0, arg1);
  return !is_out_of_range(it);
}

bool contains_tr_2(OBJ rel, OBJ arg2) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 1, arg2);
  return !is_out_of_range(it);
}

bool contains_tr_3(OBJ rel, OBJ arg3) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 2, arg3);
  return !is_out_of_range(it);
}

bool contains_tr_12(OBJ rel, OBJ arg1, OBJ arg2) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 0, arg1, arg2);
  return !is_out_of_range(it);
}

bool contains_tr_13(OBJ rel, OBJ arg1, OBJ arg3) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 2, arg3, arg1);
  return !is_out_of_range(it);
}

bool contains_tr_23(OBJ rel, OBJ arg2, OBJ arg3) {
  TERN_REL_ITER it;
  get_tern_rel_iter_by(it, rel, 1, arg2, arg3);
  return !is_out_of_range(it);
}

////////////////////////////////////////////////////////////////////////////////

bool has_field(OBJ rec_or_tag_rec, uint16 field_id) {
  if (is_opt_rec_or_tag_rec(rec_or_tag_rec)) {
    void *ptr = get_opt_repr_ptr(rec_or_tag_rec);
    uint16 repr_id = get_opt_repr_id(rec_or_tag_rec);
    return opt_repr_has_field(ptr, repr_id, field_id);
  }

  OBJ rec = is_tag_obj(rec_or_tag_rec) ? get_inner_obj(rec_or_tag_rec) : rec_or_tag_rec;

  if (!is_empty_rel(rec)) {
    BIN_REL_OBJ *ptr = rearrange_if_needed_and_get_bin_rel_ptr(rec);
    uint32 size = read_size_field(rec);
    OBJ *keys = ptr->buffer;
    for (uint32 i=0 ; i < size ; i++)
      if (is_symb(keys[i], field_id))
        return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

OBJ get_curr_left_arg(BIN_REL_ITER &it) {
  assert(!is_out_of_range(it));

  if (it.type == BIN_REL_ITER::BRIT_BIN_REL) {
    uint32 idx = it.iter.bin_rel.rev_idxs != NULL ? it.iter.bin_rel.rev_idxs[it.idx] : it.idx;
    return it.iter.bin_rel.left_col[idx];
  }
  else {
    assert(it.type == BIN_REL_ITER::BRIT_OPT_REC);
    return make_symb(it.iter.opt_rec.fields[it.idx]);
  }
}

OBJ get_curr_right_arg(BIN_REL_ITER &it) {
  assert(!is_out_of_range(it));

  if (it.type == BIN_REL_ITER::BRIT_BIN_REL) {
    uint32 idx = it.iter.bin_rel.rev_idxs != NULL ? it.iter.bin_rel.rev_idxs[it.idx] : it.idx;
    return it.iter.bin_rel.right_col[idx];
  }
  else {
    assert(it.type == BIN_REL_ITER::BRIT_OPT_REC);
    uint16 field = it.iter.opt_rec.fields[it.idx];
    return opt_repr_lookup_field(it.iter.opt_rec.ptr, it.iter.opt_rec.repr_id, field);
  }
}

OBJ tern_rel_it_get_left_arg(TERN_REL_ITER &it) {
  assert(!is_out_of_range(it));
  uint32 idx = it.ordered_idxs != NULL ? it.ordered_idxs[it.idx] : it.idx;
  return it.col1[idx];
}

OBJ tern_rel_it_get_mid_arg(TERN_REL_ITER &it) {
  assert(!is_out_of_range(it));
  uint32 idx = it.ordered_idxs != NULL ? it.ordered_idxs[it.idx] : it.idx;
  return it.col2[idx];
}

OBJ tern_rel_it_get_right_arg(TERN_REL_ITER &it) {
  assert(!is_out_of_range(it));
  uint32 idx = it.ordered_idxs != NULL ? it.ordered_idxs[it.idx] : it.idx;
  return it.col3[idx];
}

OBJ set_only_elt(OBJ set) {
  assert(read_size_field(set) == 1 && is_array_set(set));
  return *get_set_elts_ptr(set);
}

OBJ lookup(OBJ rel, OBJ key) {
  if (is_opt_rec(rel)) {
    if (is_symb(key)) {
      void *ptr = get_opt_repr_ptr(rel);
      uint16 repr_id = get_opt_repr_id(rel);
      uint16 field_id = get_symb_id(key);
      if (opt_repr_has_field(ptr, repr_id, field_id))
        return opt_repr_lookup_field(ptr, repr_id, field_id);
    }
  }
  else if (is_empty_rel(rel)) {
    soft_fail("Map is empty. Lookup failed");
  }
  else if (is_mixed_repr_map(rel)) {
    MIXED_REPR_MAP_OBJ *ptr = get_mixed_repr_map_ptr(rel);
    if (ptr->array_repr != NULL) {
      uint32 size = read_size_field(rel);
      bool found;
      uint32 idx = find_obj(ptr->array_repr->buffer, size, key, found);
      if (found) {
        OBJ *values = ptr->array_repr->buffer + size;
        return values[idx];
      }
    }
    else {
      OBJ value;
      if (tree_map_lookup(ptr->tree_repr, key, &value))
        return value;
    }
  }
  else {
    assert(is_array_map(rel)); //## CAN'T IT JUST BE A BINARY RELATION?

    BIN_REL_OBJ *ptr = get_bin_rel_ptr(rel);
    uint32 size = read_size_field(rel);
    OBJ *keys = ptr->buffer;
    OBJ *values = keys + size;
    OBJ_TYPE rel_type = get_obj_type(rel);
    if (rel_type == TYPE_NE_MAP) {
      bool found;
      uint32 idx = find_obj(keys, size, key, found);
      if (found)
        return values[idx];
    }
    else {
      assert(rel_type == TYPE_NE_BIN_REL);
      uint32 count;
      uint32 idx = find_objs_range(keys, size, key, count);
      if (count == 1)
        return values[idx];
      if (count > 1)
        soft_fail("Key is not unique. Lookup failed");
    }
  }

  if (is_symb(key)) {
    char buff[1024];
    strcpy(buff, "Map key not found: ");
    uint32 len = strlen(buff);
    printed_obj(key, buff+len, sizeof(buff)-len-1);
    soft_fail(buff);
  }

  soft_fail("Map key not found");
}

OBJ lookup_field(OBJ rec_or_tag_rec, uint16 field_id) {
  if (is_opt_rec_or_tag_rec(rec_or_tag_rec)) {
    void *ptr = get_opt_repr_ptr(rec_or_tag_rec);
    uint16 repr_id = get_opt_repr_id(rec_or_tag_rec);
    return opt_repr_lookup_field(ptr, repr_id, field_id);
  }

  OBJ rec = is_tag_obj(rec_or_tag_rec) ? get_inner_obj(rec_or_tag_rec) : rec_or_tag_rec;

  if (!is_empty_rel(rec)) {
    BIN_REL_OBJ *ptr = get_bin_rel_ptr(rec);
    uint32 size = read_size_field(rec);
    OBJ *keys = ptr->buffer;
    OBJ *values = keys + size;
    for (uint32 i=0 ; i < size ; i++)
      if (is_symb(keys[i], field_id))
        return values[i];
  }

  internal_fail();
}
