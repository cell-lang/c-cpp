#include "lib.h"


const uint32 MIN_TREE_SIZE = 9;


inline FAT_MAP_PTR make_array_map_ptr(OBJ *ptr, uint32 size, uint32 offset) {
  FAT_MAP_PTR branch;
  branch.ptr.array = size != 0 ? ptr : NULL;
  branch.size = size;
  branch.offset = offset;
  return branch;
}

inline FAT_MAP_PTR make_tree_map_ptr(BIN_TREE_MAP_OBJ *ptr, uint32 size) {
  FAT_MAP_PTR branch;
  branch.ptr.tree = ptr;
  branch.size = size;
  branch.offset = 0;
  return branch;
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR array_map_set_key_value(OBJ *keys, uint32 size, OBJ key, OBJ value) {
  OBJ *values = keys + size;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, size, key);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found) {
    assert(index < size);

    if (are_eq(values[index], value))
      return make_array_map_ptr(keys, size, size);

    if (size < MIN_TREE_SIZE) {
      BIN_REL_OBJ *new_ptr = new_map(size);
      OBJ *new_keys = new_ptr->buffer;
      OBJ *new_values = new_keys + size;

      memcpy(new_keys, keys, size * sizeof(OBJ));

      if (index > 0)
        memcpy(new_values, values, index * sizeof(OBJ));
      new_values[index] = value;
      if (index + 1 < size)
        memcpy(new_values + index + 1, values + index + 1, (size - index - 1) * sizeof(OBJ));

      return make_array_map_ptr(new_ptr->buffer, size, size);
    }
    else {
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = key;
      new_ptr->value = value;
      new_ptr->left = make_array_map_ptr(keys, index, size);
      new_ptr->right = make_array_map_ptr(keys + index + 1, size - index - 1, size);
      new_ptr->priority = rand();
      return make_tree_map_ptr(new_ptr, size);
    }
  }
  else {
    if (size + 1 < MIN_TREE_SIZE) {
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

      return make_array_map_ptr(new_ptr->buffer, size + 1, size + 1);
    }
    else {
      assert(size + 1 == MIN_TREE_SIZE);

      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = key;
      new_ptr->value = value;
      new_ptr->left = make_array_map_ptr(keys, index, size);
      new_ptr->right = make_array_map_ptr(keys + index, size - index, size);
      new_ptr->priority = rand();
      return make_tree_map_ptr(new_ptr, size + 1);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR bin_tree_map_set_key_value(BIN_TREE_MAP_OBJ *ptr, uint32 size, OBJ key, OBJ value) {
  OBJ node_key = ptr->key;
  int cr = comp_objs(key, node_key);
  if (cr == 0) {
    if (are_eq(ptr->value, value))
      return make_tree_map_ptr(ptr, size);

    BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
    new_ptr->key = ptr->key;
    new_ptr->value = ptr->value;
    new_ptr->left = ptr->left;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, size + 1);
  }

  if (cr > 0) { // key < ptr->key
    // Inserting into the left subtree

    uint32 left_size = ptr->left.size;
    FAT_MAP_PTR updated_left_ptr;

    if (ptr->left.offset == 0) {
      // The left branch is an array
      OBJ *left_ptr = ptr->left.ptr.array;
      updated_left_ptr = array_map_set_key_value(left_ptr, left_size, key, value);

      assert((updated_left_ptr.ptr.array == left_ptr) == (updated_left_ptr.size == left_size));
      assert(updated_left_ptr.size == left_size || updated_left_ptr.size == left_size + 1);

      if (updated_left_ptr.size == left_size)
        return make_tree_map_ptr(ptr, size);
    }
    else {
      // The left branch is a tree
      BIN_TREE_MAP_OBJ *left_ptr = ptr->left.ptr.tree;
      FAT_MAP_PTR updated_left_ptr = bin_tree_map_set_key_value(left_ptr, left_size, key, value);
      assert((updated_left_ptr.ptr.tree == left_ptr) == (updated_left_ptr.size == size));
      if (updated_left_ptr.size == left_size)
        return make_tree_map_ptr(ptr, size);

      assert(updated_left_ptr.size == left_size + 1 && updated_left_ptr.size > MIN_TREE_SIZE);

      if (updated_left_ptr.ptr.tree->priority > ptr->priority) {
        // The updated left subtree has a higher priority, we need to perform a rotation

        BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
        new_ptr->key = ptr->key;
        new_ptr->value = ptr->value;
        new_ptr->left = updated_left_ptr.ptr.tree->right;
        new_ptr->right = ptr->right;
        new_ptr->priority = ptr->priority;

        updated_left_ptr.ptr.tree->right.ptr.tree = new_ptr;
        updated_left_ptr.ptr.tree->right.size = 1 + new_ptr->left.size + new_ptr->right.size;
        updated_left_ptr.ptr.tree->right.offset = 0;
        updated_left_ptr.size = size + 1;

        return updated_left_ptr;
      }
    }

    BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
    new_ptr->key = node_key;
    new_ptr->value = ptr->value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    assert(new_ptr->left.size + new_ptr->right.size == size);
    return make_tree_map_ptr(new_ptr, size + 1);
  }
  else { // key > ptr->key
    // Inserting into the right subtree

    uint32 right_size = ptr->right.size;
    FAT_MAP_PTR updated_right_ptr;

    if (ptr->right.offset == 0) {
      // The right subtree is an array
      OBJ *right_ptr = ptr->right.ptr.array;
      updated_right_ptr = array_map_set_key_value(right_ptr, right_size, key, value);

      assert((updated_right_ptr.ptr.array == right_ptr) == (updated_right_ptr.size == right_size));
      assert(updated_right_ptr.size == right_size || updated_right_ptr.size == right_size + 1);

      if (updated_right_ptr.size == right_size)
        return make_tree_map_ptr(ptr, size);
    }
    else {
      BIN_TREE_MAP_OBJ *right_ptr = ptr->right.ptr.tree;
      uint32 right_size = ptr->right.size;
      updated_right_ptr = bin_tree_map_set_key_value(right_ptr, right_size, key, value);

      assert((updated_right_ptr.ptr.tree == right_ptr) == (updated_right_ptr.size == size));
      assert(updated_right_ptr.size == right_size || updated_right_ptr.size == right_size + 1);
      assert(updated_right_ptr.size > MIN_TREE_SIZE);

      if (updated_right_ptr.size == right_size)
        return make_tree_map_ptr(ptr, size);

      if (updated_right_ptr.ptr.tree->priority > ptr->priority) {
        // The updated right subtree has a higher priority, we need to perform a rotation

        BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
        new_ptr->key = ptr->key;
        new_ptr->value = ptr->value;
        new_ptr->left = ptr->right;
        new_ptr->right = updated_right_ptr.ptr.tree->right;
        new_ptr->priority = ptr->priority;

        updated_right_ptr.ptr.tree->left.ptr.tree = new_ptr;
        updated_right_ptr.ptr.tree->left.size = 1 + new_ptr->left.size + new_ptr->right.size;
        updated_right_ptr.ptr.tree->left.offset = 0;
        updated_right_ptr.size = size + 1;

        return updated_right_ptr;
      }
    }

    BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
    new_ptr->key = node_key;
    new_ptr->value = ptr->value;
    new_ptr->left = ptr->left;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    assert(new_ptr->left.size + new_ptr->right.size == size);
    return make_tree_map_ptr(new_ptr, size + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ set_key_value(OBJ map, OBJ key, OBJ value) {
  if (is_empty_rel(map)) {
    BIN_REL_OBJ *new_ptr = new_map(1);
    OBJ *array = new_ptr->buffer;
    array[0] = key;
    array[1] = value;
    return make_map(new_ptr, 1);
  }

  uint32 size = read_size_field(map);
  FAT_MAP_PTR updated_ptr;

  if (is_array_map(map)) {
    OBJ *array = get_bin_rel_ptr(map)->buffer;
    updated_ptr = array_map_set_key_value(array, size, key, value);
  }
  else {
    BIN_TREE_MAP_OBJ *node_ptr = get_tree_map_ptr(map);
    updated_ptr = bin_tree_map_set_key_value(node_ptr, size, key, value);
  }

  assert(updated_ptr.offset == 0 || updated_ptr.offset == updated_ptr.size);

  if (updated_ptr.offset == 0)
    return make_tree_map(updated_ptr.ptr.tree, updated_ptr.size);
  else
    return make_map((BIN_REL_OBJ *) updated_ptr.ptr.array, updated_ptr.size);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// OBJ drop_key(OBJ map, OBJ key) {
//   impl_fail("Not implemented yet");
// }
