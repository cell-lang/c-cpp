#include "lib.h"
#include "extern.h"


uint64 set_obj_mem_size(uint64 size);
uint64 seq_obj_mem_size(uint64 capacity);
uint64 bin_rel_obj_mem_size(uint64 size);
uint32 tern_rel_obj_mem_size(uint64 size);
uint64 map_obj_mem_size(uint64 size);
uint64 tag_obj_mem_size();


uint32 copy_memory_size(OBJ obj, bool (*already_in_place)(void*, void*), void *self) {
  if (is_inline_obj(obj))
    return 0;

  if (already_in_place(self, get_ref_obj_ptr(obj)))
    return 0;

  switch (get_physical_type(obj)) {
    case TYPE_NE_SEQ: {
      SEQ_OBJ *ptr = get_seq_ptr(obj);
    }

    case TYPE_NE_SET: {
      SET_OBJ *ptr = get_set_ptr(obj);

    }

    case TYPE_NE_BIN_REL: {
      BIN_REL_OBJ *ptr = get_bin_rel_ptr(obj);

    }

    case TYPE_NE_TERN_REL: {
      TERN_REL_OBJ *ptr = get_tern_rel_ptr(obj);

    }

    case TYPE_TAG_OBJ: {
      TAG_OBJ *ptr = get_tag_obj_ptr(obj);

    }

    case TYPE_NE_SLICE: {
      SEQ_OBJ *ptr = get_seq_ptr(obj);
      uint32 length = get_seq_length(obj);
      uint32 offset = get_seq_offset(obj);

    }

    case TYPE_NE_MAP: {
      BIN_REL_OBJ *ptr = get_bin_rel_ptr(obj);

    }

    case TYPE_NE_LOG_MAP: {
      BIN_REL_OBJ *ptr = get_bin_rel_ptr(obj);

    }

    case TYPE_OPT_REC: {
      void *ptr = get_opt_repr_ptr(obj);
      uint16 repr_id = get_opt_repr_id(obj);
    }

    case TYPE_OPT_TAG_REC: {
      void *ptr = get_opt_repr_ptr(obj);
      uint16 repr_id = get_opt_repr_id(obj);

    }
  }
}

////////////////////////////////////////////////////////////////////////////////

SEQ_OBJ *make_or_get_seq_obj_copy(OBJ *array, uint32 size) {
  assert(size > 0);

  SEQ_OBJ *seq_copy = new_seq(size);
  OBJ *buffer = seq_copy->buffer;
  for (int i=0 ; i < size ; i++)
    buffer[i] = copy_obj(array[i]);
  return seq_copy;
}

SEQ_OBJ *make_or_get_seq_obj_copy(SEQ_OBJ *seq) {
  if (seq->capacity > 0) {
    // The object has not been copied yet. Making a new object large enough
    // to accomodate all the elements of the original sequence. We are using
    // <size> and not <capacity> for the new sequence because it's best to
    // leave the decision of how much extra memory to allocate to the memory
    // allocator, which is aware of the context.
    //## IF THIS IS THE ONLY REFERENCE TO THE SEQUENCE, AND IF <length> IS
    //## LOWER THAN SIZE, WE COULD COPY ONLY THE FIRST LENGTH ELEMENTS...
    uint32 size = seq->size;
    SEQ_OBJ *seq_copy = new_seq(size);
    // Now we copy all the elements of the sequence
    OBJ *buff = seq->buffer;
    OBJ *buff_copy = seq_copy->buffer;
    for (int i=0 ; i < size ; i++)
      buff_copy[i] = copy_obj(buff[i]);
    // We mark the old sequence as "copied", and we store a pointer to the copy
    // into it. The fields of the original object are never going to be used again,
    // even by the memory manager, so we can safely overwrite them.
    // seq->capacity = 0;
    // * (SEQ_OBJ **) buff = seq_copy;
    // Returning the new object
    return seq_copy;
  }
  else {
    // The object has already been copied. We just return a pointer to the copy
    SEQ_OBJ *seq_copy = * (SEQ_OBJ **) seq->buffer;
    return seq_copy;
  }
}

