#include "lib.h"


const uint32 MIN_TREE_SIZE = 9;


static FAT_SET_PTR make_empty_set_ptr() {
  FAT_SET_PTR fat_ptr;
  fat_ptr.ptr.array = NULL;
  fat_ptr.size = 0;
  fat_ptr.is_array_or_empty = true;
  return fat_ptr;
}

static FAT_SET_PTR make_array_set_ptr(OBJ *ptr, uint32 size) {
  assert(size > 0);

for (uint32 i=1 ; i < size ; i++)
  assert(comp_objs(ptr[i-1], ptr[i]) > 0);
for (uint32 i=0 ; i < size ; i++)
  assert(get_obj_type(ptr[i]) != TYPE_NOT_A_VALUE_OBJ);

  FAT_SET_PTR fat_ptr;
  fat_ptr.ptr.array = ptr;
  fat_ptr.size = size;
  fat_ptr.is_array_or_empty = true;
  return fat_ptr;
}

static FAT_SET_PTR make_tree_set_ptr(TREE_SET_NODE *ptr, uint32 size) {
  assert(size > 0);
  // assert(size >= MIN_TREE_SIZE); //## TRY AGAIN LATER

  FAT_SET_PTR fat_ptr;
  fat_ptr.ptr.tree = ptr;
  fat_ptr.size = size;
  fat_ptr.is_array_or_empty = false;
  return fat_ptr;
}

////////////////////////////////////////////////////////////////////////////////

static void set_copy(FAT_SET_PTR fat_ptr, OBJ *dest) {
  if (fat_ptr.size > 0) {
    if (fat_ptr.is_array_or_empty) {
      memcpy(dest, fat_ptr.ptr.array, fat_ptr.size * sizeof(OBJ));
    }
    else {
      TREE_SET_NODE *ptr = fat_ptr.ptr.tree;
      FAT_SET_PTR left = ptr->left;
      set_copy(left, dest);
      dest[left.size] = ptr->value;
      set_copy(ptr->right, dest + left.size + 1);
    }
  }
}

