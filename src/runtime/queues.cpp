#include "lib.h"


void queue_u32_init(QUEUE_U32 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->array = queue->inline_array;
  queue->deduplicated = false;
}

void queue_u32_reset(QUEUE_U32 *queue) {
  queue->count = 0;
  queue->deduplicated = false;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

void queue_u32_insert(QUEUE_U32 *queue, uint32 value) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *array = queue->array;
  assert(count <= capacity);
  if (count == capacity) {
    array = resize_uint32_array(array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->array = array;
  }
  array[count] = value;
  queue->count = count + 1;
}

void queue_u32_deduplicate(QUEUE_U32 *queue, COL_UPDATE_BIT_MAP *bit_map, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(bit_map));

  uint32 count = queue->count;
  if (count > 1 && !queue->deduplicated) {
    uint32 *array = queue->array;
    uint32 target_idx = 0;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 elt = array[i];
      if (col_update_bit_map_check_and_set(bit_map, elt, mem_pool)) {
        while (i < --count) {
          uint32 last_elt = array[count];
          if (!col_update_bit_map_check_and_set(bit_map, last_elt, mem_pool)) {
            array[i] = last_elt;
            break;
          }
        }
      }
    }
    queue->count = count;
    queue->deduplicated = true;
    col_update_bit_map_clear(bit_map);
  }
}

////////////////////////////////////////////////////////////////////////////////

void queue_u32_obj_init(QUEUE_U32_OBJ *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
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
    u32_array = resize_uint32_array(u32_array, capacity, 2 * capacity);
    obj_array = resize_obj_array(obj_array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->u32_array = u32_array;
    queue->obj_array = obj_array;
  }
  u32_array[count] = u32_value;
  obj_array[count] = obj_value;
  queue->count = count + 1;
}

void queue_u32_obj_reset(QUEUE_U32_OBJ *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->u32_array = queue->inline_u32_array;
    queue->obj_array = queue->inline_obj_array;
  }
}

////////////////////////////////////////////////////////////////////////////////

void queue_u32_double_init(QUEUE_U32_FLOAT *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->u32_array = queue->inline_u32_array;
  queue->float_array = queue->inline_float_array;
}

void queue_u32_double_reset(QUEUE_U32_FLOAT *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->u32_array = queue->inline_u32_array;
    queue->float_array = queue->inline_float_array;
  }
}

void queue_u32_double_insert(QUEUE_U32_FLOAT *queue, uint32 u32_value, double double_value) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *u32_array = queue->u32_array;
  double *float_array = queue->float_array;
  assert(count <= capacity);
  if (count == capacity) {
    u32_array = resize_uint32_array(u32_array, capacity, 2 * capacity);
    float_array = resize_float_array(float_array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->u32_array = u32_array;
    queue->float_array = float_array;
  }
  u32_array[count] = u32_value;
  float_array[count] = double_value;
  queue->count = count + 1;
}

////////////////////////////////////////////////////////////////////////////////

void queue_u32_i64_init(QUEUE_U32_I64 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->u32_array = queue->inline_u32_array;
  queue->i64_array = queue->inline_i64_array;
}

void queue_u32_i64_reset(QUEUE_U32_I64 *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->u32_array = queue->inline_u32_array;
    queue->i64_array = queue->inline_i64_array;
  }
}

void queue_u32_i64_insert(QUEUE_U32_I64 *queue, uint32 u32_value, int64 i64_value) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *u32_array = queue->u32_array;
  int64 *i64_array = queue->i64_array;
  assert(count <= capacity);
  if (count == capacity) {
    u32_array = resize_uint32_array(u32_array, capacity, 2 * capacity);
    i64_array = resize_int64_array(i64_array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->u32_array = u32_array;
    queue->i64_array = i64_array;
  }
  u32_array[count] = u32_value;
  i64_array[count] = i64_value;
  queue->count = count + 1;
}

////////////////////////////////////////////////////////////////////////////////

void queue_u64_init(QUEUE_U64 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->array = queue->inline_array;
  queue->sorted = false;
}

void queue_u64_reset(QUEUE_U64 *queue) {
  queue->count = 0;
  queue->sorted = false;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

void queue_u64_insert(QUEUE_U64 *queue, uint64 value) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint64 *array = queue->array;
  assert(count <= capacity);
  if (count == capacity) {
    array = resize_uint64_array(array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->array = array;
  }
  array[count] = value;
  queue->count = count + 1;
}

void queue_u64_deduplicate(QUEUE_U64 *queue) {
  uint32 count = queue->count;
  if (count > 1) {
    uint64 *array = queue->array;
    if (!queue->sorted) {
      sort_u64(array, count);
      queue->sorted = true;
    }
    uint32 target_idx = 1;
    uint64 prev_elt = array[0];
    for (uint32 i=1 ; i < count ; i++) {
      uint64 elt = array[i];
      if (elt != prev_elt) {
        if (target_idx != i)
          array[target_idx++] = elt;
        prev_elt = elt;
      }
    }
    queue->count = target_idx;
  }
}

bool queue_u64_contains(QUEUE_U64 *queue, uint64 value) {
  uint32 count = queue->count;
  if (count > 0) {
    uint64 *array = queue->array;
    //## TODO: USE A LINEAR SEARCH WHEN THE NUMBER OF ELEMENTS IS SMALL
    if (!queue->sorted) {
      sort_u64(array, count);
      queue->sorted = true;
    }
    return sorted_u64_array_contains(array, count, value);
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

void queue_3u32_init(QUEUE_3U32 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->array = queue->inline_array;
}

void queue_3u32_reset(QUEUE_3U32 *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

void queue_3u32_insert(QUEUE_3U32 *queue, uint32 x, uint32 y, uint32 z) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  TUPLE_3U32 *array = queue->array;
  assert(count <= capacity);
  if (count == capacity) {
    //## BAD BAD BAD: FIX THIS
    array = (TUPLE_3U32 *) resize_uint32_array((uint32 *) array, 3 * capacity, 6 * capacity);
    queue->capacity = 2 * capacity;
    queue->array = array;
  }
  TUPLE_3U32 *ptr = array + count;
  ptr->x = x;
  ptr->y = y;
  ptr->z = z;
  queue->count = count + 1;
}
