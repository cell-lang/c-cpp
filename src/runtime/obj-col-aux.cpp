#include "lib.h"


void queue_u32_init(QUEUE_U32 *queue) {
  queue->capacity = OBJ_COL_AUX_INLINE_SIZE;
  queue->count = 0;
  queue->array = queue->inline_array;
}

void queue_u32_insert(QUEUE_U32 *queue, uint32 value) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *array = queue->array;
  assert(count <= capacity);
  if (count == capacity) {
    impl_fail(""); //## IMPLEMENT IMPLEMENT IMPLEMENT
  }
  array[count] = value;
  queue->count = count + 1;
}

void queue_u32_reset(QUEUE_U32 *queue) {
  queue->count = 0;
  if (queue->capacity != OBJ_COL_AUX_INLINE_SIZE) {
    queue->capacity = OBJ_COL_AUX_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

////////////////////////////////////////////////////////////////////////////////

void queue_u32_obj_init(QUEUE_U32_OBJ *queue) {
  queue->capacity = OBJ_COL_AUX_INLINE_SIZE;
  queue->count = 0;
  queue->u32_array = queue->inline_u32_array;
  queue->obj_array = queue->inline_obj_array;
}

void queue_u32_obj_insert(QUEUE_U32_OBJ *queue, uint32 u32_value, OBJ obj_value) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *u32_array = queue->u32_array;
  OBJ *obj_array = queue->obj_array;
  assert(count <= capacity);
  if (count == capacity) {
    impl_fail(""); //## IMPLEMENT IMPLEMENT IMPLEMENT
  }
  u32_array[count] = u32_value;
  obj_array[count] = obj_value;
  queue->count = count + 1;
}

void queue_u32_obj_reset(QUEUE_U32_OBJ *queue) {
  queue->count = 0;
  if (queue->capacity != OBJ_COL_AUX_INLINE_SIZE) {
    queue->capacity = OBJ_COL_AUX_INLINE_SIZE;
    queue->u32_array = queue->inline_u32_array;
    queue->obj_array = queue->inline_obj_array;
  }
}

////////////////////////////////////////////////////////////////////////////////

void obj_col_init(OBJ_COL_AUX *col_aux) {
  queue_u32_init(&col_aux->deletions);
  queue_u32_obj_init(&col_aux->insertions);
  queue_u32_obj_init(&col_aux->updates);

  // long[] bitmap = Array.emptyLongArray;

  // uint32 bitmap_size;
  // uint64 *bitmap;
  // uint64 *bitmap_inline[OBJ_COL_AUX_INLINE_SIZE];

  col_aux->max_idx_plus_one = 0;
  col_aux->dirty = false;
}

////////////////////////////////////////////////////////////////////////////////

void obj_col_aux_clear(OBJ_COL_AUX *col_aux) {
  // ObjColumn.Iter it = column.getIter();
  // while (!it.done()) {
  //   delete(it.getIdx());
  //   it.next();
  // }
  impl_fail("");
}

void obj_col_aux_delete_1(OBJ_COL_AUX *col_aux, uint32 index) {
  queue_u32_insert(&col_aux->deletions, index);
  if (index >= col_aux->max_idx_plus_one)
    col_aux->max_idx_plus_one = index + 1;
}

void obj_col_aux_insert(OBJ_COL_AUX *col_aux, uint32 index, OBJ value) {
  queue_u32_obj_insert(&col_aux->insertions, index, value);
  if (index >= col_aux->max_idx_plus_one)
    col_aux->max_idx_plus_one = index + 1;
}

void obj_col_aux_update(OBJ_COL_AUX *col_aux, uint32 index, OBJ value) {
  queue_u32_obj_insert(&col_aux->updates, index, value);
  if (index >= col_aux->max_idx_plus_one)
    col_aux->max_idx_plus_one = index + 1;
}

////////////////////////////////////////////////////////////////////////////////

void obj_col_aux_apply(OBJ_COL *col, OBJ_COL_AUX *col_aux) {
  uint32 count = col_aux->deletions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->deletions.array;
    for (uint32 i=0 ; i < count ; i++)
      obj_col_delete(col, idxs[i]);
  }

  count = col_aux->updates.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    OBJ *values = col_aux->updates.obj_array;
    for (uint32 i=0 ; i < count ; i++)
      obj_col_update(col, idxs[i], values[i]);
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    OBJ *values = col_aux->insertions.obj_array;
    for (uint32 i=0 ; i < count ; i++)
      obj_col_insert(col, idxs[i], values[i]);
  }
}

