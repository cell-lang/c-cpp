#include "lib.h"


static uint32 jenkins_hash(uint32 a, uint32 b, uint32 c) {
  a = a - b;  a = a - c;  a = a ^ (c >> 13);
  b = b - c;  b = b - a;  b = b ^ (a <<  8);
  c = c - a;  c = c - b;  c = c ^ (b >> 13);
  a = a - b;  a = a - c;  a = a ^ (c >> 12);
  b = b - c;  b = b - a;  b = b ^ (a << 16);
  c = c - a;  c = c - b;  c = c ^ (b >>  5);
  a = a - b;  a = a - c;  a = a ^ (c >>  3);
  b = b - c;  b = b - a;  b = b ^ (a << 10);
  c = c - a;  c = c - b;  c = c ^ (b >> 15);
  return c;
}

static uint32 hash6432shift(uint32 a, uint32 b) {
  uint64 key = ((uint64) a) | (((uint64) b) << 32);
  key = (~key) + (key << 18); // key = (key << 18) - key - 1;
  key = key ^ (key >> 31);
  key = key * 21; // key = (key + (key << 2)) + (key << 4);
  key = key ^ (key >> 11);
  key = key + (key << 6);
  key = key ^ (key >> 22);
  return (uint32) key;
}

static uint64 murmur64(uint64 h) {
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53L;
  h ^= h >> 33;
  return h;
}

static uint32 murmur64to32(uint32 a, uint32 b) {
  return (uint32) murmur64(((uint64) a) | (((uint64) b) << 32));
}

