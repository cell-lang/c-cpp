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

void slave_tern_table_aux_delete(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12, uint32 arg3) {
  bin_table_aux_delete(slave_table, &table_aux->slave_table_aux, surr12, arg3);
}

void slave_tern_table_aux_delete_12(SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12) {
  bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
}

void slave_tern_table_aux_delete_3(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg3) {
  bin_table_aux_delete_2(&table_aux->slave_table_aux, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_insert(SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12, uint32 arg3) {
  bin_table_aux_insert(&table_aux->slave_table_aux, surr12, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_aux_apply_deletions(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_apply_deletions(slave_table, &table_aux->slave_table_aux, NULL, NULL, remove3, store3, mem_pool);
}

void slave_tern_table_aux_apply_insertions(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_apply_insertions(slave_table, &table_aux->slave_table_aux, mem_pool);
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

void slave_tern_table_aux_prepare(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_prepare(&table_aux->slave_table_aux);
}

////////////////////////////////////////////////////////////////////////////////

uint32 slave_tern_table_aux_size(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux) {
  return bin_table_aux_size(table, &table_aux->slave_table_aux);
}

bool slave_tern_table_aux_is_empty(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux) {
  return bin_table_aux_is_empty(table, &table_aux->slave_table_aux);
}

bool slave_tern_table_aux_contains_surr(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr) {
  return bin_table_aux_contains_1(table, &table_aux->slave_table_aux, surr);
}

////////////////////////////////////////////////////////////////////////////////

bool slave_tern_table_aux_check_foreign_key_master_bin_table_forward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *target_table, MASTER_BIN_TABLE_AUX *target_table_aux) {
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

bool slave_tern_table_aux_check_foreign_key_master_bin_table_backward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  return bin_table_aux_check_foreign_key_master_bin_table_backward(table, &table_aux->slave_table_aux, src_table, src_table_aux);
}

bool slave_tern_table_aux_check_foreign_key_unary_table_3_backward(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  return bin_table_aux_check_foreign_key_unary_table_2_backward(table, &table_aux->slave_table_aux, src_table, src_table_aux);
}