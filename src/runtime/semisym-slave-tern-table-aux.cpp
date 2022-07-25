#include "lib.h"


void semisym_slave_tern_table_aux_init(SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_init(&table_aux->slave_table_aux, mem_pool);
}

void semisym_slave_tern_table_aux_reset(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_reset(&table_aux->slave_table_aux);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_clear(SLAVE_TERN_TABLE_AUX *table_aux) {
  bin_table_aux_clear(&table_aux->slave_table_aux);
}

void semisym_slave_tern_table_aux_delete(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12, uint32 arg3) {
  bin_table_aux_delete(slave_table, &table_aux->slave_table_aux, surr12, arg3);
}

void semisym_slave_tern_table_aux_delete_12(SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12) {
  bin_table_aux_delete_1(&table_aux->slave_table_aux, surr12);
}

void semisym_slave_tern_table_aux_delete_3(SLAVE_TERN_TABLE_AUX *table_aux, uint32 arg3) {
  bin_table_aux_delete_2(&table_aux->slave_table_aux, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_insert(SLAVE_TERN_TABLE_AUX *table_aux, uint32 surr12, uint32 arg3) {
  bin_table_aux_insert(&table_aux->slave_table_aux, surr12, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_aux_apply_deletions(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_apply_deletions(slave_table, &table_aux->slave_table_aux, NULL, NULL, remove3, store3, mem_pool);
}

void semisym_slave_tern_table_aux_apply_insertions(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  bin_table_aux_apply_insertions(slave_table, &table_aux->slave_table_aux, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

//## BUG BUG BUG: KEY VIOLATIONS WILL BE DETECTED BUT REPORTED INCORRECTLY

bool semisym_slave_tern_table_aux_check_key_12(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  return bin_table_aux_check_key_1(slave_table, &table_aux->slave_table_aux, mem_pool);
}

bool semisym_slave_tern_table_aux_check_key_3(BIN_TABLE *slave_table, SLAVE_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  return bin_table_aux_check_key_2(slave_table, &table_aux->slave_table_aux, mem_pool);
}
