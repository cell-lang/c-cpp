#include "lib.h"


void slave_tern_table_aux_init(SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_init(&table_aux->slave_table_aux, mem_pool);
}

void slave_tern_table_aux_reset(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_reset(&table_aux->slave_table_aux);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_clear(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_clear(&table_aux->slave_table_aux);
}

void slave_tern_table_aux_delete(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
}

void slave_tern_table_aux_delete_12(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
}

void slave_tern_table_aux_delete_13(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3) {
  MASTER_BIN_TABLE_ITER_1 iter;
  master_bin_table_iter_1_init_surrs(master_table, &iter, arg1);
  while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_1_get_surr(&iter);
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
    master_bin_table_iter_1_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_23(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg2, uint32 arg3) {
  MASTER_BIN_TABLE_ITER_2 iter;
  master_bin_table_iter_2_init(master_table, &iter, arg2);
  while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_2_get_surr(&iter);
    bin_table_aux_delete(&table_aux->slave_table_aux, surr12, arg3);
    master_bin_table_iter_2_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_1(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1) {
  MASTER_BIN_TABLE_ITER_1 iter;
  master_bin_table_iter_1_init_surrs(master_table, &iter, arg1);
  while (!master_bin_table_iter_1_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_1_get_surr(&iter);
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
    master_bin_table_iter_1_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_2(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg2) {
  MASTER_BIN_TABLE_ITER_2 iter;
  master_bin_table_iter_2_init(master_table, &iter, arg2);
  while (!master_bin_table_iter_2_is_out_of_range(&iter)) {
    uint32 surr12 = master_bin_table_iter_2_get_surr(&iter);
    bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
    master_bin_table_iter_2_move_forward(&iter);
  }
}

void slave_tern_table_aux_delete_3(MASTER_BIN_TABLE *master_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg3) {
  bin_table_aux_delete_2(&table_aux->slave_table_aux, arg3);
}

////////////////////////////////////////////////////////////////////////////////

//## BAD BAD BAD: THIS IS ALL WRONG. THE SIGNATURE OUGHT TO BE:
//##   void slave_tern_table_aux_insert(SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12, uint32 arg3)
void slave_tern_table_aux_insert(MASTER_BIN_TABLE *master_table, MASTER_BIN_TABLE_AUX *master_table_aux, SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_aux_lookup_surr(master_table, master_table_aux, arg1, arg2);
  bin_table_aux_insert(&table_aux->slave_table_aux, surr12, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_apply(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool) {
  //## MAYBE bin_table_aux_apply() SHOULD JUST CHECK THAT incr_rc_1() AND decr_rc_1() ARE NOT NULL
  //## OR MAYBE THERE SHOULD BE 2 VERSIONS OF bin_table_aux_apply()
  bin_table_aux_apply(slave_table, &table_aux->slave_table_aux, null_incr_rc, null_decr_rc, NULL, NULL, incr_rc_3, decr_rc_3, store_3, store_aux_3, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

//## BUG BUG BUG: KEY VIOLATIONS WILL BE DETECTED BUT REPORTED INCORRECTLY

bool slave_tern_table_aux_check_key_12(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  return bin_table_aux_check_key_1(slave_table, &table_aux->slave_table_aux, mem_pool);
}

bool slave_tern_table_aux_check_key_3(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  return bin_table_aux_check_key_2(slave_table, &table_aux->slave_table_aux, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

bool slave_tern_table_aux_check_foreign_key_master_bin_table_12_forward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *target_table, MASTER_BIN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->slave_table_aux.insertions.count;
  if (num_ins > 0) {
    uint64 *args = table_aux->slave_table_aux.insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint32 surr = unpack_arg1(args[i]);
      if (!master_bin_table_aux_contains_surr(target_table, target_table_aux, surr)) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool slave_tern_table_aux_check_foreign_key_unary_table_3_forward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  return bin_table_aux_check_foreign_key_unary_table_1_forward(table, &table_aux->slave_table_aux, target_table, target_table_aux);
}

////////////////////////////////////////////////////////////////////////////////

bool slave_tern_table_aux_check_foreign_key_master_bin_table_surr_backward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  return bin_table_aux_check_foreign_key_master_bin_table_surr_backward(table, &table_aux->slave_table_aux, src_table, src_table_aux);
}

bool slave_tern_table_aux_check_foreign_key_unary_table_3_backward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  return bin_table_aux_check_foreign_key_unary_table_2_backward(table, &table_aux->slave_table_aux, src_table, src_table_aux);
}