#include "lib.h"


inline int sign(int value) {
  if (value < 0)
    return -1;
  else if (value > 0)
    return 1;
  else
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

//## REMOVE
int comp_floats(double x, double y) {
  uint64 n = *((uint64 *) &x);
  uint64 m = *((uint64 *) &y);

  if (n < m)
    return 1;

  if (n > m)
    return -1;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline int intrl_cmp_obj_arrays(OBJ *elts1, OBJ *elts2, uint32 count) {
  for (int i=0 ; i < count ; i++) {
    int cr = intrl_cmp(elts1[i], elts2[i]);
    if (cr != 0)
      return cr;
  }
  return 0;
}

__attribute__ ((noinline)) int intrl_cmp_ne_obj_seqs(OBJ obj1, OBJ obj2) {
  OBJ *elts1 = (OBJ *) obj1.core_data.ptr;
  OBJ *elts2 = (OBJ *) obj2.core_data.ptr;
  uint32 count = read_size_field_unchecked(obj1);

  return intrl_cmp_obj_arrays(elts1, elts2, count);
}

__attribute__ ((noinline)) int intrl_cmp_ne_sets(OBJ obj1, OBJ obj2) {
  assert(read_size_field_unchecked(obj1) == read_size_field_unchecked(obj2));

  uint32 size = read_size_field_unchecked(obj1);
  OBJ *elts1, *elts2;

  if (is_array_set(obj1)) {
    elts1 = (OBJ *) obj1.core_data.ptr;
  }
  else {
    MIXED_REPR_SET_OBJ *ptr = (MIXED_REPR_SET_OBJ *) obj1.core_data.ptr;
    if (ptr->array_repr == NULL)
      rearrange_set_as_array(ptr, size);
    elts1 = ptr->array_repr->buffer;
  }

  if (is_array_set(obj2)) {
    elts2 = (OBJ *) obj2.core_data.ptr;
  }
  else {
    MIXED_REPR_SET_OBJ *ptr = (MIXED_REPR_SET_OBJ *) obj2.core_data.ptr;
    if (ptr->array_repr == NULL)
      rearrange_set_as_array(ptr, size);
    OBJ *elts2 = ptr->array_repr->buffer;
  }

  return intrl_cmp_obj_arrays(elts1, elts2, size);
}

__attribute__ ((noinline)) int intrl_cmp_obj_bin_rels(OBJ obj1, OBJ obj2) {
  OBJ *args1 = ((BIN_REL_OBJ *) obj1.core_data.ptr)->buffer;
  OBJ *args2 = ((BIN_REL_OBJ *) obj2.core_data.ptr)->buffer;
  uint64 count = 2ULL * read_size_field_unchecked(obj1);

  for (uint64 i=0 ; i < count ; i++) {
    int cr = intrl_cmp(args1[i], args2[i]);
    if (cr != 0)
      return cr;
  }
  return 0;
}

__attribute__ ((noinline)) int intrl_cmp_obj_tern_rels(OBJ obj1, OBJ obj2) {
  OBJ *args1 = ((BIN_REL_OBJ *) obj1.core_data.ptr)->buffer;
  OBJ *args2 = ((BIN_REL_OBJ *) obj2.core_data.ptr)->buffer;
  uint64 count = 3ULL * read_size_field_unchecked(obj1);

  for (uint64 i=0 ; i < count ; i++) {
    int cr = intrl_cmp(args1[i], args2[i]);
    if (cr != 0)
      return cr;
  }
  return 0;
}

__attribute__ ((noinline)) int intrl_cmp_ne_int_seqs(OBJ obj1, OBJ obj2) {
  assert(read_size_field_unchecked(obj1) == read_size_field_unchecked(obj2));

  uint32 len = read_size_field_unchecked(obj1);

  for (uint32 i=0 ; i < len ; i++) {
    int64 elt1 = get_int_at_unchecked(obj1, i);
    int64 elt2 = get_int_at_unchecked(obj2, i);

    if (elt1 != elt2)
      return elt1 < elt2 ? 1 : -1;
  }

  return 0;
}

__attribute__ ((noinline)) int intrl_cmp_ne_float_seqs(OBJ obj1, OBJ obj2) {
  assert(read_size_field_unchecked(obj1) == read_size_field_unchecked(obj2));

  int len = read_size_field_unchecked(obj1);
  double *elts1 = (double *) obj1.core_data.ptr;
  double *elts2 = (double *) obj2.core_data.ptr;

  for (int i=0 ; i < len ; i++) {
    double elt1 = elts1[i];
    double elt2 = elts2[i];
    if (elt1 != elt2)
      return elt1 < elt2 ? 1 : -1;
  }

  return 0;
}

__attribute__ ((noinline)) int intrl_cmp_ne_maps(OBJ obj1, OBJ obj2) {
  assert(read_size_field_unchecked(obj1) == read_size_field_unchecked(obj2));

  uint32 size = read_size_field_unchecked(obj1);
  OBJ *args1, *args2;

  if (is_array_map(obj1)) {
    args1 = (OBJ *) obj1.core_data.ptr;
  }
  else {
    MIXED_REPR_MAP_OBJ *ptr = (MIXED_REPR_MAP_OBJ *) obj1.core_data.ptr;
    if (ptr->array_repr == NULL)
      rearrange_map_as_array(ptr, size);
    args1 = ptr->array_repr->buffer;
  }

  if (is_array_map(obj2)) {
    args2 = (OBJ *) obj2.core_data.ptr;
  }
  else {
    MIXED_REPR_MAP_OBJ *ptr = (MIXED_REPR_MAP_OBJ *) obj2.core_data.ptr;
    if (ptr->array_repr == NULL)
      rearrange_map_as_array(ptr, size);
    OBJ *args2 = ptr->array_repr->buffer;
  }

  return intrl_cmp_obj_arrays(args1, args2, 2ULL * size);
}

int intrl_cmp_opt_reprs(OBJ obj1, OBJ obj2) {
  return opt_repr_cmp(obj1.core_data.ptr, obj2.core_data.ptr, get_opt_repr_id(obj1));
}

int intrl_cmp_boxed_objs(OBJ obj1, OBJ obj2) {
  return intrl_cmp(((BOXED_OBJ *) obj1.core_data.ptr)->obj, ((BOXED_OBJ *) obj2.core_data.ptr)->obj);
}

int (*intrl_cmp_disp_table[])(OBJ, OBJ) = {
  intrl_cmp_ne_int_seqs,    // TYPE_NE_INT_SEQ
  intrl_cmp_ne_float_seqs,  // TYPE_NE_FLOAT_SEQ
  NULL,                     // TYPE_NE_BOOL_SEQ
  intrl_cmp_ne_obj_seqs,    // TYPE_NE_SEQ
  intrl_cmp_ne_sets,        // TYPE_NE_SET
  intrl_cmp_ne_maps,        // TYPE_NE_MAP
  intrl_cmp_obj_bin_rels,   // TYPE_NE_BIN_REL
  intrl_cmp_obj_tern_rels,  // TYPE_NE_TERN_REL
  intrl_cmp_opt_reprs,      // TYPE_AD_HOC_TAG_REC
  intrl_cmp_boxed_objs      // TYPE_BOXED_OBJ
};

// __attribute__ ((noinline)) int intrl_cmp_non_inline(OBJ_TYPE type, OBJ obj1, OBJ obj2) {
//   switch (type) {
//     case TYPE_NE_INT_SEQ:
//       return intrl_cmp_ne_int_seqs(obj1, obj2);

//     case TYPE_NE_FLOAT_SEQ:
//       return intrl_cmp_ne_float_seqs(obj1, obj2);

//     case TYPE_NE_BOOL_SEQ:
//       internal_fail();
//       // return intrl_cmp_NE_BOOL_SEQ();

//     case TYPE_NE_SEQ:
//       return intrl_cmp_obj_arrays((OBJ *) obj1.core_data.ptr, (OBJ *) obj2.core_data.ptr, read_size_field_unchecked(obj1));

//     case TYPE_NE_SET:
//       return intrl_cmp_obj_arrays((OBJ *) obj1.core_data.ptr, (OBJ *) obj2.core_data.ptr, read_size_field_unchecked(obj1));

//     case TYPE_NE_MAP:
//       return intrl_cmp_ne_maps(obj1, obj2);

//     case TYPE_NE_BIN_REL:
//       return intrl_cmp_obj_arrays(((BIN_REL_OBJ *) obj1.core_data.ptr)->buffer, ((BIN_REL_OBJ *) obj2.core_data.ptr)->buffer, 2 * read_size_field_unchecked(obj1));

//     case TYPE_NE_TERN_REL:
//       return intrl_cmp_obj_arrays(((TERN_REL_OBJ *) obj1.core_data.ptr)->buffer, ((TERN_REL_OBJ *) obj2.core_data.ptr)->buffer, 3 * read_size_field_unchecked(obj1));

//     case TYPE_AD_HOC_TAG_REC: {
//       int rc = opt_repr_cmp(obj1.core_data.ptr, obj2.core_data.ptr, get_opt_repr_id(obj1));
//       assert(rc >= -1 & rc <= 1);
//       return rc;
//     }

//     case TYPE_BOXED_OBJ:
//       return intrl_cmp(((BOXED_OBJ *) obj1.core_data.ptr)->obj, ((BOXED_OBJ *) obj2.core_data.ptr)->obj);

//     default:
//       internal_fail();
//   }
// }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// int slow_cmp_objs(OBJ obj1, OBJ obj2);


// __attribute__ ((noinline)) int slow_cmp_same_type_opt_recs(void *ptr1, void *ptr2, uint16 repr_id, uint32 size) {
//   uint32 count;
//   uint16 *labels = opt_repr_get_fields(repr_id, count);
//   assert(count == size);

//   for (int i=0 ; i < size ; i++) {
//     uint16 label = labels[i];
//     OBJ value1 = opt_repr_lookup_field(ptr1, repr_id, label);
//     OBJ value2 = opt_repr_lookup_field(ptr2, repr_id, label);

//     int res = slow_cmp_objs(value1, value2);
//     if (res != 0)
//       return res;
//   }

//   return 0;
// }


// int slow_cmp_bin_rels_slow(OBJ rel1, OBJ rel2) {
//   assert(get_size(rel1) == get_size(rel2));

//   BIN_REL_ITER key_it1, key_it2, value_it1, value_it2;

//   get_bin_rel_iter(key_it1, rel1);
//   get_bin_rel_iter(key_it2, rel2);

//   value_it1 = key_it1;
//   value_it2 = key_it2;

//   while (!is_out_of_range(key_it1)) {
//     assert(!is_out_of_range(key_it2));

//     OBJ left_arg_1 = get_curr_left_arg(key_it1);
//     OBJ left_arg_2 = get_curr_left_arg(key_it2);

//     int res = slow_cmp_objs(left_arg_1, left_arg_2);
//     if (res != 0)
//       return res;

//     move_forward(key_it1);
//     move_forward(key_it2);
//   }

//   assert(is_out_of_range(key_it2));

//   while (!is_out_of_range(value_it1)) {
//     assert(!is_out_of_range(value_it2));

//     OBJ right_arg_1 = get_curr_right_arg(value_it1);
//     OBJ right_arg_2 = get_curr_right_arg(value_it2);

//     int res = slow_cmp_objs(right_arg_1, right_arg_2);
//     if (res != 0)
//       return res;

//     move_forward(value_it1);
//     move_forward(value_it2);
//   }

//   return 0;
// }

// ////////////////////////////////////////////////////////////////////////////////

// // Returns:   > 0     if obj1 < obj2
// //              0     if obj1 = obj2
// //            < 0     if obj1 > obj2

// int slow_cmp_objs(OBJ obj1, OBJ obj2) {
//   if (are_shallow_eq(obj1, obj2))
//     return 0;

//   bool is_always_inline_1 = is_always_inline(obj1);
//   bool is_always_inline_2 = is_always_inline(obj2);

//   if (is_always_inline_1)
//     if (is_always_inline_2)
//       return shallow_cmp(obj1, obj2);
//     else
//       return 1;
//   else if (is_always_inline_2)
//     return -1;

//   OBJ_TYPE type1 = get_logical_type(obj1);
//   OBJ_TYPE type2 = get_logical_type(obj2);

//   if (type1 != type2)
//     return type2 - type1;

//   uint32 count = 0;
//   OBJ *elems1 = 0;
//   OBJ *elems2 = 0;

//   switch (type1) {
//     case TYPE_NE_SEQ: {
//       uint32 len1 = get_seq_length(obj1);
//       uint32 len2 = get_seq_length(obj2);
//       if (len1 != len2)
//         return len2 - len1; //## BUG BUG BUG
//       count = len1;
//       elems1 = get_seq_elts_ptr(obj1);
//       elems2 = get_seq_elts_ptr(obj2);
//       break;
//     }

//     case TYPE_NE_SET: {
//       uint32 size1 = get_rel_size(obj1);
//       uint32 size2 = get_rel_size(obj2);
//       if (size1 != size2)
//         return size2 - size1; //## BUG BUG BUG
//       count = size1;
//       elems1 = get_set_elts_ptr(obj1);
//       elems2 = get_set_elts_ptr(obj2);
//       break;
//     }

//     case TYPE_NE_BIN_REL: {
//       if (is_opt_rec(obj1)) {
//         void *ptr1 = get_opt_repr_ptr(obj1);
//         uint16 repr_id_1 = get_opt_repr_id(obj1);
//         uint32 size1 = opt_repr_get_fields_count(ptr1, repr_id_1);

//         if (is_opt_rec(obj2)) {
//           void *ptr2 = get_opt_repr_ptr(obj2);
//           uint16 repr_id_2 = get_opt_repr_id(obj2);
//           uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);
//           if (size1 != size2)
//             return size2 - size1; //## BUG BUG BUG
//           else if (repr_id_1 == repr_id_2 && !opt_repr_may_have_opt_fields(repr_id_1))
//             return slow_cmp_same_type_opt_recs(ptr1, ptr2, repr_id_1, size1);
//           else
//             // return cmp_opt_recs(ptr1, repr_id_1, ptr2, repr_id_2, size1);
//             return slow_cmp_bin_rels_slow(obj1, obj2);
//         }
//         else {
//           BIN_REL_OBJ *rel2 = get_bin_rel_ptr(obj2);
//           uint32 size2 = get_rel_size(obj2);
//           if (size1 != size2)
//             return size2 - size1; //## BUG BUG BUG
//           else
//             // return cmp_opt_rec_bin_rel(ptr1, repr_id_1, rel2);
//             return slow_cmp_bin_rels_slow(obj1, obj2);
//         }
//       }
//       else if (is_opt_rec(obj2)) {
//         BIN_REL_OBJ *rel1 = get_bin_rel_ptr(obj1);
//         uint32 size1 = get_rel_size(obj1);

//         void *ptr2 = get_opt_repr_ptr(obj2);
//         uint16 repr_id_2 = get_opt_repr_id(obj2);
//         uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);

//         if (size1 != size2)
//           return size2 - size1; //## BUG BUG BUG
//         else
//           // return -cmp_opt_rec_bin_rel(ptr2, repr_id_2, rel1);
//           return slow_cmp_bin_rels_slow(obj1, obj2);
//       }

//       BIN_REL_OBJ *rel1 = get_bin_rel_ptr(obj1);
//       BIN_REL_OBJ *rel2 = get_bin_rel_ptr(obj2);
//       uint32 size1 = get_rel_size(obj1);
//       uint32 size2 = get_rel_size(obj2);
//       if (size1 != size2)
//         return size2 - size1; //## BUG BUG BUG
//       count = 2 * size1;
//       elems1 = rel1->buffer;
//       elems2 = rel2->buffer;
//       break;
//     }

//     case TYPE_NE_TERN_REL: {
//       TERN_REL_OBJ *rel1 = get_tern_rel_ptr(obj1);
//       TERN_REL_OBJ *rel2 = get_tern_rel_ptr(obj2);
//       uint32 size1 = get_rel_size(obj1);
//       uint32 size2 = get_rel_size(obj2);
//       if (size1 != size2)
//         return size2 - size1; //## BUG BUG BUG
//       count = 3 * size1;
//       elems1 = rel1->buffer;
//       elems2 = rel2->buffer;
//       break;
//     }

//     case TYPE_TAG_OBJ: {
//       uint16 tag_id_1 = get_tag_id(obj1);
//       uint16 tag_id_2 = get_tag_id(obj2);
//       if (tag_id_1 != tag_id_2)
//         return tag_id_2 - tag_id_1;
//       return slow_cmp_objs(get_inner_obj(obj1), get_inner_obj(obj2));
//     }

//     default:
//       internal_fail();
//   }

//   for (uint32 i=0 ; i < count ; i++) {
//     int cr = slow_cmp_objs(elems1[i], elems2[i]);
//     if (cr != 0)
//       return cr;
//   }

//   return 0;
// }
