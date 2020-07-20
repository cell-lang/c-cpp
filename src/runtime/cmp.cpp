#include "lib.h"
#include "extern.h"


uint32 get_tags_count(OBJ obj);
uint32 read_size_field(OBJ seq);
uint16 get_inline_tag_id(OBJ obj);
uint16 get_nested_inline_tag_id(OBJ obj);
OBJ untag_opt_tag_rec(OBJ obj);
OBJ clear_both_inline_tags(OBJ obj);
OBJ clear_all_inline_tags(OBJ obj);
OBJ_TYPE physical_to_logical_type(OBJ_TYPE type);


int fast_cmp_objs(OBJ obj1, OBJ obj2);


/* __attribute__ ((noinline)) */ int fast_cmp_bin_rels_slow(OBJ rel1, OBJ rel2) {
  assert(get_size(rel1) == get_size(rel2));

  BIN_REL_ITER key_it1, key_it2, value_it1, value_it2;

  get_bin_rel_iter(key_it1, rel1);
  get_bin_rel_iter(key_it2, rel2);

  value_it1 = key_it1;
  value_it2 = key_it2;

  while (!is_out_of_range(key_it1)) {
    assert(!is_out_of_range(key_it2));

    OBJ left_arg_1 = get_curr_left_arg(key_it1);
    OBJ left_arg_2 = get_curr_left_arg(key_it2);

    int res = fast_cmp_objs(left_arg_1, left_arg_2);
    if (res != 0)
      return res;

    move_forward(key_it1);
    move_forward(key_it2);
  }

  assert(is_out_of_range(key_it2));

  while (!is_out_of_range(value_it1)) {
    assert(!is_out_of_range(value_it2));

    OBJ right_arg_1 = get_curr_right_arg(value_it1);
    OBJ right_arg_2 = get_curr_right_arg(value_it2);

    int res = fast_cmp_objs(right_arg_1, right_arg_2);
    if (res != 0)
      return res;

    move_forward(value_it1);
    move_forward(value_it2);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OBJ get_obj_at_hack(OBJ seq, int64 idx) {
  OBJ_TYPE type = get_physical_type(seq);

  if (type == TYPE_NE_SEQ | type == TYPE_NE_SLICE) {
    OBJ *elts = (OBJ *) seq.core_data.ptr;
    return elts[idx];
  }

  switch (type) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts = (uint8 *) seq.core_data.ptr;
      return make_int(elts[idx]);
    }

    case TYPE_NE_SEQ_UINT8_INLINE:
      return make_int(inline_uint8_at(seq.core_data.int_, idx));

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts = (int16 *) seq.core_data.ptr;
      return make_int(elts[idx]);
    }

    case TYPE_NE_SEQ_INT16_INLINE:
      return make_int(inline_int16_at(seq.core_data.int_, idx));

    default:
      internal_fail();
  }
}