//////////////////////////////////////////////////////////////////////////////

void obj_col_aux_reset(OBJ_COL_AUX *col_aux) {
  queue_u32_reset(&col_aux->deletions);
  queue_u32_obj_reset(&col_aux->insertions);
  queue_u32_obj_reset(&col_aux->updates);

  col_aux->max_idx_plus_one = 0;

  if (col_aux->dirty) {
    col_aux->dirty = false;

    uint64 ops_count = (uint64) col_aux->deletions.count + (uint64) col_aux->insertions.count + (uint64) col_aux->updates.count;
    uint32 bitmap_size = col_aux->bitmap_size;

    uint64 *bitmap = col_aux->bitmap;

    if (3 * ops_count < bitmap_size) {
      uint32 count = col_aux->deletions.count;
      if (count > 0) {
        uint32 *idxs = col_aux->deletions.array;
        for (uint32 i=0 ; i < count ; i++)
          bitmap[idxs[i] / 32] = 0;
      }

      count = col_aux->insertions.count;
      if (count > 0) {
        uint32 *idxs = col_aux->insertions.u32_array;
        for (uint32 i=0 ; i < count ; i++)
          bitmap[idxs[i] / 32] = 0;
      }

      count = col_aux->updates.count;
      if (count > 0) {
        uint32 *idxs = col_aux->updates.u32_array;
        for (uint32 i=0 ; i < count ; i++)
          bitmap[idxs[i] / 32] = 0;
      }
    }
    else
      memset(bitmap, 0, bitmap_size * sizeof(uint64));
  }
}

//////////////////////////////////////////////////////////////////////////////

void record_col_1_key_violation(OBJ_COL_AUX *col_aux, uint32 idx, OBJ value, OBJ other_value, bool between_new) {
//   //## BUG: Stores may contain only part of the value (id(5) -> 5)
//   Obj key = store.surrToValue(idx);
//   Obj[] tuple1 = new Obj[] {key, value};
//   Obj[] tuple2 = new Obj[] {key, otherValue};
//   return new KeyViolationException(relvarName, KeyViolationException.key_1, tuple1, tuple2, betweenNew);

  //## IMPLEMENT IMPLEMENT IMPLEMENT
}

void record_col_1_key_violation(OBJ_COL *col, OBJ_COL_AUX *col_aux, uint32 idx, OBJ value, bool between_new) {
  if (between_new) {
    uint32 count = col_aux->updates.count;
    uint32 *idxs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      if (idxs[i] == idx) {
        record_col_1_key_violation(col_aux, idx, value, col_aux->updates.obj_array[i], between_new);
        return;
      }

    count = col_aux->insertions.count;
    idxs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < count ; i++)
      if (idxs[i] == idx) {
        record_col_1_key_violation(col_aux, idx, value, col_aux->insertions.obj_array[i], between_new);
        return;
      }

    internal_fail();
  }
  else
    record_col_1_key_violation(col_aux, idx, value, obj_col_lookup(col, idx), between_new);
}

//////////////////////////////////////////////////////////////////////////////

