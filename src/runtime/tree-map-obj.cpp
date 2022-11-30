#include "lib.h"

//## THERE'S STILL THE PROBLEM WITH new_map(..) TO FIX...
//## ALSO FIX THIS:
//##   {ptr = {array = 0x0, tree = 0x0}, size = 0, offset = 8}


const uint32 MIN_TREE_MAP_SIZE = 7;


inline FAT_MAP_PTR make_empty_map_ptr() {
  FAT_MAP_PTR fat_ptr;
  fat_ptr.ptr.array = NULL;
  fat_ptr.size = 0;
  fat_ptr.offset = 0;
  return fat_ptr;
}

inline FAT_MAP_PTR make_array_map_ptr(OBJ *ptr, uint32 size, uint32 offset) {
  assert(offset > 0);
  for (int i=0 ; i < size ; i++) {
    assert(!is_blank(ptr[i]));
    assert(get_obj_type(ptr[i]) != TYPE_NOT_A_VALUE_OBJ);
  }

  FAT_MAP_PTR fat_ptr;
  fat_ptr.ptr.array = size != 0 ? ptr : NULL;
  fat_ptr.size = size;
  fat_ptr.offset = offset;
  return fat_ptr;
}

inline FAT_MAP_PTR make_tree_map_ptr(TREE_MAP_NODE *ptr, uint32 size) {
  assert(size == 1 + ptr->left.size + ptr->right.size);

  FAT_MAP_PTR fat_ptr;
  fat_ptr.ptr.tree = ptr;
  fat_ptr.size = size;
  fat_ptr.offset = 0;
  return fat_ptr;
}

inline bool fat_map_ptr_eq(FAT_MAP_PTR fat_ptr_1, FAT_MAP_PTR fat_ptr_2) {
  bool eq = fat_ptr_1.ptr.array == fat_ptr_2.ptr.array & fat_ptr_1.size == fat_ptr_2.size;
  assert(!eq | fat_ptr_1.offset == fat_ptr_2.offset);
  return eq;
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
      TREE_MAP_NODE *ptr = fat_ptr.ptr.tree;
      FAT_MAP_PTR left_ptr = ptr->left;
      map_copy(left_ptr, dest_keys, offset);
      dest_keys[left_ptr.size] = ptr->key;
      dest_values[left_ptr.size] = ptr->value;
      map_copy(ptr->right, dest_keys + left_ptr.size + 1, offset);
    }
  }
}

