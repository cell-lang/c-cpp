#include "lib.h"

void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_reset(QUEUE_U32 *queue);

////////////////////////////////////////////////////////////////////////////////

void queue_u64_init(QUEUE_U64 *queue) {
  queue->capacity = QUEUE_INLINE_SIZE;
  queue->count = 0;
  queue->array = queue->inline_array;
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

void queue_u64_reset(QUEUE_U64 *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

////////////////////////////////////////////////////////////////////////////////

inline uint32 unpack_arg1(uint64 args) {
  return (uint32) (args >> 32);
}

inline uint32 unpack_arg2(uint64 args) {
  return (uint32) args;
}

inline uint64 pack_args(uint32 arg1, uint32 arg2) {
  uint64 args = (((uint64) arg1) << 32) | arg2;
  assert(unpack_arg1(args) == arg1);
  assert(unpack_arg2(args) == arg2);
  return args;
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_aux_init(BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u32_init(&table_aux->deletions_2);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_aux_clear(BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void bin_table_aux_delete(BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void bin_table_aux_delete_1(BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void bin_table_aux_delete_2(BIN_TABLE_AUX *table_aux, uint32 arg2) {
  queue_u32_insert(&table_aux->deletions_2, arg2);
}

void bin_table_aux_insert(BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->insertions, pack_args(arg1, arg2));

}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_aux_check_key_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  internal_fail();
}

bool bin_table_aux_check_key_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  internal_fail();
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_aux_apply(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    BIN_TABLE_ITER iter;
    bin_table_iter_init(table, &iter);
    while (!bin_table_iter_is_out_of_range(&iter)) {
      uint32 arg1 = bin_table_iter_get_1(&iter);
      decr_rc_1(store_1, store_aux_1, arg1);
      uint32 arg2 = bin_table_iter_get_2(&iter);
      decr_rc_2(store_2, store_aux_2, arg2);
    }
    bin_table_clear(table);
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (bin_table_delete(table, arg1, arg2)) {
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
        }
      }
    }

    count = table_aux->deletions_1.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg1 = array[i];
        BIN_TABLE_ITER iter;
        bin_table_iter_init_1(table, &iter, arg1);
        while (!bin_table_iter_is_out_of_range(&iter)) {
          uint32 arg2 = bin_table_iter_get_1(&iter);
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
        }
        bin_table_delete_1(table, arg1);
      }
    }

    count = table_aux->deletions_2.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_2.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg2 = array[i];
        BIN_TABLE_ITER iter;
        bin_table_iter_init_2(table, &iter, arg2);
        while (!bin_table_iter_is_out_of_range(&iter)) {
          uint32 arg1 = bin_table_iter_get_1(&iter);
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
        }
        bin_table_delete_2(table, arg2);
      }
    }
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      if (bin_table_insert(table, mem_pool, arg1, arg2)) {
        incr_rc_1(store_1, arg1);
        incr_rc_2(store_2, arg2);
      }
    }
  }
}

void bin_table_aux_reset(BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u32_reset(&table_aux->deletions_2);
  queue_u64_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

// bool bin_table_aux_contains(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {

// }

// bool bin_table_aux_contains_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg1) {

// }

// bool bin_table_aux_contains_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 arg2) {

// }

// OBJ  bin_table_aux_lookup(BIN_TABLE *table, BIN_TABLE_AUX *table_aux, uint32 surr_1) {

// }
