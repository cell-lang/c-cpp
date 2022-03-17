#include "lib.h"


void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_prepare(QUEUE_U32 *queue);
void queue_u32_reset(QUEUE_U32 *queue);
bool queue_u32_contains(QUEUE_U32 *queue, uint32 value);

void queue_u64_init(QUEUE_U64 *);
void queue_u64_insert(QUEUE_U64 *, uint64);
void queue_u64_prepare(QUEUE_U64 *);
void queue_u64_flip_words(QUEUE_U64 *);
void queue_u64_reset(QUEUE_U64 *);
bool queue_u64_contains(QUEUE_U64 *, uint64);

void queue_3u32_insert(QUEUE_U32 *queue, uint32 value1, uint32 value2, uint32 value3);
void queue_3u32_prepare(QUEUE_U32 *queue);
bool queue_3u32_contains(QUEUE_U32 *queue, uint32 value1, uint32 value2, uint32 value3);

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_init(SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_init(&table_aux->slave_table_aux, mem_pool);
  queue_u32_init(&table_aux->insertions);
}

void semisym_slave_tern_table_aux_reset(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_reset(&table_aux->slave_table_aux);
  queue_u32_reset(&table_aux->insertions);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_clear(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_clear(&table_aux->slave_table_aux);
}

void semisym_slave_tern_table_aux_delete(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
}

void semisym_slave_tern_table_aux_delete_12(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
}

void semisym_slave_tern_table_aux_delete_13(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3) {
  SYM_MASTER_BIN_TABLE_ITER_1 iter;
  sym_master_bin_table_iter_1_init(master_table, &iter, arg1);
  while (!sym_master_bin_table_iter_1_is_out_of_range(&iter)) {
    uint32 surr12 = sym_master_bin_table_iter_1_get_surr(&iter);
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
    sym_master_bin_table_iter_1_move_forward(&iter);
  }
}

void semisym_slave_tern_table_aux_delete_1(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1) {
  SYM_MASTER_BIN_TABLE_ITER_1 iter;
  sym_master_bin_table_iter_1_init(master_table, &iter, arg1);
  while (!sym_master_bin_table_iter_1_is_out_of_range(&iter)) {
    uint32 surr12 = sym_master_bin_table_iter_1_get_surr(&iter);
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
    sym_master_bin_table_iter_1_move_forward(&iter);
  }
}

void semisym_slave_tern_table_aux_delete_3(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg3) {
  bin_table_aux_delete_2(&table_aux->slave_table_aux, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_insert(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  queue_3u32_insert(&table_aux->insertions, arg1, arg2, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_apply(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool) {
  //## MAYBE bin_table_aux_apply() SHOULD JUST CHECK THAT incr_rc_1() AND decr_rc_1() ARE NOT NULL
  //## OR MAYBE THERE SHOULD BE 2 VERSIONS OF bin_table_aux_apply()
  bin_table_aux_apply(slave_table, &table_aux->slave_table_aux, null_incr_rc, null_decr_rc, NULL, NULL, incr_rc_3, decr_rc_3, store_3, store_aux_3, mem_pool);

  uint32 count = table_aux->insertions.count / 3;
  if (count > 0) {
    uint32 *ptr = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = ptr[0];
      uint32 arg2 = ptr[1];
      uint32 arg3 = ptr[2];
      uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_insert(slave_table, surr12, arg3, mem_pool))
        incr_rc_3(store_3, arg3);
      ptr += 3;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

// static void semisym_slave_tern_table_aux_record_col_3_key_violation(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

// }

// static void semisym_slave_tern_table_aux_record_cols_12_key_violation(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

// }

////////////////////////////////////////////////////////////////////////////////

bool semisym_slave_tern_table_aux_check_key_3(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux) {
  // uint32 count = table_aux->insertions.count;
  // if (count > 0) {
  //   count /= 3;

  //   queue_3u32_prepare(&table_aux->insertions);

  //   uint32 prev_arg_1 = 0xFFFFFFFF;
  //   uint32 prev_arg_2 = 0xFFFFFFFF;
  //   uint32 prev_arg_3 = 0xFFFFFFFF;

  //   uint32 *ptr = table_aux->insertions.array;
  //   for (uint32 i=0 ; i < count ; i++) {
  //     uint32 arg1 = *(ptr++);
  //     uint32 arg2 = *(ptr++);
  //     uint32 arg3 = *(ptr++);

  //     if (arg3 == prev_arg_3 && (arg1 != prev_arg_1 || arg2 != prev_arg_2)) {
  //       semisym_slave_tern_table_aux_record_col_3_key_violation(table_aux, arg1, arg2, arg3, true);
  //       return false;
  //     }

  //     if (bin_table_contains_2(slave_table, arg3)) {
  //       if (!bin_table_aux_arg2_was_deleted(slave_table, &table_aux->slave_table_aux, arg3)) {
  //         semisym_slave_tern_table_aux_record_col_3_key_violation(table_aux, arg1, arg2, arg3, false);
  //         return false;
  //       }
  //     }

  //     prev_arg_1 = arg1;
  //     prev_arg_2 = arg2;
  //     prev_arg_3 = arg3;
  //   }
  // }
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool semisym_slave_tern_table_aux_check_key_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, MASTER_BIN_TABLE_AUX *master_table_aux, SLAVE_TERN_TABLE_AUX *table_aux) {
//   uint32 count = table_aux->insertions.count;
//   if (count > 0) {
//     count /= 3;

//     queue_3u32_prepare(&table_aux->insertions);

//     uint32 prev_arg_1 = 0xFFFFFFFF;
//     uint32 prev_arg_2 = 0xFFFFFFFF;
//     uint32 prev_arg_3 = 0xFFFFFFFF;

//     uint32 *ptr = table_aux->insertions.array;
//     for (uint32 i=0 ; i < count ; i++) {
//       uint32 arg1 = *(ptr++);
//       uint32 arg2 = *(ptr++);
//       uint32 arg3 = *(ptr++);

//       if (arg1 == prev_arg_1 && arg2 == prev_arg_2 && arg3 != prev_arg_3) {
//         semisym_slave_tern_table_aux_record_cols_12_key_violation(table_aux, arg1, arg2, arg3, true);
//         return false;
//       }

//       uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
//       if (surr12 != 0xFFFFFFFF && bin_table_contains_1(slave_table, surr12))
//         if (!bin_table_aux_arg1_was_deleted(slave_table, &table_aux->slave_table_aux, surr12)) {
//           semisym_slave_tern_table_aux_record_cols_12_key_violation(table_aux, arg1, arg2, arg3, false);
//           return false;
//         }

//       prev_arg_1 = arg1;
//       prev_arg_2 = arg2;
//       prev_arg_3 = arg3;
//     }
//   }
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}