static void map_copy_range(FAT_MAP_PTR fat_ptr, uint32 first, uint32 count, OBJ *dest_keys, uint32 offset) {
  assert(first + count <= fat_ptr.size);

  if (count > 0) {
    OBJ *dest_values = dest_keys + offset;

    if (fat_ptr.offset != 0) {
      OBJ *src_keys = fat_ptr.ptr.array;
      OBJ *src_values = src_keys + fat_ptr.offset;
      memcpy(dest_keys, src_keys + first, count * sizeof(OBJ));
      memcpy(dest_values, src_values + first, count * sizeof(OBJ));
    }
    else {
      TREE_MAP_NODE *ptr = fat_ptr.ptr.tree;
      FAT_MAP_PTR left = ptr->left;

      if (left.size >= first) {
        uint32 left_copied = min_u32(count, left.size - first);
        if (left_copied > 0)
          map_copy_range(left, first, left_copied, dest_keys, offset);
        if (left_copied < count) {
          dest_keys[left_copied] = ptr->key;
          dest_values[left_copied] = ptr->value;
          uint32 right_copied = count - left_copied - 1;
          if (right_copied > 0)
            map_copy_range(ptr->right, 0, right_copied, dest_keys + left_copied + 1, offset);
        }
      }
      else
        map_copy_range(ptr->right, first - left.size - 1, count, dest_keys, offset);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR array_map_set_key_value(OBJ *keys, uint32 size, uint32 offset, OBJ key, OBJ value) {
  assert(offset > 0);

  OBJ *values = keys + offset;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, size, key);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found) {
    assert(index < size);

    if (are_eq(values[index], value))
      return make_array_map_ptr(keys, size, offset);

    if (size < MIN_TREE_MAP_SIZE) {
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
      TREE_MAP_NODE *new_ptr = new_tree_map_node();
      new_ptr->key = key;
      new_ptr->value = value;
      new_ptr->left = make_array_map_ptr(keys, index, offset);
      new_ptr->right = make_array_map_ptr(keys + index + 1, size - index - 1, offset);
      new_ptr->priority = rand();
      return make_tree_map_ptr(new_ptr, size);
    }
  }
  else {
    if (size + 1 < MIN_TREE_MAP_SIZE) {
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
      TREE_MAP_NODE *new_ptr = new_tree_map_node();
      new_ptr->key = key;
      new_ptr->value = value;
      new_ptr->left = make_array_map_ptr(keys, index, offset);
      new_ptr->right = make_array_map_ptr(keys + index, size - index, offset);
      new_ptr->priority = rand();
      return make_tree_map_ptr(new_ptr, size + 1);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR bin_tree_map_set_key_value(TREE_MAP_NODE *ptr, uint32 size, OBJ key, OBJ value) {
  assert(size == 1 + ptr->left.size + ptr->right.size);

  OBJ node_key = ptr->key;
  int cr = comp_objs(key, node_key);
  if (cr == 0) {
    if (are_eq(ptr->value, value))
      return make_tree_map_ptr(ptr, size);

    TREE_MAP_NODE *new_ptr = new_tree_map_node();
    new_ptr->key = key;
    new_ptr->value = value;
    new_ptr->left = ptr->left;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, size);
  }

  if (cr > 0) { // key < ptr->key
    // Inserting into the left subtree

    FAT_MAP_PTR left_ptr = ptr->left;
    FAT_MAP_PTR updated_left_ptr;

    if (left_ptr.size == 0) {
      BIN_REL_OBJ *new_ptr = new_map(1); //## SHOULD THIS ACTUALLY BE A MAP OR JUST AN OBJ ARRAY?
      new_ptr->buffer[0] = key;
      new_ptr->buffer[1] = value;
      updated_left_ptr = make_array_map_ptr(new_ptr->buffer, 1, 1);
    }
    else if (left_ptr.offset != 0) {
      // The left branch is an array
      updated_left_ptr = array_map_set_key_value(left_ptr.ptr.array, left_ptr.size, left_ptr.offset, key, value);
      assert(updated_left_ptr.size == left_ptr.size || updated_left_ptr.size == left_ptr.size + 1);

      if (fat_map_ptr_eq(updated_left_ptr, left_ptr))
        return make_tree_map_ptr(ptr, size);

      //## ISN'T THERE A BUG HERE? THE UPDATED LEFT SUBTREE COULD HAVE BECOME A TREE, REQUIRING A ROTATION
      //## THE BUG MAY BE REPEATED ELSEWHERE. I EXPECT THE ASSERTION BELOW TO FAIL
      // assert(updated_left_ptr.offset != 0);
    }
    else {
      // The left branch is a tree
      updated_left_ptr = bin_tree_map_set_key_value(left_ptr.ptr.tree, left_ptr.size, key, value);
      assert(updated_left_ptr.size == left_ptr.size || updated_left_ptr.size == left_ptr.size + 1);

      if (fat_map_ptr_eq(updated_left_ptr, left_ptr))
        return make_tree_map_ptr(ptr, size);

      // assert(updated_left_ptr.size > MIN_TREE_MAP_SIZE); //## TRY AGAIN LATER
    }

    if (updated_left_ptr.offset == 0 && updated_left_ptr.ptr.tree->priority > ptr->priority) {
      // The updated left subtree has a higher priority, we need to perform a rotation

      TREE_MAP_NODE *new_ptr = new_tree_map_node();
      new_ptr->key = ptr->key;
      new_ptr->value = ptr->value;
      new_ptr->left = updated_left_ptr.ptr.tree->right;
      new_ptr->right = ptr->right;
      new_ptr->priority = ptr->priority;

      updated_left_ptr.ptr.tree->right = make_tree_map_ptr(new_ptr, 1 + new_ptr->left.size + new_ptr->right.size);
      updated_left_ptr.size = 1 + updated_left_ptr.ptr.tree->left.size + updated_left_ptr.ptr.tree->right.size;

      assert(updated_left_ptr.size == size + 1); //## I EXPECTED THIS TO FAIL SOMETIMES. WHY DIDN'T THAT HAPPEN?

      return updated_left_ptr;
    }

    TREE_MAP_NODE *new_ptr = new_tree_map_node();
    new_ptr->key = node_key;
    new_ptr->value = ptr->value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, 1 + updated_left_ptr.size + new_ptr->right.size);
  }
  else { // key > ptr->key
    // Inserting into the right subtree

    FAT_MAP_PTR right_ptr = ptr->right;
    FAT_MAP_PTR updated_right_ptr;

    if (right_ptr.size == 0) {
      BIN_REL_OBJ *new_ptr = new_map(1); //## SHOULD THIS ACTUALLY BE A MAP OR JUST AN OBJ ARRAY?
      new_ptr->buffer[0] = key;
      new_ptr->buffer[1] = value;
      updated_right_ptr = make_array_map_ptr(new_ptr->buffer, 1, 1);
    }
    else if (right_ptr.offset != 0) {
      // The right subtree is an array
      updated_right_ptr = array_map_set_key_value(right_ptr.ptr.array, right_ptr.size, right_ptr.offset, key, value);
      assert(updated_right_ptr.size == right_ptr.size || updated_right_ptr.size == right_ptr.size + 1);

      if (fat_map_ptr_eq(updated_right_ptr, right_ptr))
        return make_tree_map_ptr(ptr, size);

      //## ISN'T THERE A BUG HERE? THE UPDATED LEFT SUBTREE COULD HAVE BECOME A TREE, REQUIRING A ROTATION
      //## THE BUG MAY BE REPEATED ELSEWHERE. I EXPECT THE ASSERTION BELOW TO FAIL
      // assert(updated_right_ptr.offset != 0);
    }
    else {
      updated_right_ptr = bin_tree_map_set_key_value(right_ptr.ptr.tree, right_ptr.size, key, value);

      assert(updated_right_ptr.size == right_ptr.size || updated_right_ptr.size == right_ptr.size + 1);
      // assert(updated_right_ptr.size > MIN_TREE_MAP_SIZE);

      if (fat_map_ptr_eq(updated_right_ptr, right_ptr))
        return make_tree_map_ptr(ptr, size);
    }

    if (updated_right_ptr.offset == 0 && updated_right_ptr.ptr.tree->priority > ptr->priority) {
      // The updated right subtree has a higher priority, we need to perform a rotation

      TREE_MAP_NODE *new_ptr = new_tree_map_node();
      new_ptr->key = ptr->key;
      new_ptr->value = ptr->value;
      new_ptr->left = ptr->left;
      new_ptr->right = updated_right_ptr.ptr.tree->left;
      new_ptr->priority = ptr->priority;

      updated_right_ptr.ptr.tree->left = make_tree_map_ptr(new_ptr, 1 + new_ptr->left.size + new_ptr->right.size);
      updated_right_ptr.size = 1 + updated_right_ptr.ptr.tree->left.size + updated_right_ptr.ptr.tree->right.size;
      assert(updated_right_ptr.size == size + 1); //## I EXPECTED THIS TO FAIL SOMETIMES. WHY DIDN'T THAT HAPPEN?

      return updated_right_ptr;
    }

    TREE_MAP_NODE *new_ptr = new_tree_map_node();
    new_ptr->key = node_key;
    new_ptr->value = ptr->value;
    new_ptr->left = ptr->left;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, 1 + new_ptr->left.size + updated_right_ptr.size);
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
    updated_ptr = array_map_set_key_value(array, size, size, key, value);
  }
  else {
    MIXED_REPR_MAP_OBJ *ptr = get_mixed_repr_map_ptr(map);
    if (ptr->array_repr != NULL)
      updated_ptr = array_map_set_key_value(ptr->array_repr->buffer, size, size, key, value);
    else
      updated_ptr = bin_tree_map_set_key_value(ptr->tree_repr, size, key, value);
  }

  assert(updated_ptr.offset == 0 || updated_ptr.offset == updated_ptr.size);

  if (updated_ptr.offset != 0)
    return make_map((BIN_REL_OBJ *) updated_ptr.ptr.array, updated_ptr.size);

  MIXED_REPR_MAP_OBJ *new_ptr = new_mixed_repr_map();
  new_ptr->array_repr = NULL;
  new_ptr->tree_repr = updated_ptr.ptr.tree;
  return make_mixed_repr_map(new_ptr, updated_ptr.size);
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

  // assert(fat_ptr.size >= MIN_TREE_MAP_SIZE); //## TRY AGAIN LATER

  // The fat pointer points to a binary tree
  TREE_MAP_NODE *ptr = fat_ptr.ptr.tree;
  FAT_MAP_PTR left_ptr = ptr->left;

  if (left_ptr.size == 0) {
    rem_entry_ptr[0] = ptr->key;
    rem_entry_ptr[1] = ptr->value;
    return ptr->right;
  }

  if (new_size >= MIN_TREE_MAP_SIZE) {
    FAT_MAP_PTR updated_left_ptr = remove_min_key(left_ptr, rem_entry_ptr);
    TREE_MAP_NODE *new_ptr = new_tree_map_node();
    new_ptr->key = ptr->key;
    new_ptr->value = ptr->value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, new_size);
  }

  // assert(fat_ptr.size == MIN_TREE_MAP_SIZE && right_ptr.offset != 0); //## TRY AGAIN LATER

  map_copy_range(left_ptr, 0, 1, rem_entry_ptr, 1);

  BIN_REL_OBJ *new_ptr = new_map(new_size);
  map_copy_range(fat_ptr, 1, new_size, new_ptr->buffer, new_size);
  return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
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

  // assert(fat_ptr.size >= MIN_TREE_MAP_SIZE); //## TRY AGAIN LATER

  // The fat pointer points to a binary tree
  TREE_MAP_NODE *ptr = fat_ptr.ptr.tree;
  FAT_MAP_PTR right_ptr = ptr->right;

  if (right_ptr.size == 0) {
    rem_entry_ptr[0] = ptr->key;
    rem_entry_ptr[1] = ptr->value;
    return ptr->left;
  }

  if (new_size >= MIN_TREE_MAP_SIZE) {
    FAT_MAP_PTR updated_right_ptr = remove_max_key(right_ptr, rem_entry_ptr);
    TREE_MAP_NODE *new_ptr = new_tree_map_node();
    new_ptr->key = ptr->key;
    new_ptr->value = ptr->value;
    new_ptr->left = ptr->left;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    return make_tree_map_ptr(new_ptr, new_size);
  }

  // assert(fat_ptr.size == MIN_TREE_MAP_SIZE && right_ptr.offset != 0); //## TRY AGAIN LATER

  map_copy_range(right_ptr, right_ptr.size - 1, 1, rem_entry_ptr, 1);

  BIN_REL_OBJ *new_ptr = new_map(new_size);
  map_copy_range(fat_ptr, 0, new_size, new_ptr->buffer, new_size);
  return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR array_map_drop_key(OBJ *keys, uint32 size, uint32 offset, OBJ key) {
  OBJ *values = keys + offset;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, size, key);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (!found)
    return make_array_map_ptr(keys, size, offset);

  if (size == 1)
    return make_empty_map_ptr();

  if (size <= MIN_TREE_MAP_SIZE) {
    BIN_REL_OBJ *new_ptr = new_bin_rel(size - 1);
    OBJ *new_keys = new_ptr->buffer;
    OBJ *new_values = new_keys + size - 1;

    if (index > 0) {
      memcpy(new_keys, keys, index * sizeof(OBJ));
      memcpy(new_values, values, index * sizeof(OBJ));
    }
    uint32 gt_count = size - index - 1;
    if (gt_count > 0) {
      memcpy(new_keys + index, keys + index + 1, gt_count * sizeof(OBJ));
      memcpy(new_values + index, values + index + 1, gt_count * sizeof(OBJ));
    }

    return make_array_map_ptr(new_ptr->buffer, size - 1, size - 1);
  }
  else {
    TREE_MAP_NODE *new_ptr = new_tree_map_node();
    new_ptr->priority = rand();

    if (index == 0) {
      new_ptr->key = keys[1];
      new_ptr->value = values[1];
      new_ptr->left = make_empty_map_ptr();
      new_ptr->right = make_array_map_ptr(keys + 2, size - 2, offset);
    }
    if (index == size - 1) {
      new_ptr->key = keys[size - 2];
      new_ptr->value = values[size - 2];
      new_ptr->left = make_array_map_ptr(keys, size - 2, offset);
      new_ptr->right = make_empty_map_ptr();
    }
    if (index == 1) {
      new_ptr->key = keys[0];
      new_ptr->value = values[0];
      new_ptr->left = make_empty_map_ptr();
      new_ptr->right = make_array_map_ptr(keys + 2, size - 2, offset);
    }
    else if (index == size - 2) {
      new_ptr->key = keys[size - 1];
      new_ptr->value = values[size - 1];
      new_ptr->left = make_array_map_ptr(keys, size - 2, offset);
      new_ptr->right = make_empty_map_ptr();
    }
    else if (index < size / 2) {
      new_ptr->key = keys[index + 1];
      new_ptr->value = values[index + 1];
      new_ptr->left = make_array_map_ptr(keys, index, offset);
      new_ptr->right = make_array_map_ptr(keys + index + 2, size - index - 2, offset);
    }
    else {
      new_ptr->key = keys[index - 1];
      new_ptr->value = values[index - 1];
      new_ptr->left = make_array_map_ptr(keys, index - 1, offset);
      new_ptr->right = make_array_map_ptr(keys + index + 1, size - index - 1, offset);
    }

    return make_tree_map_ptr(new_ptr, size - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_MAP_PTR bin_tree_map_drop_key(TREE_MAP_NODE *ptr, uint32 size, OBJ key) {
  // assert(size >= MIN_TREE_MAP_SIZE); //## TRY AGAIN LATER

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
    assert(new_size == left_size + right_size);
    // assert(new_size >= MIN_TREE_MAP_SIZE - 1); //## TRY AGAIN LATER

    if (new_size == MIN_TREE_MAP_SIZE - 1) {
      BIN_REL_OBJ *new_ptr = new_map(new_size);
      OBJ *keys = new_ptr->buffer;
      map_copy(ptr->left, keys, new_size);
      map_copy(ptr->right, keys + left_size, new_size);
      return make_array_map_ptr((OBJ *) keys, new_size, new_size);
    }

    if (left_size == 1) {
      TREE_MAP_NODE *new_ptr = new_tree_map_node();
      if (ptr->left.offset != 0) {
        OBJ *key_ptr = ptr->left.ptr.array;
        OBJ *value_ptr = key_ptr + ptr->left.offset;
        new_ptr->key = *key_ptr;
        new_ptr->value = *value_ptr;
      }
      else {
        TREE_MAP_NODE *left_ptr = ptr->left.ptr.tree;
        new_ptr->key = left_ptr->key;
        new_ptr->value = left_ptr->value;
      }
      new_ptr->left = make_empty_map_ptr();
      new_ptr->right = ptr->right;
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }

    if (right_size == 1) {
      TREE_MAP_NODE *new_ptr = new_tree_map_node();
      if (ptr->right.offset != 0) {
        OBJ *key_ptr = ptr->right.ptr.array;
        OBJ *value_ptr = key_ptr + ptr->right.offset;
        new_ptr->key = *key_ptr;
        new_ptr->value = *value_ptr;
      }
      else {
        TREE_MAP_NODE *right_ptr = ptr->right.ptr.tree;
        new_ptr->key = right_ptr->key;
        new_ptr->value = right_ptr->value;
      }
      new_ptr->left = ptr->left;
      new_ptr->right = make_empty_map_ptr();
      new_ptr->priority = ptr->priority;
      return make_tree_map_ptr(new_ptr, new_size);
    }

    if (left_size > right_size) {
      OBJ rem_entry[2];
      FAT_MAP_PTR updated_left_ptr = remove_max_key(ptr->left, rem_entry);
      TREE_MAP_NODE *new_ptr = new_tree_map_node();
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
      TREE_MAP_NODE *new_ptr = new_tree_map_node();
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
    if (left_size == 0)
      return make_tree_map_ptr(ptr, size);

    FAT_MAP_PTR updated_left_ptr;
    if (ptr->left.offset == 0)
      updated_left_ptr = bin_tree_map_drop_key(ptr->left.ptr.tree, left_size, key);
    else
      updated_left_ptr = array_map_drop_key(ptr->left.ptr.array, left_size, ptr->left.offset, key);

    // Checking that the subtree has actually changed, that is, that it actually contained the element to be removed
    if (updated_left_ptr.size == left_size)
      return make_tree_map_ptr(ptr, size);
    assert(updated_left_ptr.size == ptr->left.size - 1);

    assert(updated_left_ptr.size == 0 || updated_left_ptr.offset != 0 || updated_left_ptr.ptr.tree->priority <= ptr->left.ptr.tree->priority);

    if (size == MIN_TREE_MAP_SIZE) {
      uint32 new_size = size - 1;
      BIN_REL_OBJ *new_ptr = new_map(new_size);
      map_copy(updated_left_ptr, new_ptr->buffer, new_size);
      new_ptr->buffer[updated_left_ptr.size] = ptr->key;
      new_ptr->buffer[updated_left_ptr.size + new_size] = ptr->value;
      map_copy(ptr->right, new_ptr->buffer + updated_left_ptr.size + 1, new_size);
      return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
    }

    TREE_MAP_NODE *new_ptr = new_tree_map_node();
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
    if (right_size == 0)
      return make_tree_map_ptr(ptr, size);

    FAT_MAP_PTR updated_right_ptr;
    if (ptr->right.offset == 0)
      updated_right_ptr = bin_tree_map_drop_key(ptr->right.ptr.tree, right_size, key);
    else
      updated_right_ptr = array_map_drop_key(ptr->right.ptr.array, right_size, ptr->right.offset, key);

    if (updated_right_ptr.size == right_size)
      return make_tree_map_ptr(ptr, size);
    assert(updated_right_ptr.size == right_size - 1);

    assert(updated_right_ptr.size == 0 || updated_right_ptr.offset != 0 || updated_right_ptr.ptr.tree->priority <= ptr->right.ptr.tree->priority);

    // assert(new_size >= MIN_TREE_MAP_SIZE - 1);

    if (size == MIN_TREE_MAP_SIZE) {
      uint32 left_size = ptr->left.size;
      uint32 new_size = size - 1;
      BIN_REL_OBJ *new_ptr = new_map(new_size);
      map_copy(ptr->left, new_ptr->buffer, new_size);
      new_ptr->buffer[left_size] = ptr->key;
      new_ptr->buffer[left_size + new_size] = ptr->value;
      map_copy(updated_right_ptr, new_ptr->buffer + ptr->left.size + 1, new_size);
      return make_array_map_ptr(new_ptr->buffer, new_size, new_size);
    }

    TREE_MAP_NODE *new_ptr = new_tree_map_node();
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
    updated_ptr = array_map_drop_key(array, size, size, key);
  }
  else {
    MIXED_REPR_MAP_OBJ *ptr = get_mixed_repr_map_ptr(map);
    if (ptr->array_repr != NULL)
      updated_ptr = array_map_drop_key(ptr->array_repr->buffer, size, size, key);
    else
      updated_ptr = bin_tree_map_drop_key(ptr->tree_repr, size, key);
  }

  // assert(updated_ptr.offset == 0 || updated_ptr.offset == updated_ptr.size); //## WHY WAS THIS HERE IN THE FIRST PLACE?

  if (updated_ptr.size == 0)
    return make_empty_rel();

  if (updated_ptr.offset != 0)
    return make_map((BIN_REL_OBJ *) updated_ptr.ptr.array, updated_ptr.size);

  MIXED_REPR_MAP_OBJ *new_ptr = new_mixed_repr_map();
  new_ptr->array_repr = NULL;
  new_ptr->tree_repr = updated_ptr.ptr.tree;
  return make_mixed_repr_map(new_ptr, updated_ptr.size);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static bool map_contains(FAT_MAP_PTR fat_ptr, OBJ key, OBJ value) {
  if (fat_ptr.size == 0)
    return false;

  if (fat_ptr.offset == 0)
    return tree_map_contains(fat_ptr.ptr.tree, key, value);

  OBJ *keys = fat_ptr.ptr.array;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, fat_ptr.size, key);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found) {
    OBJ *values = keys + fat_ptr.offset;
    return are_eq(value, values[index]);
  }

  return false;
}

bool tree_map_contains(TREE_MAP_NODE *ptr, OBJ key, OBJ value) {
  int cr = comp_objs(key, ptr->key);

  if (cr == 0)
    return are_eq(value, ptr->value);

  if (cr > 0) // key < ptr->key
    return map_contains(ptr->left, key, value);
  else // key > ptr->key
    return map_contains(ptr->right, key, value);
}

////////////////////////////////////////////////////////////////////////////////

static bool map_contains_key(FAT_MAP_PTR fat_ptr, OBJ key) {
  if (fat_ptr.size == 0)
    return false;

  if (fat_ptr.offset == 0)
    return tree_map_contains_key(fat_ptr.ptr.tree, key);

  OBJ *keys = fat_ptr.ptr.array;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, fat_ptr.size, key);
  bool found = (code >> 32) == 0;
  return found;
}

bool tree_map_contains_key(TREE_MAP_NODE *ptr, OBJ key) {
  int cr = comp_objs(key, ptr->key);

  if (cr == 0)
    return true;

  if (cr > 0) // key < ptr->key
    return map_contains_key(ptr->left, key);
  else // key > ptr->key
    return map_contains_key(ptr->right, key);
}

////////////////////////////////////////////////////////////////////////////////

static bool map_lookup(FAT_MAP_PTR fat_ptr, OBJ key, OBJ *value) {
  if (fat_ptr.size == 0)
    return false;

  if (fat_ptr.offset == 0)
    return tree_map_lookup(fat_ptr.ptr.tree, key, value);

  OBJ *keys = fat_ptr.ptr.array;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(keys, fat_ptr.size, key);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found) {
    OBJ *values = keys + fat_ptr.offset;
    *value = values[index];
    return true;
  }

  return false;
}

