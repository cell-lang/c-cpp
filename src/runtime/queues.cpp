#include "lib.h"


void queue_u32_init(QUEUE_U32 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->array = queue->inline_array;
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

void queue_u32_prepare(QUEUE_U32 *queue) {
  uint32 count = queue->count;
  if (count > 16)
    sort_u32(queue->array, queue->count);
}

void queue_u32_reset(QUEUE_U32 *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

bool queue_u32_contains(QUEUE_U32 *queue, uint32 value) {
  uint32 count = queue->count;
  if (count > 0) {
    uint32 *array = queue->array;
    if (count > 16)
      return sorted_u32_array_contains(array, count, value);
    //## WE COULD SPEED THIS UP BY READING A 64-BIT WORD AT A TIME
    for (uint32 i=0 ; i < count ; i++)
      if (array[i] == value)
        return true;
  }
  return false;
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

void queue_u32_double_reset(QUEUE_U32_FLOAT *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->u32_array = queue->inline_u32_array;
    queue->float_array = queue->inline_float_array;
  }
}

////////////////////////////////////////////////////////////////////////////////

void queue_u32_i64_init(QUEUE_U32_I64 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->u32_array = queue->inline_u32_array;
  queue->i64_array = queue->inline_i64_array;
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

void queue_u32_i64_reset(QUEUE_U32_I64 *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->u32_array = queue->inline_u32_array;
    queue->i64_array = queue->inline_i64_array;
  }
}