__attribute__ ((noinline)) int fast_cmp_seqs_ignoring_inline_tags_slow(OBJ seq1, OBJ seq2) {
  assert(read_size_field(seq1) == read_size_field(seq2));

  uint32 len = read_size_field(seq1);
  for (int i=0 ; i < len ; i++) {
    int cr = fast_cmp_objs(get_obj_at_hack(seq1, i), get_obj_at_hack(seq2, i));
    if (cr != 0)
      return cr;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) int fast_cmp_seqs_ignoring_inline_tags_ne_seq_or_slice_uint8(OBJ seq1, uint32 len, OBJ seq2, OBJ_TYPE phys_type_2) {
  uint8 *elts1_u8 = (uint8 *) seq1.core_data.ptr;

  switch (phys_type_2) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts2_u8 = (uint8 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_u8[i];
        int elt2 = elts2_u8[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_u8[i];
        int elt2 = inline_uint8_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts2_i16 = (int16 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_u8[i];
        int elt2 = elts2_i16[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_u8[i];
        int elt2 = inline_int16_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    default:
      return fast_cmp_seqs_ignoring_inline_tags_slow(seq1, seq2);
  }
}

__attribute__ ((noinline)) int fast_cmp_seqs_ignoring_inline_tags_ne_seq_uint8_inline(OBJ seq1, uint32 len, OBJ seq2, OBJ_TYPE phys_type_2) {
  switch (phys_type_2) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts2_u8 = (uint8 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_uint8_at(seq1.core_data.int_, i);
        int elt2 = elts2_u8[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_uint8_at(seq1.core_data.int_, i);
        int elt2 = inline_uint8_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts2_i16 = (int16 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_int16_at(seq1.core_data.int_, i);
        int elt2 = elts2_i16[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_int16_at(seq1.core_data.int_, i);
        int elt2 = inline_int16_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    default:
      return fast_cmp_seqs_ignoring_inline_tags_slow(seq1, seq2);
  }
}

__attribute__ ((noinline)) int fast_cmp_seqs_ignoring_inline_tags_ne_seq_or_slice_int16(OBJ seq1, uint32 len, OBJ seq2, OBJ_TYPE phys_type_2) {
  int16 *elts1_i16 = (int16 *) seq1.core_data.ptr;

  switch (phys_type_2) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts2_u8 = (uint8 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_i16[i];
        int elt2 = elts2_u8[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_i16[i];
        int elt2 = inline_uint8_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts2_i16 = (int16 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_i16[i];
        int elt2 = elts2_i16[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = elts1_i16[i];
        int elt2 = inline_int16_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    default:
      return fast_cmp_seqs_ignoring_inline_tags_slow(seq1, seq2);
  }
}

__attribute__ ((noinline)) int fast_cmp_seqs_ignoring_inline_tags_ne_seq_int16_inline(OBJ seq1, uint32 len, OBJ seq2, OBJ_TYPE phys_type_2) {
  switch (phys_type_2) {
    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8: {
      uint8 *elts2_u8 = (uint8 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_int16_at(seq1.core_data.int_, i);
        int elt2 = elts2_u8[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_UINT8_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_int16_at(seq1.core_data.int_, i);
        int elt2 = inline_uint8_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16: {
      int16 *elts2_i16 = (int16 *) seq2.core_data.ptr;
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_int16_at(seq1.core_data.int_, i);
        int elt2 = elts2_i16[i];
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    case TYPE_NE_SEQ_INT16_INLINE: {
      for (int i=0 ; i < len ; i++) {
        int elt1 = inline_int16_at(seq1.core_data.int_, i);
        int elt2 = inline_int16_at(seq2.core_data.int_, i);
        if (elt1 != elt2)
          return elt2 - elt1;
      }
      return 0;
    }

    default:
      return fast_cmp_seqs_ignoring_inline_tags_slow(seq1, seq2);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) int fast_cmp_seqs_ignoring_inline_tags(OBJ seq1, OBJ_TYPE phys_type_1, OBJ seq2, OBJ_TYPE phys_type_2) {
  assert(
    phys_type_1 == TYPE_NE_SEQ |
    phys_type_1 == TYPE_NE_SLICE |
    phys_type_1 == TYPE_NE_SEQ_UINT8 |
    phys_type_1 == TYPE_NE_SLICE_UINT8 |
    phys_type_1 == TYPE_NE_SEQ_UINT8_INLINE |
    phys_type_1 == TYPE_NE_SEQ_INT16 |
    phys_type_1 == TYPE_NE_SLICE_INT16 |
    phys_type_1 == TYPE_NE_SEQ_INT16_INLINE
  );
  assert(
    phys_type_2 == TYPE_NE_SEQ |
    phys_type_2 == TYPE_NE_SLICE |
    phys_type_2 == TYPE_NE_SEQ_UINT8 |
    phys_type_2 == TYPE_NE_SLICE_UINT8 |
    phys_type_2 == TYPE_NE_SEQ_UINT8_INLINE |
    phys_type_2 == TYPE_NE_SEQ_INT16 |
    phys_type_2 == TYPE_NE_SLICE_INT16 |
    phys_type_2 == TYPE_NE_SEQ_INT16_INLINE
  );

  int64 len1 = read_size_field(seq1);
  int64 len2 = read_size_field(seq2);

  if (len1 != len2)
    return len2 - len1;

  if ((phys_type_1 == TYPE_NE_SEQ | phys_type_1 == TYPE_NE_SLICE) & (phys_type_2 == TYPE_NE_SEQ | phys_type_2 == TYPE_NE_SLICE)) {
    OBJ *elts1 = (OBJ *) seq1.core_data.ptr;
    OBJ *elts2 = (OBJ *) seq2.core_data.ptr;
    for (int i=0 ; i < len1 ; i++) {
      int cr = fast_cmp_objs(elts1[i], elts2[i]);
      if (cr != 0)
        return cr;
    }
    return 0;
  }

  switch (phys_type_1) {
    case TYPE_NE_SEQ:
    case TYPE_NE_SLICE:
      return fast_cmp_seqs_ignoring_inline_tags_slow(seq1, seq2);

    case TYPE_NE_SEQ_UINT8:
    case TYPE_NE_SLICE_UINT8:
      return fast_cmp_seqs_ignoring_inline_tags_ne_seq_or_slice_uint8(seq1, len1, seq2, phys_type_2);

    case TYPE_NE_SEQ_UINT8_INLINE:
      return fast_cmp_seqs_ignoring_inline_tags_ne_seq_uint8_inline(seq1, len1, seq2, phys_type_2);

    case TYPE_NE_SEQ_INT16:
    case TYPE_NE_SLICE_INT16:
      return fast_cmp_seqs_ignoring_inline_tags_ne_seq_or_slice_int16(seq1, len1, seq2, phys_type_2);

    case TYPE_NE_SEQ_INT16_INLINE:
      return fast_cmp_seqs_ignoring_inline_tags_ne_seq_int16_inline(seq1, len1, seq2, phys_type_2);

    default:
      internal_fail();
  }
}

/* __attribute__ ((noinline)) */ int fast_cmp_objs_ignoring_inline_tags(OBJ obj1, OBJ_TYPE phys_type_1, OBJ obj2, OBJ_TYPE phys_type_2) {
  OBJ_TYPE log_type_1 = physical_to_logical_type(phys_type_1);
  OBJ_TYPE log_type_2 = physical_to_logical_type(phys_type_2);

  if (log_type_1 != log_type_2)
    return log_type_2 - log_type_1;

  uint32 count;
  OBJ *elts1;
  OBJ *elts2;

  switch (log_type_1) {
    case TYPE_NE_SEQ:
      return fast_cmp_seqs_ignoring_inline_tags(obj1, phys_type_1, obj2, phys_type_2);

    case TYPE_NE_SET: {
      assert(phys_type_1 == TYPE_NE_SET);
      assert(phys_type_2 == TYPE_NE_SET);

      int size1 = read_size_field(obj1);
      int size2 = read_size_field(obj2);

      if (size1 != size2)
        return size2 - size1; //## BUG

      SET_OBJ *ptr1 = (SET_OBJ *) get_ref_obj_ptr(obj1);
      SET_OBJ *ptr2 = (SET_OBJ *) get_ref_obj_ptr(obj2);

      count = size1;
      elts1 = ptr1->buffer;
      elts2 = ptr2->buffer;

      break;
    }

    case TYPE_NE_BIN_REL: {
      assert(phys_type_1 == TYPE_NE_BIN_REL | phys_type_1 == TYPE_NE_MAP | phys_type_1 == TYPE_NE_LOG_MAP | phys_type_1 == TYPE_OPT_REC);
      assert(phys_type_2 == TYPE_NE_BIN_REL | phys_type_2 == TYPE_NE_MAP | phys_type_2 == TYPE_NE_LOG_MAP | phys_type_2 == TYPE_OPT_REC);

      if (phys_type_1 == TYPE_OPT_REC) {
        void *ptr1 = get_opt_repr_ptr(obj1);
        uint16 repr_id_1 = get_opt_repr_id(obj1);
        uint32 size1 = opt_repr_get_fields_count(ptr1, repr_id_1);

        if (phys_type_2 == TYPE_OPT_REC) {
          void *ptr2 = get_opt_repr_ptr(obj2);
          uint16 repr_id_2 = get_opt_repr_id(obj2);
          uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);
          if (size1 != size2)
            return size2 - size1; //## BUG
          else if (repr_id_1 == repr_id_2)
            return opt_repr_cmp(ptr1, ptr2, repr_id_1);
          else
            // return cmp_opt_recs(ptr1, repr_id_1, ptr2, repr_id_2, size1);
            return fast_cmp_bin_rels_slow(obj1, obj2);
        }
        else {
          BIN_REL_OBJ *rel2 = get_bin_rel_ptr(obj2);
          uint32 size2 = read_size_field(obj2);
          if (size1 != size2)
            return size2 - size1; //## BUG
          else
            // return cmp_opt_rec_bin_rel(ptr1, repr_id_1, rel2);
            return fast_cmp_bin_rels_slow(obj1, obj2);
        }
      }
      else if (phys_type_2 == TYPE_OPT_REC) {
        BIN_REL_OBJ *rel1 = get_bin_rel_ptr(obj1);
        uint32 size1 = read_size_field(obj1);

        void *ptr2 = get_opt_repr_ptr(obj2);
        uint16 repr_id_2 = get_opt_repr_id(obj2);
        uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);

        if (size1 != size2)
          return size2 - size1; //## BUG BUG BUG
        else
          // return -cmp_opt_rec_bin_rel(ptr2, repr_id_2, rel1);
          return fast_cmp_bin_rels_slow(obj1, obj2);
      }

      BIN_REL_OBJ *rel1 = get_bin_rel_ptr(obj1);
      BIN_REL_OBJ *rel2 = get_bin_rel_ptr(obj2);

      uint32 size1 = read_size_field(obj1);
      uint32 size2 = read_size_field(obj2);

      if (size1 != size2)
        return size2 - size1; //## BUG

      count = 2 * size1;
      elts1 = rel1->buffer;
      elts2 = rel2->buffer;

      break;
    }

    case TYPE_NE_TERN_REL: {
      assert(phys_type_1 == TYPE_NE_TERN_REL);
      assert(phys_type_2 == TYPE_NE_TERN_REL);

      TERN_REL_OBJ *rel1 = (TERN_REL_OBJ *) obj1.core_data.ptr;
      TERN_REL_OBJ *rel2 = (TERN_REL_OBJ *) obj2.core_data.ptr;

      uint32 size1 = read_size_field(obj1);
      uint32 size2 = read_size_field(obj2);

      if (size1 != size2)
        return size2 - size1; //## BUG

      count = 3 * size1;
      elts1 = rel1->buffer;
      elts2 = rel2->buffer;

      break;
    }

    case TYPE_TAG_OBJ: {
      assert(phys_type_1 == TYPE_TAG_OBJ | phys_type_1 == TYPE_OPT_TAG_REC);
      assert(phys_type_2 == TYPE_TAG_OBJ | phys_type_2 == TYPE_OPT_TAG_REC);

      if (phys_type_1 == TYPE_OPT_TAG_REC) {
        if (phys_type_2 == TYPE_OPT_TAG_REC) {
          uint16 repr_id_1 = get_opt_repr_id(obj1);
          uint16 repr_id_2 = get_opt_repr_id(obj2);

          if (repr_id_1 == repr_id_2) {
            void *ptr1 = obj1.core_data.ptr;
            void *ptr2 = obj2.core_data.ptr;
            return opt_repr_cmp(ptr1, ptr2, repr_id_1);
          }

          uint16 tag_id_1 = opt_repr_get_tag_id(repr_id_1);
          uint16 tag_id_2 = opt_repr_get_tag_id(repr_id_2);

          if (tag_id_1 != tag_id_2)
            return tag_id_2 - tag_id_1;

          void *ptr1 = obj1.core_data.ptr;
          void *ptr2 = obj2.core_data.ptr;

          uint32 size1 = opt_repr_get_fields_count(ptr1, repr_id_1);
          uint32 size2 = opt_repr_get_fields_count(ptr2, repr_id_2);

          if (size1 != size2)
            return size2 - size1; //## BUG

          return fast_cmp_bin_rels_slow(untag_opt_tag_rec(obj1), untag_opt_tag_rec(obj2)); //## OPTIMIZABLE
        }
        else {
          TAG_OBJ *ptr = (TAG_OBJ *) obj2.core_data.ptr;
          //## CAN THIS BE OPTIMIZED? WE ALREADY KNOW THE FIRST OBJECT IS
          //## AN AD-HOC RECORD, AND THE SECOND ONE A TAGGED VALUE
          return fast_cmp_objs(clear_all_inline_tags(obj1), ptr->obj);

          // uint16 tag_id_1 = opt_repr_get_tag_id(get_opt_repr_id(obj1));
          // TAG_OBJ *ptr = (TAG_OBJ *) obj2.core_data.ptr;
          // uint16 tag_id_2 = ptr->tag_id;

          // if (tag_id_1 != tag_id_2)
          //   return tag_id_2 - tag_id_1;

          // return fast_cmp_objs(untag_opt_tag_rec(obj1), ptr->obj);
        }
      }
      else {
        if (phys_type_2 == TYPE_OPT_TAG_REC) {
          //## CAN THIS BE OPTIMIZED? WE ALREADY KNOW THE FIRST OBJECT IS
          //## A TAGGED VALUE, AND THE SECOND ONE AN AD-HOC RECORD
          TAG_OBJ *ptr = (TAG_OBJ *) obj1.core_data.ptr;
          return fast_cmp_objs(ptr->obj, clear_all_inline_tags(obj2));

          // TAG_OBJ *ptr = (TAG_OBJ *) obj1.core_data.ptr;
          // uint16 tag_id_1 = ptr->tag_id;
          // uint16 tag_id_2 = opt_repr_get_tag_id(get_opt_repr_id(obj2));

          // if (tag_id_1 != tag_id_2)
          //   return tag_id_2 - tag_id_1;

          // return fast_cmp_objs(ptr->obj, untag_opt_tag_rec(obj2));
        }
        else {
          TAG_OBJ *ptr1 = (TAG_OBJ *) obj1.core_data.ptr;
          TAG_OBJ *ptr2 = (TAG_OBJ *) obj2.core_data.ptr;
          //## CAN THIS BE OPTIMIZED? WE ALREADY KNOW BOTH OBJECT ARE TAGGED
          return fast_cmp_objs(ptr1->obj, ptr2->obj);
        }
      }
    }

    default:
      internal_fail();
  }

  for (int i=0 ; i < count ; i++) {
    int cr = fast_cmp_objs(elts1[i], elts2[i]);
    if (cr != 0)
      return cr;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Returns:   > 0     if obj1 < obj2
//              0     if obj1 = obj2
//            < 0     if obj1 > obj2

/* __attribute__ ((noinline)) */ int fast_cmp_objs(OBJ obj1, OBJ obj2) {
  OBJ_TYPE type1 = get_physical_type(obj1);
  OBJ_TYPE type2 = get_physical_type(obj2);

  if (type1 <= MAX_ALWAYS_INLINE_OBJ_TYPE) {
    if (type2 <= MAX_ALWAYS_INLINE_OBJ_TYPE)
      return shallow_cmp(obj1, obj2);
    else
      return 1;
  }
  else if (type2 <= MAX_ALWAYS_INLINE_OBJ_TYPE)
    return -1;

  int tags_count_1 = get_tags_count(obj1);
  int tags_count_2 = get_tags_count(obj2);

  if (tags_count_1 > 0) {
    if (tags_count_2 > 0) {
      // Both obj1 and obj2 have inline tags

      uint16 tag_id_1 = get_inline_tag_id(obj1);
      uint16 tag_id_2 = get_inline_tag_id(obj2);

      if (tag_id_1 != tag_id_2)
        return tag_id_2 - tag_id_1;

      if (tags_count_1 == 2) {
        if (tags_count_2 == 2) {
          // Both obj1 and obj2 have two inline tags

          tag_id_1 = get_nested_inline_tag_id(obj1);
          tag_id_2 = get_nested_inline_tag_id(obj2);

          if (tag_id_1 != tag_id_2)
            return tag_id_2 - tag_id_1;
        }
        else {
          assert(tags_count_2 == 1);

          // obj1 has two inline tags, obj2 has just one

          if (type2 == TYPE_TAG_OBJ) {
            tag_id_1 = get_nested_inline_tag_id(obj1);
            TAG_OBJ *ptr = (TAG_OBJ *) obj2.core_data.ptr;
            OBJ untagged_obj_2 = ptr->obj;
            tag_id_2 = get_tag_id(untagged_obj_2);

            if (tag_id_1 != tag_id_2)
              return tag_id_2 - tag_id_1;

            // ptr->obj could be an inline object
            return fast_cmp_objs(clear_both_inline_tags(obj1), get_inner_obj(untagged_obj_2)); //## OPTIMIZABLE
          }
          else if (type2 == TYPE_OPT_TAG_REC) {
            tag_id_1 = get_nested_inline_tag_id(obj1);
            tag_id_2 = opt_repr_get_tag_id(get_opt_repr_id(obj2));

            if (tag_id_1 != tag_id_2)
              return tag_id_2 - tag_id_1;

            // The physical type of obj1 could be of TYPE_TAG_OBJ or TYPE_OPT_TAG_REC
            return fast_cmp_objs(clear_both_inline_tags(obj1), untag_opt_tag_rec(obj2)); //## OPTIMIZABLE
          }
          else
            return -1;
        }
      }
      else if (tags_count_2 == 2) {
        assert (tags_count_1 == 1);

        // obj2 has two inline tags, obj1 has only one

        if (type1 == TYPE_TAG_OBJ) {
          TAG_OBJ *ptr = (TAG_OBJ *) obj1.core_data.ptr;
          OBJ untagged_obj_1 = ptr->obj;
          tag_id_1 = get_tag_id(untagged_obj_1);
          tag_id_2 = get_nested_inline_tag_id(obj2);

          if (tag_id_1 != tag_id_2)
            return tag_id_2 - tag_id_1;

          return fast_cmp_objs(get_inner_obj(ptr->obj), clear_both_inline_tags(obj2)); //## OPTIMIZABLE

        }
        else if (type1 == TYPE_OPT_TAG_REC) {
          tag_id_1 = opt_repr_get_tag_id(get_opt_repr_id(obj1));
          tag_id_2 = get_nested_inline_tag_id(obj2);

          if (tag_id_1 != tag_id_2)
            return tag_id_2 - tag_id_1;

          return fast_cmp_objs(untag_opt_tag_rec(obj1), clear_both_inline_tags(obj2));
        }
        else
          return 1;
      }
    }
    else {
      assert(tags_count_1 > 0 & tags_count_2 == 0);

      // obj1 has inline tags, obj2 doesn't

      assert(type2 != TYPE_TAG_OBJ);

      if (type2 == TYPE_OPT_TAG_REC) {
        uint16 tag_id_1 = get_inline_tag_id(obj1);
        uint16 tag_id_2 = opt_repr_get_tag_id(get_opt_repr_id(obj2));

        if (tag_id_1 != tag_id_2)
          return tag_id_2 - tag_id_1;

        return fast_cmp_objs(get_inner_obj(obj1), untag_opt_tag_rec(obj2));
      }
      else
        return -1;
    }
  }
  else if (tags_count_2 > 0) {
    assert(tags_count_1 == 0 & tags_count_2 > 0);

    // obj1 has no inline tags, obj2 does

    assert(type1 != TYPE_TAG_OBJ);

    if (type1 == TYPE_OPT_TAG_REC) {
      uint16 tag_id_1 = opt_repr_get_tag_id(get_opt_repr_id(obj1));
      uint16 tag_id_2 = get_inline_tag_id(obj2);

      if (tag_id_1 != tag_id_2)
        return tag_id_2 - tag_id_1;

      return fast_cmp_objs(untag_opt_tag_rec(obj1), get_inner_obj(obj2)); //## OPTIMIZABLE
    }
    else
      return 1;
  }

  // Here inline tags have all been accounted for, and they have to be ignored
  return fast_cmp_objs_ignoring_inline_tags(obj1, type1, obj2, type2);
}

////////////////////////////////////////////////////////////////////////////////

int cmp_objs(OBJ obj1, OBJ obj2) {
  return fast_cmp_objs(obj1, obj2);
}

inline int sign(int value) {
  if (value < 0)
    return -1;
  else if (value > 0)
    return 1;
  else
    return 0;
}


/* __attribute__ ((noinline)) */ int comp_objs(OBJ obj1, OBJ obj2) {
  // int slow_cmp_objs(OBJ obj1, OBJ obj2);

  // if (sign(fast_cmp_objs(obj1, obj2)) != sign(slow_cmp_objs(obj1, obj2))) {
  //   print(obj1);
  //   print(obj2);

  //   int fcr = fast_cmp_objs(obj1, obj2);
  //   int scr = slow_cmp_objs(obj1, obj2);

  //   printf("expected: %d, actual: %d\n", scr, fcr);

  //   exit(1);
  // }

  return fast_cmp_objs(obj1, obj2);
}

/* __attribute__ ((noinline)) */ bool are_eq(OBJ obj1, OBJ obj2) {
  if (are_shallow_eq(obj1, obj2))
    return true;

  if (is_always_inline(obj1) | is_always_inline(obj2))
    return false;

  return fast_cmp_objs(obj1, obj2) == 0;
}