bool obj_col_aux_build_bitmap_and_check_key(OBJ_COL *col, OBJ_COL_AUX *col_aux) {
  assert(col_aux->insertions.count > 0 && col_aux->updates.count > 0);
  assert(col_aux->max_idx_plus_one > 0);

  uint32 max_idx = col_aux->max_idx_plus_one - 1;
  uint64 *bitmap = col_aux->bitmap;

  if (max_idx / 32 >= col_aux->bitmap_size) {
    //bitmap = Array.extend(bitmap, Array.capacity(bitmap.length, maxIdx / 32 + 1));
    impl_fail("");
  }

  col_aux->dirty = true;

  // 00 - untouched
  // 01 - deleted
  // 10 - inserted
  // 11 - updated or inserted and deleted

  uint32 count = col_aux->deletions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->deletions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      uint32 slot_idx = idx / 32;
      uint32 shift = 2 * (idx % 32);
      uint64 slot = bitmap[slot_idx];
      slot |= 1ULL << shift;
      bitmap[slot_idx] = slot;
    }
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->updates.u32_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      uint32 slot_idx = idx / 32;
      uint32 shift = 2 * (idx % 32);
      uint64 slot = bitmap[slot_idx];
      if (((slot >> shift) & 2) != 0) {
        //## HERE I WOULD ACTUALLY NEED TO CHECK THAT THE NEW VALUE IS DIFFERENT FROM THE OLD ONE
        record_col_1_key_violation(col, col_aux, idx, col_aux->updates.obj_array[i], true);
        return false;
      }
      slot |= 3ULL << shift;
      bitmap[slot_idx] = slot;
    }
  }

  count = col_aux->insertions.count;
  if (count > 0) {
    uint32 *idxs = col_aux->insertions.u32_array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 idx = idxs[i];
      uint32 slot_idx = idx / 32;
      uint32 shift = 2 * (idx % 32);
      uint64 slot = bitmap[slot_idx];
      uint64 flags = (slot >> shift) & 3;
      if (flags >= 2) {
        //## HERE I WOULD ACTUALLY NEED TO CHECK THAT THE NEW VALUE IS DIFFERENT FROM THE OLD ONE
        record_col_1_key_violation(col, col_aux, idx, col_aux->insertions.obj_array[i], true);
        return false;
      }
      if (flags == 0 && obj_col_contains_1(col, idx)) {
        //## HERE I WOULD ACTUALLY NEED TO CHECK THAT THE NEW VALUE IS DIFFERENT FROM THE OLD ONE
        record_col_1_key_violation(col, col_aux, idx, col_aux->insertions.obj_array[i], false);
        return false;
      }
      slot |= 2ULL << shift;
      bitmap[slot_idx] = slot;
    }
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////

bool obj_col_aux_contains_1(OBJ_COL *col, OBJ_COL_AUX *col_aux, uint32 surr_1) {
  if (surr_1 < col_aux->max_idx_plus_one) {
    // This call is only needed to build the delete/update/insert bitmap
    //## BUG BUG BUG: IS THIS NECESSARY?
    // if (!col_aux->dirty)
    //   obj_col_aux_build_bitmap_and_check_key(col, col_aux);
    assert(col_aux->dirty);

    uint32 slot_idx = surr_1 / 32;
    uint32 shift = 2 * (surr_1 % 32);
    uint64 slot = col_aux->bitmap[slot_idx];
    uint64 status = slot >> shift;

    if ((status & 2) != 0)
      return true;  // Inserted/updated
    else if ((status & 1) != 0)
      return false; // Deleted and not reinserted
  }

  return obj_col_contains_1(col, surr_1);
}

//////////////////////////////////////////////////////////////////////////////

OBJ obj_col_aux_lookup(OBJ_COL *col, OBJ_COL_AUX *col_aux, uint32 surr_1) {
  if (surr_1 < col_aux->max_idx_plus_one && (col_aux->insertions.count != 0 || col_aux->updates.count != 0)) {
    assert(col_aux->dirty);

    uint32 slot_idx = surr_1 / 32;
    uint32 shift = 2 * (surr_1 % 32);
    uint64 slot = col_aux->bitmap[slot_idx];
    uint64 status = slot >> shift;

    if ((status & 2) != 0) {
      //## THIS IS A PERFORMANCE DISASTER

      uint32 count = col_aux->insertions.count;
      if (count > 0) {
        uint32 *idxs = col_aux->insertions.u32_array;
        for (uint32 i=0 ; i < count ; i++)
          if (idxs[i] == surr_1)
            return col_aux->insertions.obj_array[i];
      }

      count = col_aux->updates.count;
      if (count > 0) {
        uint32 *idxs = col_aux->updates.u32_array;
        for (uint32 i=0 ; i < count ; i++)
          if (idxs[i] == surr_1)
            return col_aux->updates.obj_array[i];
      }

      internal_fail();
    }
  }

  return obj_col_lookup(col, surr_1);
}

//////////////////////////////////////////////////////////////////////////////

bool obj_col_aux_check_key_1(OBJ_COL *col, OBJ_COL_AUX *col_aux) {
  if (col_aux->insertions.count != 0 || col_aux->updates.count != 0) {
    assert(col_aux->max_idx_plus_one > 0 && !col_aux->dirty);
    return obj_col_aux_build_bitmap_and_check_key(col, col_aux);
  }
  else
    return true;
}