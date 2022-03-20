#include "lib.h"


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