static void set_copy_range(FAT_SET_PTR fat_ptr, uint32 first, uint32 count, OBJ *dest) {
  assert(first + count <= fat_ptr.size);

  if (count > 0) {
    if (fat_ptr.is_array_or_empty) {
      memcpy(dest, fat_ptr.ptr.array + first, count * sizeof(OBJ));
    }
    else {
      TREE_SET_NODE *ptr = fat_ptr.ptr.tree;
      FAT_SET_PTR left = ptr->left;

      if (left.size >= first) {
        uint32 left_copied = min_u32(count, left.size - first);
        if (left_copied > 0)
          set_copy_range(left, first, left_copied, dest);
        if (left_copied < count) {
          dest[left_copied] = ptr->value;
          uint32 right_copied = count - left_copied - 1;
          if (right_copied > 0)
            set_copy_range(ptr->right, 0, right_copied, dest + left_copied + 1);
        }
      }
      else
        set_copy_range(ptr->right, first - left.size - 1, count, dest);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_SET_PTR array_set_insert(OBJ *elts, uint32 size, OBJ elt) {
  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(elts, size, elt);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (found)
    return make_array_set_ptr(elts, size);

  if (size + 1 < MIN_TREE_SIZE) {
    SET_OBJ *new_ptr = new_set(size + 1);
    OBJ *new_elts = new_ptr->buffer;

    if (index > 0)
      memcpy(new_elts, elts, index * sizeof(OBJ));
    new_elts[index] = elt;
    uint32 gt_count = size - index;
    if (gt_count > 0)
      memcpy(new_elts + index + 1, elts + index, gt_count * sizeof(OBJ));

    return make_array_set_ptr(new_elts, size + 1);
  }
  else {
    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = elt;
    new_ptr->left = index > 0 ? make_array_set_ptr(elts, index) : make_empty_set_ptr();
    new_ptr->right = index < size ? make_array_set_ptr(elts + index, size - index) : make_empty_set_ptr();
    new_ptr->priority = rand();
    return make_tree_set_ptr(new_ptr, size + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_SET_PTR bin_tree_set_insert(TREE_SET_NODE *ptr, uint32 size, OBJ elt) {
  OBJ value = ptr->value;
  int cr = comp_objs(elt, value);
  if (cr == 0)
    return make_tree_set_ptr(ptr, size);

  if (cr > 0) { // elt < ptr->value
    // Inserting into the left subtree
    FAT_SET_PTR left_ptr = ptr->left;
    FAT_SET_PTR updated_left_ptr;

    if (left_ptr.is_array_or_empty)
     updated_left_ptr = array_set_insert(left_ptr.ptr.array, left_ptr.size, elt);
    else
      updated_left_ptr = bin_tree_set_insert(left_ptr.ptr.tree, left_ptr.size, elt);

    assert((updated_left_ptr.ptr.array == left_ptr.ptr.array) == (updated_left_ptr.size == left_ptr.size));

    // Checking that the subtree has actually changed, that is, that it didn't already contain the new element
    if (updated_left_ptr.size == left_ptr.size)
      return make_tree_set_ptr(ptr, size);

    assert(updated_left_ptr.size == left_ptr.size + 1);

    if (!updated_left_ptr.is_array_or_empty) {
      // The updated left subset is actually a tree, so we need to make sure the heap property is maintained
      if (updated_left_ptr.ptr.tree->priority > ptr->priority) {
        // The updated left subtree has a higher priority, we need to perform a rotation
        FAT_SET_PTR right_ptr = ptr->right;
        FAT_SET_PTR updated_left_ptr_right_ptr = updated_left_ptr.ptr.tree->right;

        TREE_SET_NODE *new_ptr = new_tree_set_node();
        new_ptr->value = ptr->value;
        new_ptr->left = updated_left_ptr_right_ptr;
        new_ptr->right = right_ptr;
        new_ptr->priority = ptr->priority;

        uint32 new_right_size = 1 + updated_left_ptr_right_ptr.size + right_ptr.size;
        updated_left_ptr.ptr.tree->right = make_tree_set_ptr(new_ptr, new_right_size);
        return make_tree_set_ptr(updated_left_ptr.ptr.tree, size + 1);
      }
    }

    // Either the updated left subset is an array, or its priority is lower than the root's, so no need for rotations
    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_set_ptr(new_ptr, size + 1);
  }
  else { // elt > ptr->value
    // Inserting into the right subtree
    FAT_SET_PTR right_ptr = ptr->right;
    FAT_SET_PTR updated_right_ptr;

    if (right_ptr.is_array_or_empty)
      updated_right_ptr = array_set_insert(right_ptr.ptr.array, right_ptr.size, elt);
    else
      updated_right_ptr = bin_tree_set_insert(right_ptr.ptr.tree, right_ptr.size, elt);

    assert((updated_right_ptr.ptr.array == right_ptr.ptr.array) == (updated_right_ptr.size == right_ptr.size));

    // Checking that the subtree has actually changed, that is, that it didn't already contain the new element
    if (updated_right_ptr.size == right_ptr.size)
      return make_tree_set_ptr(ptr, size);

    assert(updated_right_ptr.size == right_ptr.size + 1);

    if (!updated_right_ptr.is_array_or_empty) {
      // The updated right subset is actually a tree, so we need to make sure the heap property is maintained
      if (updated_right_ptr.ptr.tree->priority > ptr->priority) {
        // The updated right subtree has a higher priority, we need to perform a rotation
        FAT_SET_PTR left_ptr = ptr->left;
        FAT_SET_PTR updated_right_ptr_left_ptr = updated_right_ptr.ptr.tree->left;

        TREE_SET_NODE *new_ptr = new_tree_set_node();
        new_ptr->value = ptr->value;
        new_ptr->left = left_ptr;
        new_ptr->right = updated_right_ptr_left_ptr;
        new_ptr->priority = ptr->priority;

        uint32 new_left_size = 1 + left_ptr.size + updated_right_ptr_left_ptr.size;
        updated_right_ptr.ptr.tree->left = make_tree_set_ptr(new_ptr, new_left_size);
        return make_tree_set_ptr(updated_right_ptr.ptr.tree, size + 1);
      }
    }

    // Either the right subset is an array, or its priority if lower than the root's, so no need to perform a rotation
    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = value;
    new_ptr->left = ptr->left;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    return make_tree_set_ptr(new_ptr, size + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ set_insert(OBJ set, OBJ elt) {
  uint32 size = read_size_field(set);

  if (size == 0) {
    SET_OBJ *new_ptr = new_set(1);
    OBJ *array = new_ptr->buffer;
    array[0] = elt;
    return make_set(new_ptr, 1);
  }

  FAT_SET_PTR updated_ptr;

  if (is_array_set(set)) {
    OBJ *elts = get_set_elts_ptr(set);
    updated_ptr = array_set_insert(elts, size, elt);
  }
  else {
    MIXED_REPR_SET_OBJ *ptr = get_mixed_repr_set_ptr(set);
    if (ptr->array_repr != NULL)
      updated_ptr = array_set_insert(ptr->array_repr->buffer, size, elt);
    else
      updated_ptr = bin_tree_set_insert(ptr->tree_repr, size, elt);
  }

  if (updated_ptr.is_array_or_empty)
    return make_set((SET_OBJ *) updated_ptr.ptr.array, updated_ptr.size);

  MIXED_REPR_SET_OBJ *new_ptr = new_mixed_repr_set();
  new_ptr->array_repr = NULL;
  new_ptr->tree_repr = updated_ptr.ptr.tree;
  return make_mixed_repr_set(new_ptr, updated_ptr.size);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static FAT_SET_PTR remove_min_value(FAT_SET_PTR fat_ptr, OBJ *value_ptr) {
  uint32 size = fat_ptr.size;
  assert(size > 0);

  if (fat_ptr.is_array_or_empty) {
    OBJ *elts = fat_ptr.ptr.array;
    *value_ptr = elts[0];
    return size > 1 ? make_array_set_ptr(elts + 1, size - 1) : make_empty_set_ptr();
  }

  // assert(size >= MIN_TREE_SIZE); //## TRY AGAIN LATER

  TREE_SET_NODE *ptr = fat_ptr.ptr.tree;
  if (size == 1) {
    *value_ptr = ptr->value;
    return make_empty_set_ptr();
  }

  FAT_SET_PTR left_ptr = ptr->left;
  if (left_ptr.size == 0) {
    *value_ptr = ptr->value;
    return ptr->right;
  }

  if (size > MIN_TREE_SIZE) {
    FAT_SET_PTR updated_left_ptr = remove_min_value(left_ptr, value_ptr);
    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = ptr->value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_set_ptr(new_ptr, size - 1);
  }

  set_copy_range(fat_ptr, 0, 1, value_ptr);
  SET_OBJ *new_ptr = new_set(size - 1);
  set_copy_range(fat_ptr, 1, size - 1, new_ptr->buffer);
  return make_array_set_ptr(new_ptr->buffer, size - 1);
}

static FAT_SET_PTR remove_max_value(FAT_SET_PTR fat_ptr, OBJ *value_ptr) {
  uint32 size = fat_ptr.size;
  assert(size > 0);

  if (fat_ptr.is_array_or_empty) {
    OBJ *elts = fat_ptr.ptr.array;
    *value_ptr = elts[size - 1];
    return size > 1 ? make_array_set_ptr(elts, size - 1) : make_empty_set_ptr();
  }

  // assert(size >= MIN_TREE_SIZE); //## TRY AGAIN LATER

  TREE_SET_NODE *ptr = fat_ptr.ptr.tree;
  if (size == 1) {
    *value_ptr = ptr->value;
    return make_empty_set_ptr();
  }

  FAT_SET_PTR right_ptr = ptr->right;

  if (right_ptr.size == 0) {
    *value_ptr = ptr->value;
    return ptr->left;
  }

  if (size > MIN_TREE_SIZE) {
    FAT_SET_PTR updated_right_ptr = remove_max_value(right_ptr, value_ptr);
    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = ptr->value;
    new_ptr->left = ptr->left;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    return make_tree_set_ptr(new_ptr, size - 1);
  }

  set_copy_range(fat_ptr, size - 1, 1, value_ptr);
  SET_OBJ *new_ptr = new_set(size - 1);
  set_copy_range(fat_ptr, 0, size - 1, new_ptr->buffer);
  return make_array_set_ptr(new_ptr->buffer, size - 1);
}

////////////////////////////////////////////////////////////////////////////////

static FAT_SET_PTR array_set_remove(OBJ *elts, uint32 size, OBJ elt) {
  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(elts, size, elt);
  uint32 index = (uint32) code;
  bool found = (code >> 32) == 0;

  if (!found)
    return size > 0 ? make_array_set_ptr(elts, size) : make_empty_set_ptr();

  if (size == 1)
    return make_empty_set_ptr();

  if (index == 0)
    return make_array_set_ptr(elts + 1, size - 1);

  if (index == size - 1)
    return make_array_set_ptr(elts, size - 1);

  if (size <= MIN_TREE_SIZE) {
    SET_OBJ *new_ptr = new_set(size - 1);

    if (index > 0)
      memcpy(new_ptr->buffer, elts, index * sizeof(OBJ));
    uint32 gt_count = size - index - 1;
    if (gt_count > 0)
      memcpy(new_ptr->buffer + index, elts + index + 1, gt_count * sizeof(OBJ));

    return make_array_set_ptr(new_ptr->buffer, size - 1);
  }
  else {
    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->priority = rand();

    if (index == 1) {
      new_ptr->value = elts[0];
      new_ptr->left = make_empty_set_ptr();
      new_ptr->right = make_array_set_ptr(elts + 2, size - 2);
    }
    else if (index == size - 2) {
      new_ptr->value = elts[size - 1];
      new_ptr->left = make_array_set_ptr(elts, size - 2);
      new_ptr->right = make_empty_set_ptr();
    }
    else if (index < size / 2) {
      new_ptr->value = elts[index + 1];
      new_ptr->left = make_array_set_ptr(elts, index);
      new_ptr->right = make_array_set_ptr(elts + index + 2, size - index - 2);
    }
    else {
      new_ptr->value = elts[index - 1];
      new_ptr->left = make_array_set_ptr(elts, index - 1);
      new_ptr->right = make_array_set_ptr(elts + index + 1, size - index - 1);
    }

    return make_tree_set_ptr(new_ptr, size - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

static FAT_SET_PTR bin_tree_set_remove(TREE_SET_NODE *ptr, uint32 size, OBJ elt) {
  // assert(size >= MIN_TREE_SIZE); //## TRY AGAIN LATER

  OBJ value = ptr->value;
  int cr = comp_objs(elt, value);

  if (cr == 0) {
    FAT_SET_PTR left_ptr = ptr->left;
    FAT_SET_PTR right_ptr = ptr->right;

    if (left_ptr.size == 0)
      return right_ptr;

    if (right_ptr.size == 0)
      return left_ptr;

    // assert(size > MIN_TREE_SIZE || (left_ptr.is_array_or_empty && right_ptr.is_array_or_empty)); //## TRY AGAIN LATER

    if (size == MIN_TREE_SIZE) {
      SET_OBJ *new_ptr = new_set(size - 1);
      set_copy(left_ptr, new_ptr->buffer);
      set_copy(right_ptr, new_ptr->buffer + left_ptr.size);
      return make_array_set_ptr(new_ptr->buffer, size - 1);
    }

    if (left_ptr.size == 1) {
      TREE_SET_NODE *new_ptr = new_tree_set_node();
      new_ptr->value = left_ptr.ptr.array[0];
      new_ptr->left = make_empty_set_ptr();
      new_ptr->right = right_ptr;
      new_ptr->priority = ptr->priority;
      return make_tree_set_ptr(new_ptr, size - 1);
    }

    if (right_ptr.size == 1) {
      TREE_SET_NODE *new_ptr = new_tree_set_node();
      new_ptr->value = right_ptr.ptr.array[0];
      new_ptr->left = left_ptr;
      new_ptr->right = make_empty_set_ptr();
      new_ptr->priority = ptr->priority;
      return make_tree_set_ptr(new_ptr, size - 1);
    }

    if (left_ptr.size > right_ptr.size) {
      OBJ max_value;
      FAT_SET_PTR updated_left_ptr = remove_max_value(left_ptr, &max_value);
      TREE_SET_NODE *new_ptr = new_tree_set_node();
      new_ptr->value = max_value;
      new_ptr->left = updated_left_ptr;
      new_ptr->right = right_ptr;
      new_ptr->priority = ptr->priority;
      return make_tree_set_ptr(new_ptr, size - 1);
    }
    else {
      OBJ max_value;
      FAT_SET_PTR updated_right_ptr = remove_min_value(right_ptr, &max_value);
      TREE_SET_NODE *new_ptr = new_tree_set_node();
      new_ptr->value = max_value;
      new_ptr->left = left_ptr;
      new_ptr->right = updated_right_ptr;
      new_ptr->priority = ptr->priority;
      return make_tree_set_ptr(new_ptr, size - 1);
    }
  }

  if (cr > 0) { // elt < ptr->value
    // Removing the element from the left subtree
    FAT_SET_PTR left_ptr = ptr->left;
    FAT_SET_PTR updated_left_ptr;

    if (left_ptr.is_array_or_empty)
      updated_left_ptr = array_set_remove(left_ptr.ptr.array, left_ptr.size, elt);
    else
      updated_left_ptr = bin_tree_set_remove(left_ptr.ptr.tree, left_ptr.size, elt);

    assert((updated_left_ptr.size != left_ptr.size) || (updated_left_ptr.ptr.array == left_ptr.ptr.array));

    // Checking that the subtree has actually changed, that is, that it actually contained the element to be removed
    if (updated_left_ptr.size == left_ptr.size)
      return make_tree_set_ptr(ptr, size);

    assert(updated_left_ptr.size == left_ptr.size - 1);

    if (!updated_left_ptr.is_array_or_empty) {
      // assert(size > MIN_TREE_SIZE); //## TRY AGAIN LATER

      // The updated left subset is actually a tree, so we need to make sure the heap property is maintained
      if (updated_left_ptr.ptr.tree->priority > ptr->priority) {
        // The updated left subtree has a higher priority, we need to perform a rotation
        FAT_SET_PTR right_ptr = ptr->right;
        FAT_SET_PTR updated_left_ptr_right_ptr = updated_left_ptr.ptr.tree->right;

        TREE_SET_NODE *new_ptr = new_tree_set_node();
        new_ptr->value = ptr->value;
        new_ptr->left = updated_left_ptr_right_ptr;
        new_ptr->right = right_ptr;
        new_ptr->priority = ptr->priority;

        uint32 new_right_size = 1 + updated_left_ptr_right_ptr.size + right_ptr.size;
        updated_left_ptr.ptr.tree->right = make_tree_set_ptr(new_ptr, new_right_size);
        return make_tree_set_ptr(new_ptr, size + 1);
      }
    }

    if (size == MIN_TREE_SIZE) {
      SET_OBJ *new_ptr = new_set(size - 1);
      set_copy(updated_left_ptr, new_ptr->buffer);
      new_ptr->buffer[updated_left_ptr.size] = value;
      set_copy(ptr->right, new_ptr->buffer + updated_left_ptr.size + 1);
      return make_array_set_ptr(new_ptr->buffer, size - 1);
    }

    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = value;
    new_ptr->left = updated_left_ptr;
    new_ptr->right = ptr->right;
    new_ptr->priority = ptr->priority;
    return make_tree_set_ptr(new_ptr, size - 1);
  }
  else { // elt > ptr->value
    // Removing the element from the right subtree
    FAT_SET_PTR right_ptr = ptr->right;
    FAT_SET_PTR updated_right_ptr;

    if (right_ptr.is_array_or_empty)
      updated_right_ptr = array_set_remove(right_ptr.ptr.array, right_ptr.size, elt);
    else
      updated_right_ptr = bin_tree_set_remove(right_ptr.ptr.tree, right_ptr.size, elt);

    assert((updated_right_ptr.size != right_ptr.size) || (updated_right_ptr.ptr.array == right_ptr.ptr.array));

    // Checking that the subtree has actually changed, that is, that it actually contained the element to remove
    if (updated_right_ptr.size == right_ptr.size)
      return make_tree_set_ptr(ptr, size);

    assert(updated_right_ptr.size == right_ptr.size - 1);

    if (!updated_right_ptr.is_array_or_empty) {
      // assert(size > MIN_TREE_SIZE); //## TRY AGAIN LATER

      // The updated right subset is actually a tree, so we need to make sure the heap property is maintained
      if (updated_right_ptr.ptr.tree->priority > ptr->priority) {
        // The updated right subtree has a higher priority, we need to perform a rotation
        FAT_SET_PTR left_ptr = ptr->left;
        FAT_SET_PTR updated_right_ptr_left_ptr = updated_right_ptr.ptr.tree->left;

        TREE_SET_NODE *new_ptr = new_tree_set_node();
        new_ptr->value = ptr->value;
        new_ptr->left = left_ptr;
        new_ptr->right = updated_right_ptr_left_ptr;
        new_ptr->priority = ptr->priority;

        uint32 new_left_size = 1 + left_ptr.size + updated_right_ptr_left_ptr.size;
        updated_right_ptr.ptr.tree->left = make_tree_set_ptr(new_ptr, new_left_size);
        return make_tree_set_ptr(updated_right_ptr.ptr.tree, size + 1);
      }
    }

    FAT_SET_PTR left_ptr = ptr->left;

    if (size == MIN_TREE_SIZE) {
      SET_OBJ *new_ptr = new_set(size - 1);
      set_copy(left_ptr, new_ptr->buffer);
      new_ptr->buffer[left_ptr.size] = value;
      set_copy(updated_right_ptr, new_ptr->buffer + left_ptr.size + 1);
      return make_array_set_ptr(new_ptr->buffer, size - 1);
    }

    TREE_SET_NODE *new_ptr = new_tree_set_node();
    new_ptr->value = value;
    new_ptr->left = left_ptr;
    new_ptr->right = updated_right_ptr;
    new_ptr->priority = ptr->priority;
    return make_tree_set_ptr(new_ptr, size - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ set_remove(OBJ set, OBJ elt) {
  uint32 size = read_size_field(set);

  if (size == 0)
    return set;

  FAT_SET_PTR updated_ptr;

  if (is_array_set(set)) {
    OBJ *elts = get_set_elts_ptr(set);
    updated_ptr = array_set_remove(elts, size, elt);
  }
  else {
    MIXED_REPR_SET_OBJ *ptr = get_mixed_repr_set_ptr(set);
    if (ptr->array_repr != NULL)
      updated_ptr = array_set_remove(ptr->array_repr->buffer, size, elt);
    else
      updated_ptr = bin_tree_set_remove(ptr->tree_repr, size, elt);
  }

  if (updated_ptr.size == 0)
    return make_empty_rel();

  if (updated_ptr.is_array_or_empty)
    return make_set((SET_OBJ *) updated_ptr.ptr.array, updated_ptr.size);

  MIXED_REPR_SET_OBJ *new_ptr = new_mixed_repr_set();
  new_ptr->array_repr = NULL;
  new_ptr->tree_repr = updated_ptr.ptr.tree;
  return make_mixed_repr_set(new_ptr, updated_ptr.size);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool tree_set_contains(TREE_SET_NODE *, OBJ);

bool set_contains(FAT_SET_PTR fat_ptr, OBJ elt) {
  if (fat_ptr.size == 0)
    return false;

  if (!fat_ptr.is_array_or_empty)
    return tree_set_contains(fat_ptr.ptr.tree, elt);

  OBJ *elts = fat_ptr.ptr.array;

  uint64 code = encoded_index_or_insertion_point_in_unique_sorted_array(elts, fat_ptr.size, elt);
  bool found = (code >> 32) == 0;
  return found;
}

bool tree_set_contains(TREE_SET_NODE *ptr, OBJ value) {
  int cr = comp_objs(value, ptr->value);

  if (cr == 0)
    return true;

  if (cr > 0) // value < ptr->value
    return set_contains(ptr->left, value);
  else // value > ptr->value
    return set_contains(ptr->right, value);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void rearrange_set_as_array(MIXED_REPR_SET_OBJ *ptr, uint32 size) {
  assert(ptr->array_repr == NULL);

  SET_OBJ *new_ptr = new_set(size);
  FAT_SET_PTR fat_ptr = make_tree_set_ptr(ptr->tree_repr, size);
  set_copy(fat_ptr, new_ptr->buffer);
  ptr->array_repr = new_ptr;
  //## THE TREE REPRESENTATION IS CLEARED FOR DEBUGGING ONLY. THE MEMORY IS NOT RELEASED ANYWAY...
  ptr->tree_repr = NULL;
}

OBJ *rearrange_if_needed_and_get_set_elts_ptr(OBJ set) {
  assert(is_ne_set(set));

  if (is_array_set(set))
    return get_set_elts_ptr(set);

  MIXED_REPR_SET_OBJ *ptr = get_mixed_repr_set_ptr(set);
  if (ptr->array_repr == NULL)
    rearrange_set_as_array(ptr, read_size_field(set));
  return ptr->array_repr->buffer;
}