bool tree_map_lookup(TREE_MAP_NODE *ptr, OBJ key, OBJ *value) {
  int cr = comp_objs(key, ptr->key);

  if (cr == 0) {
    *value = ptr->value;
    return true;
  }

  if (cr > 0) // key < ptr->key
    return map_lookup(ptr->left, key, value);
  else // key > ptr->key
    return map_lookup(ptr->right, key, value);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void rearrange_map_as_array(MIXED_REPR_MAP_OBJ *ptr, uint32 size) {
  assert(ptr->array_repr == NULL);

  BIN_REL_OBJ *new_ptr = new_map(size);
  FAT_MAP_PTR fat_ptr = make_tree_map_ptr(ptr->tree_repr, size);
  map_copy(fat_ptr, new_ptr->buffer, size);
  ptr->array_repr = new_ptr;
  //## THE TREE REPRESENTATION IS CLEARED FOR DEBUGGING ONLY. THE MEMORY IS NOT RELEASED ANYWAY...
  ptr->tree_repr = NULL;
}

BIN_REL_OBJ *rearrange_if_needed_and_get_bin_rel_ptr(OBJ map) {
  assert(is_ne_bin_rel(map));

  if (!is_ne_map(map) || is_array_map(map))
    return get_bin_rel_ptr(map);

  MIXED_REPR_MAP_OBJ *mixed_repr_ptr = get_mixed_repr_map_ptr(map);
  if (mixed_repr_ptr->array_repr == NULL)
    rearrange_map_as_array(mixed_repr_ptr, read_size_field(map));
  return mixed_repr_ptr->array_repr;
}