SET_OBJ *make_or_get_set_obj_copy(SET_OBJ *set) {
  uint32 size = set->size;
  if (size > 0) {
    // The object has not been copied yet, so we do it now.
    SET_OBJ *set_copy = new_set(size);
    // Now we copy all the elements of the sequence
    OBJ *buff = set->buffer;
    OBJ *buff_copy = set_copy->buffer;
    for (int i=0 ; i < size ; i++)
      buff_copy[i] = copy_obj(buff[i]);
    // We mark the old sequence as "copied", and we store a pointer to the copy
    // into it. The fields of the original object are never going to be used again,
    // even by the memory manager, so we can safely overwrite them.
    // set->size = 0;
    // * (SET_OBJ **) buff = set_copy;
    // Returning the new object
    return set_copy;
  }
  else {
    // The object has already been copied. We just return a pointer to the copy
    SET_OBJ *set_copy = * (SET_OBJ **) set->buffer;
    return set_copy;
  }
}

BIN_REL_OBJ *make_or_get_bin_rel_obj_copy(BIN_REL_OBJ *rel) {
  uint32 size = rel->size;
  if (size > 0) {
    // The object has not been copied yet, so we do it now.
    BIN_REL_OBJ *rel_copy = new_bin_rel(size);
    // Now we copy all the elements of the collection
    OBJ *buff = rel->buffer;
    OBJ *buff_copy = rel_copy->buffer;
    for (int i=0 ; i < 2 * size ; i++)
      buff_copy[i] = copy_obj(buff[i]);
    // Now we copy the extra data at the end
    uint32 *rev_idxs = get_right_to_left_indexes(rel);
    uint32 *rev_idxs_copy = get_right_to_left_indexes(rel_copy);
    memcpy(rev_idxs_copy, rev_idxs, size * sizeof(uint32));
    // We mark the old object as "copied", and we store a pointer to the copy
    // into it. The fields of the original object are never going to be used again,
    // even by the memory manager, so we can safely overwrite them.
    // rel->size = 0;
    // * (BIN_REL_OBJ **) buff = rel_copy;
    // Returning the new object
    return rel_copy;
  }
  else {
    // The object has already been copied. We just return a pointer to the copy
    BIN_REL_OBJ *rel_copy = * (BIN_REL_OBJ **) rel->buffer;
    return rel_copy;
  }
}

TERN_REL_OBJ *make_or_get_tern_rel_obj_copy(TERN_REL_OBJ *rel) {
  uint32 size = rel->size;
  if (size > 0) {
    // The object has not been copied yet, so we do it now.
    TERN_REL_OBJ *rel_copy = new_tern_rel(size);
    // Now we copy all the elements of the collection
    OBJ *buff = rel->buffer;
    OBJ *buff_copy = rel_copy->buffer;
    for (int i=0 ; i < 3 * size ; i++)
      buff_copy[i] = copy_obj(buff[i]);
    // Now we copy the extra data at the end
    uint32 *idxs_start = get_rotated_index(rel, 1);
    uint32 *copy_idxs_start = get_rotated_index(rel_copy, 1);
    memcpy(copy_idxs_start, idxs_start, 2 * size * sizeof(uint32));
    // We mark the old object as "copied", and we store a pointer to the copy
    // into it. The fields of the original object are never going to be used again,
    // even by the memory manager, so we can safely overwrite them.
    // rel->size = 0;
    // * (TERN_REL_OBJ **) buff = rel_copy;
    // Returning the new object
    return rel_copy;
  }
  else {
    // The object has already been copied. We just return a pointer to the copy
    TERN_REL_OBJ *rel_copy = * (TERN_REL_OBJ **) rel->buffer;
    return rel_copy;
  }
}

BIN_REL_OBJ *make_or_get_map_obj_copy(BIN_REL_OBJ *map) {
  uint32 size = map->size;
  if (size > 0) {
    // The object has not been copied yet, so we do it now.
    BIN_REL_OBJ *map_copy = new_map(size);
    // Now we copy all the elements of the sequence
    OBJ *buff = map->buffer;
    OBJ *buff_copy = map_copy->buffer;
    for (int i=0 ; i < 2 * size ; i++)
      buff_copy[i] = copy_obj(buff[i]);
    // We mark the old sequence as "copied", and we store a pointer to the copy
    // into it. The fields of the original object are never going to be used again,
    // even by the memory manager, so we can safely overwrite them.
    // map->size = 0;
    // * (BIN_REL_OBJ **) buff = map_copy;
    // Returning the new object
    return map_copy;
  }
  else {
    // The object has already been copied. We just return a pointer to the copy
    BIN_REL_OBJ *map_copy = * (BIN_REL_OBJ **) map->buffer;
    return map_copy;
  }
}

