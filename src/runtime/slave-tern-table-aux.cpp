#include "lib.h"


void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_prepare(QUEUE_U32 *queue);
void queue_u32_reset(QUEUE_U32 *queue);
bool queue_u32_contains(QUEUE_U32 *queue, uint32 value);

////////////////////////////////////////////////////////////////////////////////

void queue_u64_init(QUEUE_U64 *);
void queue_u64_insert(QUEUE_U64 *, uint64);
void queue_u64_prepare(QUEUE_U64 *);
void queue_u64_flip_words(QUEUE_U64 *);
void queue_u64_reset(QUEUE_U64 *);
bool queue_u64_contains(QUEUE_U64 *, uint64);

////////////////////////////////////////////////////////////////////////////////

void queue_3u32_insert(QUEUE_U32 *queue, uint32 value1, uint32 value2, uint32 value3) {
  //## IMPLEMENT BETTER
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *array = queue->array;
  assert(count <= capacity);
  if (count + 3 >= capacity) {
    assert(2 * capacity > count + 3);
    array = resize_uint32_array(array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->array = array;
  }
  array[count] = value1;
  array[count + 1] = value2;
  array[count + 2] = value3;
  queue->count = count + 3;
}

void queue_3u32_prepare(QUEUE_U32 *queue) {
  uint32 count = queue->count % 3;
  if (count > 16)
    sort_3u32(queue->array, queue->count);
}

bool queue_3u32_contains(QUEUE_U32 *queue, uint32 value1, uint32 value2, uint32 value3) {
  assert(queue->count % 3 == 0);
  uint32 count = queue->count % 3;
  if (count > 0) {
    uint32 *ptr = queue->array;
    if (count > 16)
      return sorted_3u32_array_contains(ptr, count, value1, value2, value3);
    //## WE COULD SPEED THIS UP BY READING A 64-BIT WORD AT A TIME
    for (uint32 i=0 ; i < count ; i++) {
      if (ptr[0] == value1 && ptr[1] == value2 && ptr[2] == value3)
        return true;
      ptr += 3;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_init(SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_init(&table_aux->slave_table_aux, mem_pool);
  queue_u32_init(&table_aux->insertions);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_clear(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_clear(&table_aux->slave_table_aux);
}

void slave_tern_table_aux_delete(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
}

void slave_tern_table_aux_delete_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
}

void slave_tern_table_aux_delete_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3) {
  MASTER_BIN_TABLE_ITER_1 iter;
  master_bin_table_iter_1_init(master_table, &iter, arg1);
  while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_1_get_surr(&iter);
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
    master_bin_table_iter_1_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg2, uint32 arg3) {
  MASTER_BIN_TABLE_ITER_2 iter;
  master_bin_table_iter_2_init(master_table, &iter, arg2);
  while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_2_get_surr(&iter);
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
    master_bin_table_iter_2_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1) {
  MASTER_BIN_TABLE_ITER_1 iter;
  master_bin_table_iter_1_init(master_table, &iter, arg1);
  while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_1_get_surr(&iter);
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
    master_bin_table_iter_1_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_2(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg2) {
  MASTER_BIN_TABLE_ITER_2 iter;
  master_bin_table_iter_2_init(master_table, &iter, arg2);
  while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_2_get_surr(&iter);
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
    master_bin_table_iter_2_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_3(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg3) {
  bin_table_aux_delete_2(&table_aux->slave_table_aux, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_insert(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  queue_3u32_insert(&table_aux->insertions, arg1, arg2, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void null_incr_rc(void *, uint32) {

}

void null_decr_rc(void *, void *, uint32) {

}

void slave_tern_table_aux_apply(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool) {
  //## MAYBE bin_table_aux_apply() SHOULD JUST CHECK THAT incr_rc_1() AND decr_rc_1() ARE NOT NULL
  //## OR MAYBE THERE SHOULD BE 2 VERSIONS OF bin_table_aux_apply()
  bin_table_aux_apply(slave_table, &table_aux->slave_table_aux, null_incr_rc, null_decr_rc, NULL, NULL, incr_rc_3, decr_rc_3, store_3, store_aux_3, mem_pool);

  uint32 count = table_aux->insertions.count % 3;
  if (count > 0) {
    uint32 *ptr = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = ptr[0];
      uint32 arg2 = ptr[1];
      uint32 arg3 = ptr[2];
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_insert(slave_table, surr12, arg3, mem_pool))
        incr_rc_3(store_3, arg3);
      ptr += 3;
    }
  }
}

void slave_tern_table_aux_reset(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_reset(&table_aux->slave_table_aux);
  queue_u32_reset(&table_aux->insertions);
}

////////////////////////////////////////////////////////////////////////////////

static void slave_tern_table_aux_record_col_3_key_violation(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

}

static void slave_tern_table_aux_record_cols_12_key_violation(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

bool slave_tern_table_aux_check_key_3(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    count %= 3;

    queue_3u32_prepare(&table_aux->insertions);

    uint32 prev_arg_1 = 0xFFFFFFFF;
    uint32 prev_arg_2 = 0xFFFFFFFF;
    uint32 prev_arg_3 = 0xFFFFFFFF;

    uint32 *ptr = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = *(ptr++);
      uint32 arg2 = *(ptr++);
      uint32 arg3 = *(ptr++);

      if (arg3 == prev_arg_3 && (arg1 != prev_arg_1 || arg2 != prev_arg_2)) {
        slave_tern_table_aux_record_col_3_key_violation(table_aux, arg1, arg2, arg3, true);
        return false;
      }

      if (bin_table_contains_2(slave_table, arg3)) {
        if (!bin_table_aux_arg2_was_deleted(&table_aux->slave_table_aux, arg3)) {
          slave_tern_table_aux_record_col_3_key_violation(table_aux, arg1, arg2, arg3, false);
          return false;
        }
      }

      prev_arg_1 = arg1;
      prev_arg_2 = arg2;
      prev_arg_3 = arg3;
    }
  }
  return true;
}

bool slave_tern_table_aux_check_key_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, MASTER_BIN_TABLE_AUX *master_table_aux, SLAVE_TERN_TABLE_AUX *table_aux) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    count %= 3;

    queue_3u32_prepare(&table_aux->insertions);

    uint32 prev_arg_1 = 0xFFFFFFFF;
    uint32 prev_arg_2 = 0xFFFFFFFF;
    uint32 prev_arg_3 = 0xFFFFFFFF;

    uint32 *ptr = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = *(ptr++);
      uint32 arg2 = *(ptr++);
      uint32 arg3 = *(ptr++);

      if (arg1 == prev_arg_1 && arg2 == prev_arg_2 && arg3 != prev_arg_3) {
        slave_tern_table_aux_record_cols_12_key_violation(table_aux, arg1, arg2, arg3, true);
        return false;
      }

      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
      if (surr12 != 0xFFFFFFFF && bin_table_contains_1(slave_table, surr12))
        if (!bin_table_aux_arg1_was_deleted(&table_aux->slave_table_aux, surr12)) {
          slave_tern_table_aux_record_cols_12_key_violation(table_aux, arg1, arg2, arg3, false);
          return false;
        }

      prev_arg_1 = arg1;
      prev_arg_2 = arg2;
      prev_arg_3 = arg3;
    }
  }
  return true;
}
