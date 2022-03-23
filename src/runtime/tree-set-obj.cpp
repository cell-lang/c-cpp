#include "lib.h"


static void set_obj_copy(OBJ set, OBJ *dest) {
  if (!is_empty_rel(set)) {
    if (is_array_set(set)) {
      uint32 size = read_size_field(set);
      OBJ *elts = get_set_elts_ptr(set);
      memcpy(dest, elts, size * sizeof(OBJ));
    }
    else {
      assert(is_bin_tree_set(set));
      BIN_TREE_SET_OBJ *ptr = get_tree_set_ptr(set);
      OBJ left_subtree = ptr->left_subtree;
      set_obj_copy(left_subtree, dest);
      uint32 offset = read_size_field(left_subtree);
      dest[offset++] = ptr->value;
      set_obj_copy(ptr->right_subtree, dest + offset);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static OBJ array_set_insert(OBJ set, OBJ elt) {
  uint32 size = read_size_field(set);
  OBJ *elts = get_set_elts_ptr(set);

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(elts, size, elt);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found)
    return set;

  if (size <= 8) {
    SET_OBJ *new_ptr = new_set(size);
    OBJ *new_elts = new_ptr->buffer;

    if (index > 0)
      memcpy(new_elts, elts, index * sizeof(OBJ));
    new_elts[index] = elt;
    uint32 gt_count = size - index - 1;
    if (gt_count > 0)
      memcpy(new_elts + index + 1, elts + index + 1, gt_count * sizeof(OBJ));

    return make_set(new_ptr, size + 1);
  }
  else {
    BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
    new_ptr->value = elt;
    new_ptr->left_subtree = index > 0 ? make_set((SET_OBJ *) elts, index) : make_empty_rel();
    new_ptr->right_subtree = index < size ? make_set((SET_OBJ *) (elts + index), size - index) : make_empty_rel();
    new_ptr->priority = rand();
    return make_tree_set(new_ptr, size + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

static OBJ bin_tree_set_insert(OBJ set, OBJ elt) {
  BIN_TREE_SET_OBJ *ptr = get_tree_set_ptr(set);
  OBJ value = ptr->value;
  int cr = comp_objs(elt, value);
  if (cr == 0)
    return set;

  uint32 size = read_size_field(set);

  if (cr > 0) { // elt < ptr->value
    // Inserting into the left subtree
    OBJ left_subtree = ptr->left_subtree;
    OBJ updated_left_subtree = set_insert(left_subtree, elt);

    // Checking that the subtree has actually changed, that is, that it didn't already contain the new element
    if (are_shallow_eq(updated_left_subtree, left_subtree))
      return set;

    if (is_bin_tree_set(updated_left_subtree)) {
      // The updated left subset is actually a tree, so we need to make sure the heap property is maintained
      BIN_TREE_SET_OBJ *updated_left_subtree_ptr = get_tree_set_ptr(updated_left_subtree);
      if (updated_left_subtree_ptr->priority > ptr->priority) {
        // The updated left subtree has a higher priority, we need to perform a rotation
        OBJ right_subtree = ptr->right_subtree;
        OBJ updated_left_subtree_right_subtree = updated_left_subtree_ptr->right_subtree;

        BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
        new_ptr->value = ptr->value;
        new_ptr->left_subtree = updated_left_subtree_right_subtree;
        new_ptr->right_subtree = right_subtree;
        new_ptr->priority = ptr->priority;

        uint32 new_right_subtree_size = 1 + read_size_field(right_subtree) + read_size_field(updated_left_subtree_right_subtree);
        updated_left_subtree_ptr->right_subtree = make_tree_set(new_ptr, new_right_subtree_size);
        return make_tree_set(updated_left_subtree_ptr, size + 1);
      }
    }

    // Either the updated left subset is an array, or its priority is lower than the root's, so no need for rotations
    BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
    new_ptr->value = value;
    new_ptr->left_subtree = updated_left_subtree;
    new_ptr->right_subtree = ptr->right_subtree;
    new_ptr->priority = ptr->priority;
    return make_tree_set(new_ptr, size + 1);
  }
  else { // elt > ptr->value
    // Inserting into the right subtree
    OBJ right_subtree = ptr->right_subtree;
    OBJ updated_right_subtree = set_insert(right_subtree, elt);

    // Checking that the subtree has actually changed, that is, that it didn't already contain the new element
    if (are_shallow_eq(updated_right_subtree, right_subtree))
      return set;

    if (is_bin_tree_set(updated_right_subtree)) {
      // The updated right subset is actually a tree, so we need to make sure the heap property is maintained
      BIN_TREE_SET_OBJ *updated_right_subtree_ptr = get_tree_set_ptr(updated_right_subtree);
      if (updated_right_subtree_ptr->priority > ptr->priority) {
        // The updated right subtree has a higher priority, we need to perform a rotation
        OBJ left_subtree = ptr->left_subtree;
        OBJ updated_right_subtree_left_subtree = updated_right_subtree_ptr->left_subtree;

        BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
        new_ptr->value = ptr->value;
        new_ptr->left_subtree = left_subtree;
        new_ptr->right_subtree = updated_right_subtree_left_subtree;
        new_ptr->priority = ptr->priority;

        uint32 new_left_subtree_size = 1 + read_size_field(left_subtree) + read_size_field(updated_right_subtree_left_subtree);
        updated_right_subtree_ptr->left_subtree = make_tree_set(new_ptr, new_left_subtree_size);
        return make_tree_set(updated_right_subtree_ptr, size + 1);
      }
    }

    // Either the right subset is an array, or its priority if lower than the root's, so no need to perform a rotation
    BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
    new_ptr->value = value;
    new_ptr->left_subtree = ptr->left_subtree;
    new_ptr->right_subtree = updated_right_subtree;
    new_ptr->priority = ptr->priority;
    return make_tree_set(new_ptr, size + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ set_insert(OBJ set, OBJ elt) {
  if (is_empty_rel(set)) {
    SET_OBJ *new_ptr = new_set(1);
    OBJ *array = new_ptr->buffer;
    array[0] = elt;
    return make_set(new_ptr, 1);
  }

  if (is_array_set(set))
    return array_set_insert(set, elt);

  assert(is_bin_tree_set(set));
  return bin_tree_set_insert(set, elt);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ remove_min_value(OBJ set, OBJ *value_ptr) {
  uint32 size = read_size_field(set);
  assert(size > 0);

  if (is_array_set(set)) {
    OBJ *elts = get_set_elts_ptr(set);
    *value_ptr = elts[0];
    return size > 1 ? make_set((SET_OBJ *) (elts + 1), size - 1) : make_empty_rel();
  }
  else {
    assert(is_bin_tree_set(set));
    assert(size > 8);

    BIN_TREE_SET_OBJ *ptr = get_tree_set_ptr(set);
    OBJ left_subtree = ptr->left_subtree;
    if (is_empty_rel(left_subtree)) {
      *value_ptr = ptr->value;
      return ptr->right_subtree;
    }

    if (size > 9) {
      OBJ updated_left_subtree = remove_min_value(left_subtree, value_ptr);
      BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
      new_ptr->value = ptr->value;
      new_ptr->left_subtree = updated_left_subtree;
      new_ptr->right_subtree = ptr->right_subtree;
      new_ptr->priority = ptr->priority;
      return make_tree_set(new_ptr, size - 1);
    }
    else {
      SET_OBJ *new_ptr = new_set(size - 1);

      uint32 left_size = read_size_field(left_subtree);
      OBJ *left_ptr = get_set_elts_ptr(left_subtree);
      if (left_size > 1)
        memcpy(new_ptr->buffer, left_ptr + 1, (left_size - 1) * sizeof(OBJ));

      new_ptr->buffer[left_size - 1] = ptr->value;

      OBJ right_subtree = ptr->right_subtree;
      uint32 right_size = read_size_field(right_subtree);
      if (right_size > 0) {
        OBJ *right_ptr = get_set_elts_ptr(right_subtree);
        memcpy(new_ptr->buffer + left_size, right_ptr, right_size);
      }

      *value_ptr = *left_ptr;
      return make_set(new_ptr, size - 1);
    }
  }
}

OBJ remove_max_value(OBJ set, OBJ *value_ptr) {
  uint32 size = read_size_field(set);
  assert(size > 0);

  if (is_array_set(set)) {
    OBJ *elts = get_set_elts_ptr(set);
    *value_ptr = elts[size - 1];
    return size > 1 ? make_set((SET_OBJ *) elts, size - 1) : make_empty_rel();
  }
  else {
    assert(is_bin_tree_set(set));
    assert(size > 8);

    BIN_TREE_SET_OBJ *ptr = get_tree_set_ptr(set);
    OBJ right_subtree = ptr->right_subtree;
    if (is_empty_rel(right_subtree)) {
      *value_ptr = ptr->value;
      return ptr->left_subtree;
    }

    if (size > 9) {
      OBJ updated_right_subtree = remove_max_value(right_subtree, value_ptr);
      BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
      new_ptr->value = ptr->value;
      new_ptr->left_subtree = ptr->left_subtree;
      new_ptr->right_subtree = updated_right_subtree;
      new_ptr->priority = ptr->priority;
      return make_tree_set(new_ptr, size - 1);
    }
    else {
      SET_OBJ *new_ptr = new_set(size - 1);

      OBJ left_subtree = ptr->left_subtree;
      uint32 left_size = read_size_field(left_subtree);
      if (left_size > 0) {
        OBJ *left_ptr = get_set_elts_ptr(left_subtree);
        memcpy(new_ptr->buffer, left_ptr, left_size * sizeof(OBJ));
      }

      new_ptr->buffer[left_size] = ptr->value;

      uint32 right_size = read_size_field(right_subtree);
      OBJ *right_ptr = get_set_elts_ptr(right_subtree);
      if (right_size > 1)
        memcpy(new_ptr->buffer + left_size + 1, right_ptr, right_size - 1);

      *value_ptr = right_ptr[right_size - 1];
      return make_set(new_ptr, size - 1);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ array_set_remove(OBJ set, OBJ elt) {
  uint32 size = read_size_field(set);
  OBJ *elts = get_set_elts_ptr(set);

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(elts, size, elt);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (!found)
    return set;

  if (size == 1)
    return make_empty_rel();

  if (index == 0)
    return make_set((SET_OBJ *) elts + 1, size - 1);

  if (index == size - 1)
    return make_set((SET_OBJ *) elts, size - 1);

  if (size <= 9) {
    SET_OBJ *new_ptr = new_set(size);
    OBJ *new_elts = new_ptr->buffer;

    if (index > 0)
      memcpy(new_elts, elts, index * sizeof(OBJ));
    uint32 gt_count = size - index - 1;
    if (gt_count > 0)
      memcpy(new_elts + index, elts + index + 1, gt_count * sizeof(OBJ));

    return make_set(new_ptr, size - 1);
  }
  else {
    BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
    new_ptr->priority = rand();

    if (index == 1) {
      new_ptr->value = elts[0];
      new_ptr->left_subtree = make_empty_rel();
      new_ptr->right_subtree = make_set((SET_OBJ *) elts + 2, size - 2);
    }
    else if (index == size - 2) {
      new_ptr->value = elts[size - 1];
      new_ptr->left_subtree = make_set((SET_OBJ *) elts, size - 2);
      new_ptr->right_subtree = make_empty_rel();
    }
    else if (index < size / 2) {
      new_ptr->value = elts[index + 1];
      new_ptr->left_subtree = make_set((SET_OBJ *) elts, index);
      new_ptr->right_subtree = make_set((SET_OBJ *) elts + index + 2, size - index - 2);
    }
    else {
      new_ptr->value = elts[index - 1];
      new_ptr->left_subtree = make_set((SET_OBJ *) elts, index - 1);
      new_ptr->right_subtree = make_set((SET_OBJ *) elts + index + 1, size - index - 1);
    }

    return make_tree_set(new_ptr, size - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ bin_tree_set_remove(OBJ set, OBJ elt) {
  BIN_TREE_SET_OBJ *ptr = get_tree_set_ptr(set);
  OBJ value = ptr->value;
  int cr = comp_objs(elt, value);

  if (cr == 0) {
    OBJ left_subtree = ptr->left_subtree;
    OBJ right_subtree = ptr->right_subtree;

    uint32 left_size = read_size_field(left_subtree);
    uint32 right_size = read_size_field(right_subtree);

    if (left_size == 0)
      return right_subtree;

    if (right_size == 0)
      return left_subtree;

    uint32 new_size = left_size + right_size;
    assert(new_size >= 8);

    if (new_size == 8) {
      SET_OBJ *new_ptr = new_set(8);
      set_obj_copy(left_subtree, (OBJ *) new_ptr);
      set_obj_copy(right_subtree, ((OBJ *) new_ptr) + left_size);
      return make_set(new_ptr, 8);
    }

    if (left_size == 1) {
      OBJ *left_ptr = get_set_elts_ptr(left_subtree);
      BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
      new_ptr->value = *left_ptr;
      new_ptr->left_subtree = make_empty_rel();
      new_ptr->right_subtree = right_subtree;
      new_ptr->priority = ptr->priority;
      return make_tree_set(new_ptr, new_size);
    }

    if (right_size == 1) {
      OBJ *right_ptr = get_set_elts_ptr(right_subtree);
      BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
      new_ptr->value = *right_ptr;
      new_ptr->left_subtree = left_subtree;
      new_ptr->right_subtree = make_empty_rel();
      new_ptr->priority = ptr->priority;
      return make_tree_set(new_ptr, new_size);
    }

    if (left_size > right_size) {
      OBJ max_value;
      OBJ updated_left_subtree = remove_max_value(left_subtree, &max_value);
      BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
      new_ptr->value = max_value;
      new_ptr->left_subtree = updated_left_subtree;
      new_ptr->right_subtree = right_subtree;
      new_ptr->priority = ptr->priority;
      return make_tree_set(new_ptr, new_size);
    }
    else {
      OBJ max_value;
      OBJ updated_right_subtree = remove_min_value(right_subtree, &max_value);
      BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
      new_ptr->value = max_value;
      new_ptr->left_subtree = left_subtree;
      new_ptr->right_subtree = updated_right_subtree;
      new_ptr->priority = ptr->priority;
      return make_tree_set(new_ptr, new_size);
    }
  }

  uint32 size = read_size_field(set);

  if (cr > 0) { // elt < ptr->value
    // Removing the element from the left subtree
    OBJ left_subtree = ptr->left_subtree;
    OBJ updated_left_subtree = set_remove(left_subtree, elt);

    // Checking that the subtree has actually changed, that is, that it actually contained the element to be removed
    if (are_shallow_eq(updated_left_subtree, left_subtree))
      return set;

    // The priority of the updated subtree is not supposed to have changed
    assert(!is_bin_tree_set(updated_left_subtree) || get_tree_set_ptr(updated_left_subtree)->priority <= ptr->priority);

    uint32 new_size = size - 1;
    uint32 new_left_size = read_size_field(updated_left_subtree);

    assert(new_size >= 8);
    assert(new_left_size == read_size_field(left_subtree) -1);

    if (new_size == 8) {
      SET_OBJ *new_ptr = new_set(8);
      set_obj_copy(updated_left_subtree, (OBJ *) new_ptr);
      set_obj_copy(ptr->right_subtree, ((OBJ *) new_ptr) + new_left_size);
      return make_set(new_ptr, 8);
    }

    BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
    new_ptr->value = value;
    new_ptr->left_subtree = updated_left_subtree;
    new_ptr->right_subtree = ptr->right_subtree;
    new_ptr->priority = ptr->priority;
    return make_tree_set(new_ptr, size + 1);
  }
  else { // elt > ptr->value
    // Removing the element from the right subtree
    OBJ right_subtree = ptr->right_subtree;
    OBJ updated_right_subtree = set_remove(right_subtree, elt);

    // Checking that the subtree has actually changed, that is, that it actually contained the element to remove
    if (are_shallow_eq(updated_right_subtree, right_subtree))
      return set;

    // The priority of the updated subtree is not supposed to have changed
    assert(!is_bin_tree_set(updated_right_subtree) || get_tree_set_ptr(updated_right_subtree)->priority <= ptr->priority);

    OBJ left_subtree = ptr->left_subtree;

    uint32 new_size = size - 1;
    uint32 left_size = read_size_field(left_subtree);

    assert(new_size >= 8);

    if (new_size == 8) {
      SET_OBJ *new_ptr = new_set(8);
      set_obj_copy(left_subtree, (OBJ *) new_ptr);
      set_obj_copy(updated_right_subtree, ((OBJ *) new_ptr) + left_size);
      return make_set(new_ptr, 8);
    }

    BIN_TREE_SET_OBJ *new_ptr = new_bin_tree_set();
    new_ptr->value = value;
    new_ptr->left_subtree = left_subtree;
    new_ptr->right_subtree = updated_right_subtree;
    new_ptr->priority = ptr->priority;
    return make_tree_set(new_ptr, size + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ set_remove(OBJ set, OBJ elt) {
  if (is_empty_rel(set))
    return set;

  if (is_array_set(set))
    return array_set_remove(set, elt);

  assert(is_bin_tree_set(set));
  return bin_tree_set_remove(set, elt);
}