TAG_OBJ *make_or_get_tag_obj_copy(TAG_OBJ *tag_obj) {
  if (tag_obj->unused_field == 0) {
    // The object has not been copied yet, so we do it now
    TAG_OBJ *tag_obj_copy = new_tag_obj();
    tag_obj_copy->tag_id = tag_obj->tag_id;
    tag_obj_copy->obj = copy_obj(tag_obj->obj);
    // We mark the old object as "copied", and we store a pointer to the copy
    // into it. The fields of the original object are never going to be used again,
    // even by the memory manager, so we can safely overwrite them.
    // tag_obj->unused_field = 0xFFFF;
    // * (TAG_OBJ **) &tag_obj->obj = tag_obj_copy;
    // Returning the new object
    return tag_obj_copy;
  }
  else {
    // The object has already been copied. We just return a pointer to the copy
    TAG_OBJ *tag_obj_copy = * (TAG_OBJ **) &tag_obj->obj;
    return tag_obj_copy;
  }
}

////////////////////////////////////////////////////////////////////////////////

OBJ copy_obj(OBJ obj) {
  if (is_inline_obj(obj) || !needs_copying(get_ref_obj_ptr(obj)))
    return obj;

  switch (get_physical_type(obj)) {
    case TYPE_NE_SEQ: {
      SEQ_OBJ *seq_copy = make_or_get_seq_obj_copy(get_seq_ptr(obj));
      return repoint_to_copy(obj, seq_copy->buffer);
    }

    case TYPE_NE_SET: {
      SET_OBJ *set_copy = make_or_get_set_obj_copy(get_set_ptr(obj));
      return repoint_to_copy(obj, set_copy);
    }

    case TYPE_NE_BIN_REL: {
      BIN_REL_OBJ *rel_copy = make_or_get_bin_rel_obj_copy(get_bin_rel_ptr(obj));
      return repoint_to_copy(obj, rel_copy);
    }

    case TYPE_NE_TERN_REL: {
      TERN_REL_OBJ *rel_copy = make_or_get_tern_rel_obj_copy(get_tern_rel_ptr(obj));
      return repoint_to_copy(obj, rel_copy);
    }

    case TYPE_TAG_OBJ: {
      TAG_OBJ *tag_obj_copy = make_or_get_tag_obj_copy(get_tag_obj_ptr(obj));
      return repoint_to_copy(obj, tag_obj_copy);
    }

    case TYPE_NE_SLICE: {
      SEQ_OBJ *seq_copy = make_or_get_seq_obj_copy(get_seq_buffer_ptr(obj), get_seq_length(obj));
      return repoint_slice_to_seq(obj, seq_copy);
    }

    case TYPE_NE_MAP: {
      BIN_REL_OBJ *map_copy = make_or_get_map_obj_copy(get_bin_rel_ptr(obj));
      return repoint_to_copy(obj, map_copy);
    }

    case TYPE_NE_LOG_MAP: {
      BIN_REL_OBJ *rel_copy = make_or_get_bin_rel_obj_copy(get_bin_rel_ptr(obj));
      return repoint_to_copy(obj, rel_copy);
    }

    case TYPE_OPT_REC: {
      void *ptr = get_opt_repr_ptr(obj);
      uint16 repr_id = get_opt_repr_id(obj);
      void *copy = opt_repr_copy(ptr, repr_id);
      return get_inner_obj(make_opt_tag_rec(copy, repr_id)); //## BAD: INEFFICIENT
    }

    case TYPE_OPT_TAG_REC: {
      void *ptr = get_opt_repr_ptr(obj);
      uint16 repr_id = get_opt_repr_id(obj);
      void *copy = opt_repr_copy(ptr, repr_id);
      return repoint_to_copy(obj, copy);
    }

    default:
      internal_fail();
  }
}
