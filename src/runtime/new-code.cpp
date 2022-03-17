#include "lib.h"



int32 cast_int32(int64 val64) {
  int32 val32 = (int32) val64;
  if (val32 != val64)
    soft_fail("Invalid 64 to 32 bit integer conversion");
  return val32;
}

OBJ set_insert(OBJ set, OBJ elt) {
  if (is_empty_rel(set)) {
    SET_OBJ *new_ptr = new_set(1);
    OBJ *array = new_ptr->buffer;
    array[0] = elt;
    return make_set(new_ptr, 1);

  }

  uint32 size = read_size_field(set);
  OBJ *elts = get_set_elts_ptr(set);

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(elts, size, elt);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found) {
    if (are_eq(elts[index], elt))
      return set;

    SET_OBJ *new_ptr = new_set(size);
    OBJ *new_elts = new_ptr->buffer;

    if (index > 0)
      memcpy(new_elts, elts, index * sizeof(OBJ));
    new_elts[index] = elt;
    if (index + 1 < size)
      memcpy(new_elts + index + 1, elts + index + 1, (size - index - 1) * sizeof(OBJ));

    return make_set(new_ptr, size);
  }
  else {
    SET_OBJ *new_ptr = new_set(size + 1);
    OBJ *new_elts = new_ptr->buffer;

    if (index > 0)
      memcpy(new_elts, elts, index * sizeof(OBJ));
    new_elts[index] = elt;
    if (index < size)
      memcpy(new_elts + index + 1, elts + index, (size - index) * sizeof(OBJ));

    return make_set(new_ptr, size + 1);
  }
}

OBJ set_key_value(OBJ map, OBJ key, OBJ value) {
  if (is_empty_rel(map)) {
    BIN_REL_OBJ *new_ptr = new_map(1);
    OBJ *array = new_ptr->buffer;
    array[0] = key;
    array[1] = value;
    return make_map(new_ptr, 1);

  }

  uint32 map_type = get_map_type(map);

  if (map_type == ARRAY_MAP_TAG) {
    BIN_REL_OBJ *ptr = get_bin_rel_ptr(map);
    uint32 size = read_size_field(map);
    OBJ *keys = ptr->buffer;
    OBJ *values = keys + size;

    uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, size, key);
    uint32 index = (uint32) code;
    bool found = (code >> 32) == 0;

    if (found) {
      if (are_eq(values[index], value))
        return map;

      BIN_REL_OBJ *new_ptr = new_map(size);
      OBJ *new_keys = new_ptr->buffer;
      OBJ *new_values = new_keys + size;

      memcpy(new_keys, keys, size * sizeof(OBJ));

      if (index > 0)
        memcpy(new_values, values, index * sizeof(OBJ));
      new_values[index] = value;
      if (index + 1 < size)
        memcpy(new_values + index + 1, values + index + 1, (size - index - 1) * sizeof(OBJ));

      return make_map(new_ptr, size);
    }
    else {
      BIN_REL_OBJ *new_ptr = new_map(size + 1);
      OBJ *new_keys = new_ptr->buffer;
      OBJ *new_values = new_keys + size + 1;

      if (index > 0) {
        memcpy(new_keys, keys, index * sizeof(OBJ));
        memcpy(new_values, values, index * sizeof(OBJ));
      }

      new_keys[index] = key;
      new_values[index] = value;

      if (index < size) {
        memcpy(new_keys + index + 1, keys + index, (size - index) * sizeof(OBJ));
        memcpy(new_values + index + 1, values + index, (size - index) * sizeof(OBJ));
      }

      return make_map(new_ptr, size + 1);
    }
  }

  internal_fail();
}

OBJ drop_key(OBJ map, OBJ key) {
  impl_fail("Not implemented yet");
}
