#include "lib.h"

void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_prepare(QUEUE_U32 *queue);
void queue_u32_reset(QUEUE_U32 *queue);
bool queue_u32_contains(QUEUE_U32 *queue, uint32 value);

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

void queue_u64_prepare(QUEUE_U64 *queue) {
  uint32 count = queue->count;
  if (count > 16)
    sort_u64(queue->array, count);
}

void queue_u64_flip_words(QUEUE_U64 *queue) {
  uint32 count = queue->count;
  if (count > 0) {
    uint64 *array = queue->array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 word = array[i];
      uint64 flipped_word = (word << 32) | ((word >> 32) & 0xFFFFFFFF);
      array[i] = flipped_word;
    }
  }
}

void queue_u64_reset(QUEUE_U64 *queue) {
  queue->count = 0;
  if (queue->capacity != QUEUE_INLINE_SIZE) {
    queue->capacity = QUEUE_INLINE_SIZE;
    queue->array = queue->inline_array;
  }
}

bool queue_u64_contains(QUEUE_U64 *queue, uint64 value) {
  uint32 count = queue->count;
  if (count > 0) {
    uint64 *array = queue->array;
    if (count > 16)
      return sorted_u64_array_contains(array, count, value);
    for (uint32 i=0 ; i < count ; i++)
      if (array[i] == value)
        return true;
  }
  return false;
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
  table_aux->deletions_prepared = false;
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

static void bin_table_aux_prepare_deletions(BIN_TABLE_AUX *table_aux) {
  assert(!table_aux->deletions_prepared);
  queue_u64_prepare(&table_aux->deletions);
  queue_u32_prepare(&table_aux->deletions_1);
  queue_u32_prepare(&table_aux->deletions_2);
  table_aux->deletions_prepared = true;
}

////////////////////////////////////////////////////////////////////////////////

static bool bin_table_aux_was_deleted(BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (!table_aux->deletions_prepared)
    bin_table_aux_prepare_deletions(table_aux);
  return queue_u64_contains(&table_aux->deletions, pack_args(arg1, arg2)) ||
         queue_u32_contains(&table_aux->deletions_1, arg1) ||
         queue_u32_contains(&table_aux->deletions_2, arg2);
}

////////////////////////////////////////////////////////////////////////////////

static void record_col_1_key_violation(BIN_TABLE *col, BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_aux_check_key_1(BIN_TABLE *table, BIN_TABLE_AUX *table_aux) {
  assert(table_aux->insertions.count > 0);

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    queue_u64_prepare(&table_aux->insertions);

    bool clear = table_aux->clear;
    uint64 *array = table_aux->insertions.array;

    uint32 prev_arg1 = 0xFFFFFFFF;
    uint32 prev_arg2 = 0xFFFFFFFF;
    for (int i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(i == 0 || arg1 > prev_arg1 || (arg1 == prev_arg1 && arg2 >= prev_arg2));
      if (arg1 == prev_arg1 && arg2 != prev_arg2) {
        record_col_1_key_violation(table, table_aux, arg1, arg2, true);
        return false;
      }
      prev_arg1 = arg1;
      prev_arg2 = arg2;
      if (!clear) {
        if (bin_table_contains_1(table, arg1))
          if (!bin_table_aux_was_deleted(table_aux, arg1, arg2)) {
            record_col_1_key_violation(table, table_aux, arg1, arg2, false);
            return false;
          }
      }
    }
  }

  return true;
}

bool bin_table_aux_check_key_2(BIN_TABLE *table, BIN_TABLE_AUX *table_aux) {
  assert(table_aux->insertions.count > 0);

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    queue_u64_flip_words(&table_aux->insertions);
    queue_u64_prepare(&table_aux->insertions);

    bool clear = table_aux->clear;
    uint64 *array = table_aux->insertions.array;

    uint32 prev_arg1 = 0xFFFFFFFF;
    uint32 prev_arg2 = 0xFFFFFFFF;
    for (int i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      assert(i == 0 || arg2 > prev_arg2 || (arg2 == prev_arg2 && arg1 >= prev_arg1));
      if (arg2 == prev_arg2 && arg1 != prev_arg1) {
        record_col_1_key_violation(table, table_aux, arg1, arg2, true);
        queue_u64_flip_words(&table_aux->insertions);
        return false;
      }
      prev_arg1 = arg1;
      prev_arg2 = arg2;
      if (!clear) {
        if (bin_table_contains_2(table, arg2))
          if (!bin_table_aux_was_deleted(table_aux, arg1, arg2)) {
            record_col_1_key_violation(table, table_aux, arg1, arg2, false);
            queue_u64_flip_words(&table_aux->insertions);
            return false;
          }
      }
    }
    queue_u64_flip_words(&table_aux->insertions);
  }

  return true;
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
    bin_table_clear(table, mem_pool);
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
        BIN_TABLE_ITER_1 iter;
        bin_table_iter_1_init(table, &iter, arg1);
        while (!bin_table_iter_1_is_out_of_range(&iter)) {
          uint32 arg2 = bin_table_iter_1_get_1(&iter);
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
        BIN_TABLE_ITER_2 iter;
        bin_table_iter_2_init(table, &iter, arg2);
        while (!bin_table_iter_2_is_out_of_range(&iter)) {
          uint32 arg1 = bin_table_iter_2_get_1(&iter);
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
      if (bin_table_insert(table, arg1, arg2, mem_pool)) {
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
  table_aux->deletions_prepared = false;
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
