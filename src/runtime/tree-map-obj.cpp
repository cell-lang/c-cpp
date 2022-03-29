#include "lib.h"


const uint32 MIN_TREE_SIZE = 9;


inline FAT_MAP_PTR make_empty_map_ptr() {
  FAT_MAP_PTR fat_ptr;
  fat_ptr.ptr.array = NULL;
  fat_ptr.size = 0;
  fat_ptr.offset = 0;
  return fat_ptr;
}

inline FAT_MAP_PTR make_array_map_ptr(OBJ *ptr, uint32 size, uint32 offset) {
  FAT_MAP_PTR fat_ptr;
  fat_ptr.ptr.array = size != 0 ? ptr : NULL;
  fat_ptr.size = size;
  fat_ptr.offset = offset;
  return fat_ptr;
}

inline FAT_MAP_PTR make_tree_map_ptr(BIN_TREE_MAP_OBJ *ptr, uint32 size) {
  FAT_MAP_PTR fat_ptr;
  fat_ptr.ptr.tree = ptr;
  fat_ptr.size = size;
  fat_ptr.offset = 0;
  return fat_ptr;
}

////////////////////////////////////////////////////////////////////////////////

static void map_copy(FAT_MAP_PTR fat_ptr, OBJ *dest_keys, uint32 offset) {
  if (fat_ptr.size > 0) {
    OBJ *dest_values = dest_keys + offset;

    if (fat_ptr.offset != 0) {
      OBJ *src_keys = fat_ptr.ptr.array;
      OBJ *src_values = src_keys + fat_ptr.offset;
      memcpy(dest_keys, src_keys, fat_ptr.size * sizeof(OBJ));
      memcpy(dest_values, src_values, fat_ptr.size * sizeof(OBJ));
    }
    else {
      BIN_TREE_MAP_OBJ *ptr = fat_ptr.ptr.tree;
      FAT_MAP_PTR left_ptr = ptr->left;
      map_copy(left_ptr, dest_keys, offset);
      dest_keys[left_ptr.size] = ptr->key;
      dest_values[left_ptr.size] = ptr->value;
      map_copy(ptr->right, dest_keys + left_ptr.size + 1, offset);
    }
  }
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
  uint32 size = read_size_field(map);

  if (size == 0) {
    BIN_REL_OBJ *new_ptr = new_map(1);
    OBJ *array = new_ptr->buffer;
    array[0] = key;
    array[1] = value;
    return make_map(new_ptr, 1);
  }

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

static FAT_MAP_PTR remove_min_key(FAT_MAP_PTR fat_ptr, OBJ *rem_entry_ptr) {
  assert(fat_ptr.size > 0);

  uint32 new_size = fat_ptr.size - 1;

  if (fat_ptr.offset != 0) {
    // The fat pointer points to an array
    OBJ *keys = fat_ptr.ptr.array;
    OBJ *values = keys + fat_ptr.offset;
    rem_entry_ptr[0] = keys[0];
    rem_entry_ptr[1] = values[0];
    if (fat_ptr.size > 1)
      return make_array_map_ptr(keys + 1, new_size, fat_ptr.offset);
    else
      return make_empty_map_ptr();
  }
  else {
    assert(fat_ptr.size >= MIN_TREE_SIZE);

    // The fat pointer points to a binary tree
    BIN_TREE_MAP_OBJ *ptr = fat_ptr.ptr.tree;
    FAT_MAP_PTR left_ptr = ptr->left;

    if (left_ptr.size == 0) {
      rem_entry_ptr[0] = ptr->key;
      rem_entry_ptr[1] = ptr->value;
      return ptr->right;
    }

    if (new_size >= MIN_TREE_SIZE) {
      FAT_MAP_PTR updated_left_ptr = remove_min_key(left_ptr, rem_entry_ptr);
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = ptr->key;
      new_ptr->value = ptr->value;
      new_ptr->left = updated_left_ptr;
      new_ptr->right = ptr->right;
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }
    else {
      assert(fat_ptr.size == MIN_TREE_SIZE && left_ptr.offset != 0);

      OBJ *left_keys = left_ptr.ptr.array;
      OBJ *left_values = left_keys + left_ptr.offset;

      rem_entry_ptr[0] = left_keys[0];
      rem_entry_ptr[1] = left_values[0];

      BIN_REL_OBJ *new_ptr = new_map(new_size);
      OBJ *new_keys = new_ptr->buffer;
      OBJ *new_values = new_keys + new_size;

      if (left_ptr.size > 1) {
        memcpy(new_keys, left_keys + 1, (left_ptr.size - 1) * sizeof(OBJ));
        memcpy(new_values, left_values + 1, (left_ptr.size - 1) * sizeof(OBJ)) ;
      }

      new_keys[left_ptr.size - 1] = ptr->key;
      new_values[left_ptr.size - 1] = ptr->value;

      FAT_MAP_PTR right_ptr = ptr->right;
      if (right_ptr.size > 0) {
        assert(right_ptr.offset != 0);
        OBJ *right_keys = right_ptr.ptr.array;
        OBJ *right_values = right_keys + right_ptr.offset;
        memcpy(new_keys + left_ptr.size, right_keys, right_ptr.size * sizeof(OBJ));
        memcpy(new_values + left_ptr.size, right_values, right_ptr.size * sizeof(OBJ));
      }

      return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
    }
  }
}

static FAT_MAP_PTR remove_max_key(FAT_MAP_PTR fat_ptr, OBJ *rem_entry_ptr) {
  assert(fat_ptr.size > 0);

  uint32 new_size = fat_ptr.size - 1;

  if (fat_ptr.offset != 0) {
    // The fat pointer points to an array
    OBJ *keys = fat_ptr.ptr.array;
    OBJ *values = keys + fat_ptr.offset;
    rem_entry_ptr[0] = keys[fat_ptr.size - 1];
    rem_entry_ptr[1] = values[fat_ptr.size - 1];
    if (fat_ptr.size > 1)
      return make_array_map_ptr(keys, new_size, fat_ptr.offset);
    else
      return make_empty_map_ptr();
  }
  else {
    assert(fat_ptr.size >= MIN_TREE_SIZE);

    // The fat pointer points to a binary tree
    BIN_TREE_MAP_OBJ *ptr = fat_ptr.ptr.tree;
    FAT_MAP_PTR right_ptr = ptr->right;

    if (right_ptr.size == 0) {
      rem_entry_ptr[0] = ptr->key;
      rem_entry_ptr[1] = ptr->value;
      return ptr->left;
    }

    if (new_size >= MIN_TREE_SIZE) {
      FAT_MAP_PTR updated_right_ptr = remove_min_key(right_ptr, rem_entry_ptr);
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = ptr->key;
      new_ptr->value = ptr->value;
      new_ptr->left = ptr->left;
      new_ptr->right = updated_right_ptr;
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }
    else {
      assert(fat_ptr.size == MIN_TREE_SIZE && right_ptr.offset != 0);

      OBJ *right_keys = right_ptr.ptr.array;
      OBJ *right_values = right_keys + right_ptr.offset;

      rem_entry_ptr[0] = right_keys[right_ptr.size - 1];
      rem_entry_ptr[1] = right_values[right_ptr.size - 1];

      BIN_REL_OBJ *new_ptr = new_map(new_size);
      OBJ *new_keys = new_ptr->buffer;
      OBJ *new_values = new_keys + new_size;

      FAT_MAP_PTR left_ptr = ptr->left;
      if (left_ptr.size > 0) {
        assert(left_ptr.offset != 0);
        OBJ *left_keys = left_ptr.ptr.array;
        OBJ *left_values = left_keys + left_ptr.offset;
        memcpy(new_keys, left_keys, left_ptr.size * sizeof(OBJ));
        memcpy(new_values, left_values, left_ptr.size * sizeof(OBJ));
      }

      new_keys[left_ptr.size] = ptr->key;
      new_values[left_ptr.size] = ptr->value;

      if (right_ptr.size > 1) {
        memcpy(new_keys + left_ptr.size + 1, right_keys, (right_ptr.size - 1) * sizeof(OBJ));
        memcpy(new_values + left_ptr.size + 1, right_values, (right_ptr.size - 1) * sizeof(OBJ));
      }

      return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR array_map_drop_key(OBJ *keys, uint32 size, OBJ key) {
  OBJ *values = keys + size;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, size, key);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (!found)
    return make_array_map_ptr(keys, size, size);

  if (size == 1)
    return make_empty_map_ptr();

  if (size <= MIN_TREE_SIZE) {
    BIN_REL_OBJ *new_ptr = new_bin_rel(size - 1);
    OBJ *new_keys = new_ptr->buffer;
    OBJ *new_values = new_keys + size;

    if (index > 0) {
      memcpy(new_keys, keys, index * sizeof(OBJ));
      memcpy(new_values, values, index * sizeof(OBJ));
    }
    uint32 gt_count = size - index - 1;
    if (gt_count > 0) {
      memcpy(new_keys + index, keys + index + 1, gt_count * sizeof(OBJ));
      memcpy(new_values + index, values + index + 1, gt_count * sizeof(OBJ));
    }

    return make_array_map_ptr(new_ptr->buffer, size - 1, size);
  }
  else {
    BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
    new_ptr->priority = rand();

    if (index == 0) {
      new_ptr->key = keys[1];
      new_ptr->value = values[1];
      new_ptr->left = make_empty_map_ptr();
      new_ptr->right = make_array_map_ptr(keys + 2, size - 2, size);
    }
    if (index == size - 1) {
      new_ptr->key = keys[size - 2];
      new_ptr->value = values[size - 2];
      new_ptr->left = make_array_map_ptr(keys, size - 2, size);
      new_ptr->right = make_empty_map_ptr();
    }
    if (index == 1) {
      new_ptr->key = keys[0];
      new_ptr->value = values[0];
      new_ptr->left = make_empty_map_ptr();
      new_ptr->right = make_array_map_ptr(keys + 2, size - 2, size);
    }
    else if (index == size - 2) {
      new_ptr->key = keys[size - 1];
      new_ptr->value = values[size - 1];
      new_ptr->left = make_array_map_ptr(keys, size - 2, size);
      new_ptr->right = make_empty_map_ptr();
    }
    else if (index < size / 2) {
      new_ptr->key = keys[index + 1];
      new_ptr->value = values[index + 1];
      new_ptr->left = make_array_map_ptr(keys, index, size);
      new_ptr->right = make_array_map_ptr(keys + index + 2, size - index - 2, size);
    }
    else {
      new_ptr->key = keys[index - 1];
      new_ptr->value = values[index - 1];
      new_ptr->left = make_array_map_ptr(keys, index - 1, size);
      new_ptr->right = make_array_map_ptr(keys + index + 1, size - index - 1, size);
    }

    return make_tree_map_ptr(new_ptr, size - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR bin_tree_map_drop_key(BIN_TREE_MAP_OBJ *ptr, uint32 size, OBJ key) {
  assert(size >= MIN_TREE_SIZE);

  OBJ node_key = ptr->key;
  int cr = comp_objs(key, node_key);

  if (cr == 0) {
    uint32 left_size = ptr->left.size;
    if (left_size == 0)
      return ptr->right;

    uint32 right_size = ptr->right.size;
    if (right_size == 0)
      return ptr->left;

    uint32 new_size = size - 1;
    assert(new_size == left_size + right_size && new_size >= MIN_TREE_SIZE - 1);

    if (new_size == MIN_TREE_SIZE - 1) {
      BIN_REL_OBJ *new_ptr = new_map(new_size);
      OBJ *keys = new_ptr->buffer;
      map_copy(ptr->left, keys, new_size);
      map_copy(ptr->right, keys + left_size, new_size);
      return make_array_map_ptr((OBJ *) keys, new_size, new_size);
    }

    if (left_size == 1) {
      assert(ptr->left.offset != 0);
      OBJ *key_ptr = ptr->left.ptr.array;
      OBJ *value_ptr = key_ptr + ptr->left.offset;
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = *key_ptr;
      new_ptr->value = *value_ptr;
      new_ptr->left = make_empty_map_ptr();
      new_ptr->right = ptr->right;
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }

    if (right_size == 1) {
      assert(ptr->left.offset != 0);
      OBJ *key_ptr = ptr->right.ptr.array;
      OBJ *value_ptr = key_ptr + ptr->right.offset;
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = *key_ptr;
      new_ptr->value = *value_ptr;
      new_ptr->left = ptr->left;
      new_ptr->right = make_empty_map_ptr();
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }

    if (left_size > right_size) {
      OBJ rem_entry[2];
      FAT_MAP_PTR updated_left_ptr = remove_max_key(ptr->left, rem_entry);
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = rem_entry[0];
      new_ptr->value = rem_entry[1];
      new_ptr->left = updated_left_ptr;
      new_ptr->right = ptr->right;
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }
    else {
      OBJ rem_entry[2];
      FAT_MAP_PTR updated_right_ptr = remove_min_key(ptr->right, rem_entry);
      BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
      new_ptr->key = rem_entry[0];
      new_ptr->value = rem_entry[1];
      new_ptr->left = ptr->left;
      new_ptr->right = updated_right_ptr;
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }
  }

  if (cr > 0) { // key < ptr->key
    // Removing the element from the left subtree
    uint32 left_size = ptr->left.size;
    FAT_MAP_PTR updated_left_ptr;
    if (ptr->left.offset == 0)
      updated_left_ptr = bin_tree_map_drop_key(ptr->left.ptr.tree, left_size, key);
    else
      updated_left_ptr = array_map_drop_key(ptr->left.ptr.array, left_size, key);

    // Checking that the subtree has actually changed, that is, that it actually contained the element to be removed
    if (updated_left_ptr.size == left_size)
      return make_tree_map_ptr(ptr, size);
    assert(updated_left_ptr.size == ptr->left.size - 1);

    // The priority of the updated subtree is not supposed to have changed
    assert(updated_left_ptr.offset != 0 || updated_left_ptr.ptr.tree->priority == ptr->left.ptr.tree->priority);

    uint32 new_size = size - 1;
    assert(new_size >= MIN_TREE_SIZE - 1);

    if (new_size == MIN_TREE_SIZE - 1) {
      BIN_REL_OBJ *new_ptr = new_map(new_size);
      map_copy(updated_left_ptr, new_ptr->buffer, new_size);
      map_copy(ptr->right, new_ptr->buffer + updated_left_ptr.size, new_size);
      return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
    }

    BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
    new_ptr->key = ptr->key; //## USE node_key INSTEAD, HERE AND IN THE CODE ABOVE
    new_ptr->value = ptr->value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, size - 1);
  }
  else { // key > ptr->key
    // Removing the element from the right subtree
    uint32 right_size = ptr->right.size;
    FAT_MAP_PTR updated_right_ptr;
    if (ptr->right.offset == 0)
      updated_right_ptr = bin_tree_map_drop_key(ptr->right.ptr.tree, right_size, key);
    else
      updated_right_ptr = array_map_drop_key(ptr->right.ptr.array, right_size, key);

    if (updated_right_ptr.size == right_size)
      return make_tree_map_ptr(ptr, size);
    assert(updated_right_ptr.size == right_size - 1);

    // The priority of the updated subtree is not supposed to have changed
    assert(updated_right_ptr.offset != 0 || updated_right_ptr.ptr.tree->priority == ptr->right.ptr.tree->priority);

    uint32 new_size = size - 1;
    assert(new_size >= MIN_TREE_SIZE - 1);

    if (new_size == MIN_TREE_SIZE - 1) {
      BIN_REL_OBJ *new_ptr = new_map(new_size);
      map_copy(ptr->left, new_ptr->buffer, new_size);
      map_copy(updated_right_ptr, new_ptr->buffer + ptr->left.size, new_size);
      return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
    }

    BIN_TREE_MAP_OBJ *new_ptr = new_bin_tree_map();
    new_ptr->key = ptr->key; //## USE node_key INSTEAD, HERE AND ELSEWHERE
    new_ptr->value = ptr->value;
    new_ptr->left = ptr->left;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, size - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ drop_key(OBJ map, OBJ key) {
  uint32 size = read_size_field(map);

  if (size == 0)
    return map;

  FAT_MAP_PTR updated_ptr;

  if (is_array_map(map)) {
    OBJ *array = get_bin_rel_ptr(map)->buffer;
    updated_ptr = array_map_drop_key(array, size, key);
  }
  else {
    BIN_TREE_MAP_OBJ *node_ptr = get_tree_map_ptr(map);
    updated_ptr = bin_tree_map_drop_key(node_ptr, size, key);
  }

  assert(updated_ptr.offset == 0 || updated_ptr.offset == updated_ptr.size);

  if (updated_ptr.offset == 0)
    return make_tree_map(updated_ptr.ptr.tree, updated_ptr.size);
  else
    return make_map((BIN_REL_OBJ *) updated_ptr.ptr.array, updated_ptr.size);
}
