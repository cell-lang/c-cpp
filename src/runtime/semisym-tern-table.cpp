#include "lib.h"


bool semisym_tern_table_insert(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  int32 code = sym_master_bin_table_insert_ex(&table->master, arg1, arg2, mem_pool);
  uint32 surr12 = code >= 0 ? code : -code - 1;
  return bin_table_insert(&table->slave, surr12, arg3, mem_pool);
}

void semisym_tern_table_clear(TERN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  sym_master_bin_table_clear(&table->master, mem_pool);
  bin_table_clear(&table->slave, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

uint32 semisym_tern_table_size(TERN_TABLE *table) {
  return bin_table_size(&table->slave);
}

//## BAD BAD BAD: FOR SOME OF THESE OPERATION THE SLAVE VERSION ENDS UP DOING MORE WORK THAN NECESSARY

uint32 semisym_tern_table_count_1(TERN_TABLE *table, uint32 arg1) {
  return semisym_slave_tern_table_count_1(&table->master, &table->slave, arg1);
}

uint32 semisym_tern_table_count_3(TERN_TABLE *table, uint32 arg3) {
  return semisym_slave_tern_table_count_3(&table->slave, arg3);
}

uint32 semisym_tern_table_count_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return semisym_slave_tern_table_count_12(&table->master, &table->slave, arg1, arg2);
}

uint32 semisym_tern_table_count_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return semisym_slave_tern_table_count_13(&table->master, &table->slave, arg1, arg3);
}

bool semisym_tern_table_contains(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3) {
  return semisym_slave_tern_table_contains(&table->master, &table->slave, arg1, arg2, arg3);
}

bool semisym_tern_table_contains_1(TERN_TABLE *table, uint32 arg1) {
  return semisym_slave_tern_table_contains_1(&table->master, &table->slave, arg1);
}

bool semisym_tern_table_contains_3(TERN_TABLE *table, uint32 arg3) {
  return semisym_slave_tern_table_contains_3(&table->slave, arg3);
}

bool semisym_tern_table_contains_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return semisym_slave_tern_table_contains_12(&table->master, &table->slave, arg1, arg2);
}

bool semisym_tern_table_contains_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return semisym_slave_tern_table_contains_13(&table->master, &table->slave, arg1, arg3);
}

uint32 semisym_tern_table_lookup_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return semisym_slave_tern_table_lookup_12(&table->master, &table->slave, arg1, arg2);
}

uint32 semisym_tern_table_lookup_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return semisym_slave_tern_table_lookup_13(&table->master, &table->slave, arg1, arg3);
}

////////////////////////////////////////////////////////////////////////////////

bool semisym_tern_table_cols_12_are_key(TERN_TABLE *table) {
  return semisym_slave_tern_table_cols_12_are_key(&table->slave);
}

bool semisym_tern_table_cols_13_are_key(TERN_TABLE *table) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool semisym_tern_table_col_3_is_key(TERN_TABLE *table) {
  return semisym_slave_tern_table_col_3_is_key(&table->slave);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_copy_to(TERN_TABLE *table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  semisym_slave_tern_table_copy_to(&table->master, &table->slave, surr_to_obj_1_2, store_1_2, surr_to_obj_3, store_3, strm_1, strm_2, strm_3);
}

void semisym_tern_table_write(WRITE_FILE_STATE *write_state, TERN_TABLE *table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  semisym_slave_tern_table_write(write_state, &table->master, &table->slave, surr_to_obj_1_2, store_1_2, surr_to_obj_3, store_3, idx1, idx2, idx3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_iter_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER *iter) {
  semisym_slave_tern_table_iter_init(&table->master, &table->slave, iter);
}

void semisym_tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *iter) {
  semisym_slave_tern_table_iter_move_forward(iter);
}

bool semisym_tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *iter) {
  return semisym_slave_tern_table_iter_is_out_of_range(iter);
}

uint32 semisym_tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *iter) {
  return semisym_slave_tern_table_iter_get_1(iter);
}

uint32 semisym_tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *iter) {
  return semisym_slave_tern_table_iter_get_2(iter);
}

uint32 semisym_tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *iter) {
  return semisym_slave_tern_table_iter_get_3(iter);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_iter_1_init(TERN_TABLE *table, SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter, uint32 arg1) {
  semisym_slave_tern_table_iter_1_init(&table->master, &table->slave, iter, arg1);
}

void semisym_tern_table_iter_1_move_forward(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  semisym_slave_tern_table_iter_1_move_forward(iter);
}

bool semisym_tern_table_iter_1_is_out_of_range(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  return semisym_slave_tern_table_iter_1_is_out_of_range(iter);
}

uint32 semisym_tern_table_iter_1_get_1(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  return semisym_slave_tern_table_iter_1_get_1(iter);
}

uint32 semisym_tern_table_iter_1_get_2(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  return semisym_slave_tern_table_iter_1_get_2(iter);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_iter_3_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_3 *iter, uint32 arg3) {
  semisym_slave_tern_table_iter_3_init(&table->master, &table->slave, iter, arg3);
}

void semisym_tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *iter) {
  semisym_slave_tern_table_iter_3_move_forward(iter);
}

bool semisym_tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *iter) {
  return semisym_slave_tern_table_iter_3_is_out_of_range(iter);
}

uint32 semisym_tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *iter) {
  return semisym_slave_tern_table_iter_3_get_1(iter);

}

uint32 semisym_tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *iter) {
  return semisym_slave_tern_table_iter_3_get_2(iter);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_iter_12_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_12 *iter, uint32 arg1, uint32 arg2) {
  semisym_slave_tern_table_iter_12_init(&table->master, &table->slave, iter, arg1, arg2);
}

void semisym_tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *iter) {
  semisym_slave_tern_table_iter_12_move_forward(iter);
}

bool semisym_tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *iter) {
  return semisym_slave_tern_table_iter_12_is_out_of_range(iter);
}

uint32 semisym_tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *iter) {
  return semisym_slave_tern_table_iter_12_get_1(iter);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_iter_13_init(TERN_TABLE *table, SLAVE_TERN_TABLE_ITER_13 *iter, uint32 arg1, uint32 arg3) {
  semisym_slave_tern_table_iter_13_init(&table->master, &table->slave, iter, arg1, arg3);
}

void semisym_tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *iter) {
  semisym_slave_tern_table_iter_13_move_forward(iter);
}

bool semisym_tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *iter) {
  return semisym_slave_tern_table_iter_13_is_out_of_range(iter);
}

uint32 semisym_tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *iter) {
  return semisym_slave_tern_table_iter_13_get_1(iter);
}