static uint32 wang32hash(uint32 key) {
  key = ~key + (key << 15); // key = (key << 15) - key - 1;
  key = key ^ (key >> 12);
  key = key + (key << 2);
  key = key ^ (key >> 4);
  key = key * 2057; // key = (key + (key << 3)) + (key << 11);
  key = key ^ (key >> 16);
  return key;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// static uint32 hashcode_32(uint32 n) {
//   return n;
// }

uint32 hashcode_64(uint64 n) {
  return hash6432shift((uint32) n, (uint32) (n >> 32));
}

static uint32 hashcode_32_32(uint32 n1, uint32 n2) {
  return hash6432shift(n1, n2);
}

static uint32 hashcode_32_32_32(uint32 n1, uint32 n2, uint32 n3) {
  return jenkins_hash(n1, n2, n3);
}

static uint32 hashcode_64_64(uint64 n1, uint64 n2) {
  return 31 * hashcode_64(n1) + hashcode_64(n2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static uint64 extra_data_hashcode(OBJ obj) {
  return hashcode_64(obj.extra_data << REPR_INFO_WIDTH);
}

static uint32 compute_ne_int_seq_hashcode(OBJ obj) {
  uint32 len = read_size_field_unchecked(obj);
  uint64 hashcode = extra_data_hashcode(obj);
  for (uint32 i=0 ; i < len ; i++)
    hashcode = 31 * hashcode + hashcode_64(get_int_at_unchecked(obj, i));
  return hashcode_64(hashcode);
}

static uint32 compute_ne_float_seq_hashcode(OBJ obj) {
  uint32 len = read_size_field_unchecked(obj);
  double *elts = (double *) obj.core_data.ptr;
  uint64 hashcode = extra_data_hashcode(obj);
  for (int i=0 ; i < len ; i++)
    hashcode = 31 * hashcode + hashcode_64(bits_cast_double_uint64(elts[i]));
  return hashcode_64(hashcode);
}

static uint32 compute_ne_bool_seq_hashcode(OBJ obj) {
  internal_fail();
}

static uint32 compute_obj_array_hashcode(OBJ *objs, uint32 size, uint64 extra_data_hashcode) {
  uint64 hashcode = extra_data_hashcode;
  for (uint32 i=0 ; i < size ; i++)
    hashcode = 31 * hashcode + hashcode_64(compute_hashcode(objs[i]));
  return hashcode_64(hashcode);
}

static uint32 compute_ne_seq_hashcode(OBJ obj) {
  return compute_obj_array_hashcode((OBJ *) obj.core_data.ptr, read_size_field_unchecked(obj), extra_data_hashcode(obj));
}

////////////////////////////////////////////////////////////////////////////////

static uint64 compute_tree_set_hashcode(TREE_SET_NODE *, uint64 hashcode);

static uint64 compute_set_hashcode(FAT_SET_PTR fat_ptr, uint64 hashcode) {
  if (fat_ptr.size == 0)
    return hashcode;

  if (!fat_ptr.is_array_or_empty)
    return compute_obj_array_hashcode(fat_ptr.ptr.array, fat_ptr.size, hashcode);
  else
    return compute_tree_set_hashcode(fat_ptr.ptr.tree, hashcode);
}

static uint64 compute_tree_set_hashcode(TREE_SET_NODE *ptr, uint64 hashcode) {
  hashcode = compute_set_hashcode(ptr->left, hashcode);
  hashcode = 31 * hashcode + hashcode_64(compute_hashcode(ptr->value));
  return compute_set_hashcode(ptr->right, hashcode);
}

static uint32 compute_ne_set_hashcode(OBJ obj) {
  uint32 size = read_size_field_unchecked(obj);

  uint64 hashcode = extra_data_hashcode(obj);

  if (is_array_set(obj))
    return compute_obj_array_hashcode((OBJ *) obj.core_data.ptr, size, hashcode);

  //## TODO: CHECK THAT THE HASHCODES OF ARRAY-BASED AND TREE-BASED MAPS MATCH

  assert(is_mixed_repr_map(obj));

  MIXED_REPR_SET_OBJ *ptr = get_mixed_repr_set_ptr(obj);
  if (ptr->array_repr != NULL)
    return compute_obj_array_hashcode(ptr->array_repr->buffer, size, hashcode);

  return compute_tree_set_hashcode(ptr->tree_repr, hashcode);
}

////////////////////////////////////////////////////////////////////////////////

static uint64 compute_tree_map_keys_partial_hashcode(TREE_MAP_NODE *, uint64 hashcode);

static uint64 compute_map_keys_partial_hashcode(FAT_MAP_PTR fat_ptr, uint64 hashcode) {
  if (fat_ptr.size == 0)
    return hashcode;

  if (fat_ptr.offset == 0)
    return compute_tree_map_keys_partial_hashcode(fat_ptr.ptr.tree, hashcode);

  OBJ *keys = fat_ptr.ptr.array;
  for (uint32 i=0 ; i < fat_ptr.size ; i++)
    hashcode = 31 * hashcode + hashcode_64(compute_hashcode(keys[i]));
  return hashcode;
}

static uint64 compute_tree_map_keys_partial_hashcode(TREE_MAP_NODE *ptr, uint64 hashcode) {
  hashcode = compute_map_keys_partial_hashcode(ptr->left, hashcode);
  hashcode = 31 * hashcode + hashcode_64(compute_hashcode(ptr->key));
  return compute_map_keys_partial_hashcode(ptr->right, hashcode);
}

static uint64 compute_tree_map_values_partial_hashcode(TREE_MAP_NODE *, uint64 hashcode);

static uint64 compute_map_values_partial_hashcode(FAT_MAP_PTR fat_ptr, uint64 hashcode) {
  if (fat_ptr.size == 0)
    return hashcode;

  if (fat_ptr.offset == 0)
    return compute_tree_map_values_partial_hashcode(fat_ptr.ptr.tree, hashcode);

  OBJ *values = fat_ptr.ptr.array + fat_ptr.offset;
  for (uint32 i=0 ; i < fat_ptr.size ; i++)
    hashcode = 31 * hashcode + hashcode_64(compute_hashcode(values[i]));
  return hashcode;
}

static uint64 compute_tree_map_values_partial_hashcode(TREE_MAP_NODE *ptr, uint64 hashcode) {
  hashcode = compute_map_keys_partial_hashcode(ptr->left, hashcode);
  hashcode = 31 * hashcode + hashcode_64(compute_hashcode(ptr->key));
  return compute_map_keys_partial_hashcode(ptr->right, hashcode);
}

static uint32 compute_ne_map_hashcode(OBJ obj) {
  uint32 size = read_size_field_unchecked(obj);

  uint64 hashcode = extra_data_hashcode(obj);

  if (is_array_map(obj))
    return compute_obj_array_hashcode(((BIN_REL_OBJ *) obj.core_data.ptr)->buffer, 2ULL * size, hashcode);

  //## TODO: CHECK THAT THE HASHCODES OF ARRAY-BASED AND TREE-BASED MAPS MATCH

  assert(is_mixed_repr_map(obj));

  MIXED_REPR_MAP_OBJ *ptr = get_mixed_repr_map_ptr(obj);
  if (ptr->array_repr != NULL)
    return compute_obj_array_hashcode(ptr->array_repr->buffer, 2ULL * size, hashcode);

  hashcode = compute_tree_map_keys_partial_hashcode(ptr->tree_repr, hashcode);
  hashcode = compute_tree_map_values_partial_hashcode(ptr->tree_repr, hashcode);
  return hashcode_64(hashcode);
}

////////////////////////////////////////////////////////////////////////////////

static uint32 compute_ne_bin_rel_hashcode(OBJ obj) {
  return compute_obj_array_hashcode(((BIN_REL_OBJ *) obj.core_data.ptr)->buffer, 2ULL * read_size_field_unchecked(obj), extra_data_hashcode(obj));
}

static uint32 compute_ne_tern_rel_hashcode(OBJ obj) {
  return compute_obj_array_hashcode(((TERN_REL_OBJ *) obj.core_data.ptr)->buffer, 3ULL * read_size_field_unchecked(obj), extra_data_hashcode(obj));
}

static uint32 compute_ad_hoc_tag_rec_hashcode(OBJ obj) {
  return opt_repr_hashcode(obj.core_data.ptr, get_opt_repr_id(obj));
}

static uint32 compute_boxed_obj_hashcode(OBJ obj) {
  return 31 * hashcode_64(obj.extra_data) + compute_hashcode(get_boxed_obj_ptr(obj)->obj);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint32 compute_hashcode(OBJ obj) {
  if (is_inline_obj(obj)) {
    assert(get_phys_repr_info(obj) == 0);
    //## FOR THIS TO WORK I ALSO NEED TO MAKE SURE THAT INLINE REPRESENTATIONS ARE UNIQUE AND THAT
    //## INLINE AND NON-INLINE ONES DO NOT OVERLAP. ADD THE REQUIRED CHECKS IN mem-utils.h IF NEED BE
    return hashcode_64_64(obj.core_data.int_, obj.extra_data);
  }

  switch (get_obj_type(obj)) {
    case TYPE_NE_INT_SEQ:
      return compute_ne_int_seq_hashcode(obj);

    case TYPE_NE_FLOAT_SEQ:
      return compute_ne_float_seq_hashcode(obj);

    case TYPE_NE_BOOL_SEQ:
      return compute_ne_bool_seq_hashcode(obj);

    case TYPE_NE_SEQ:
      return compute_ne_seq_hashcode(obj);

    case TYPE_NE_SET:
      return compute_ne_set_hashcode(obj);

    case TYPE_NE_MAP:
      return compute_ne_map_hashcode(obj);

    case TYPE_NE_BIN_REL:
      return compute_ne_bin_rel_hashcode(obj);

    case TYPE_NE_TERN_REL:
      return compute_ne_tern_rel_hashcode(obj);

    case TYPE_AD_HOC_TAG_REC:
      return compute_ad_hoc_tag_rec_hashcode(obj);

    case TYPE_BOXED_OBJ:
      return compute_boxed_obj_hashcode(obj);

    default:
      internal_fail();
  }
}

// const uint32 BASE_VALUE   = 17;
// const uint32 MULTIPLIER   = 37;

// const uint32 MULT_BASE_VALUE = BASE_VALUE * MULTIPLIER;

// ////////////////////////////////////////////////////////////////////////////////

// uint32 combined_hash_code(uint32 start_value, OBJ *array, uint32 count) {
//   uint32 hash_code = start_value;
//   for (uint32 i=0 ; i < count ; i++)
//     hash_code = MULTIPLIER * hash_code + compute_hash_code(array[i]);
//   return hash_code;
// }

// uint32 compute_hash_code(OBJ obj) {
//   if (is_tag_obj(obj))
//     return MULTIPLIER * (MULT_BASE_VALUE + get_tag_idx(obj)) + compute_hash_code(get_inner_obj(obj));

//   switch (get_physical_type(obj)) {
//     case TYPE_BLANK_OBJ:
//     case TYPE_NULL_OBJ:
//       fail();

//     case TYPE_SYMBOL:
//       return MULT_BASE_VALUE + get_symb_idx(obj);

//     case TYPE_INTEGER:
//     case TYPE_FLOAT: {
//       uint64 core_data = obj.core_data.int_;
//       return MULT_BASE_VALUE + (uint32) (core_data ^ (core_data >> 32));
//     }

//     case TYPE_SEQUENCE: {
//       uint32 size = get_seq_length(obj);
//       return combined_hash_code(MULT_BASE_VALUE + size, size > 0 ? get_seq_buffer_ptr(obj) : NULL, size);
//     }

//     case TYPE_SET: {
//       if (is_empty_rel(obj))
//         return MULT_BASE_VALUE;
//       SET_OBJ *ptr = get_set_ptr(obj);
//       uint32 size = ptr->size;
//       return combined_hash_code(MULT_BASE_VALUE + size, ptr->buffer, size);
//     }

//     case TYPE_BIN_REL: {
//       BIN_REL_OBJ *ptr = get_bin_rel_ptr(obj);
//       uint32 size = ptr->size;
//       return combined_hash_code(MULT_BASE_VALUE + size, ptr->buffer, 2 * size);
//     }

//     case TYPE_TERN_REL: {
//       TERN_REL_OBJ *ptr = get_tern_rel_ptr(obj);
//       uint32 size = ptr->size;
//       return combined_hash_code(MULT_BASE_VALUE + size, ptr->buffer, 3 * size);
//     }

//     case TYPE_TAG_OBJ:
//       fail();

//     case TYPE_SLICE: {
//       uint32 size = get_seq_length(obj);
//       return combined_hash_code(MULT_BASE_VALUE + size, get_seq_buffer_ptr(obj), size);
//     }

//     case TYPE_MAP:
//     case TYPE_LOG_MAP: {
//       BIN_REL_OBJ *ptr = get_bin_rel_ptr(obj);
//       uint32 size = ptr->size;
//       return combined_hash_code(MULT_BASE_VALUE + size, ptr->buffer, 2 * size);
//     }
//   }
//   fail();
// }
