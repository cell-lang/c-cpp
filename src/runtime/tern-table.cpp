#include "lib.h"


bool tern_table_insert(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  int32 code = master_bin_table_insert_ex(&table->master, arg1, arg2, mem_pool);
  uint32 surr12 = code >= 0 ? code : -code - 1;
  return bin_table_insert(&table->slave, surr12, arg3, mem_pool);
}

// bool tern_table_delete(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3) {
//   uint32 surr12 = master_bin_table_lookup_surr(&table->master, arg1, arg2);
//   if (surr12 != 0xFFFFFFFF) {
//     if (bin_table_delete(&table->slave, surr12, arg3)) {
//       if (bin_table_count_1(&table->slave, surr12) == 0) {
//         bool found = master_bin_table_delete(&table->master, arg1, arg2); //## MAYBE IT WOULD BE FASTER TO PROVIDE THE SURROGATE INSTEAD?
//         assert(found);
//       }
//       return true;
//     }
//   }
//   return false;
// }

void tern_table_clear(TERN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  master_bin_table_clear(&table->master, mem_pool);
  bin_table_clear(&table->slave, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

uint32 tern_table_size(TERN_TABLE *table) {
  return bin_table_size(&table->slave);
}

uint32 tern_table_count_1(TERN_TABLE *table, uint32 arg1) {
  return slave_tern_table_count_1(&table->master, &table->slave, arg1);
}

uint32 tern_table_count_2(TERN_TABLE *table, uint32 arg2) {
  return slave_tern_table_count_2(&table->master, &table->slave, arg2);
}

uint32 tern_table_count_3(TERN_TABLE *table, uint32 arg3) {
  return slave_tern_table_count_3(&table->slave, arg3);
}

uint32 tern_table_count_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return slave_tern_table_count_12(&table->master, &table->slave, arg1, arg2);
}

uint32 tern_table_count_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return slave_tern_table_count_13(&table->master, &table->slave, arg1, arg3);
}

uint32 tern_table_count_23(TERN_TABLE *table, uint32 arg2, uint32 arg3) {
  return slave_tern_table_count_23(&table->master, &table->slave, arg2, arg3);
}

bool tern_table_contains(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3) {
  return slave_tern_table_contains(&table->master, &table->slave, arg1, arg2, arg3);
}

bool tern_table_contains_1(TERN_TABLE *table, uint32 arg1) {
  return slave_tern_table_contains_1(&table->master, &table->slave, arg1);
}

bool tern_table_contains_2(TERN_TABLE *table, uint32 arg2) {
  return slave_tern_table_contains_2(&table->master, &table->slave, arg2);
}

bool tern_table_contains_3(TERN_TABLE *table, uint32 arg3) {
  return slave_tern_table_contains_3(&table->slave, arg3);
}

bool tern_table_contains_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return slave_tern_table_contains_12(&table->master, &table->slave, arg1, arg2);
}

bool tern_table_contains_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return slave_tern_table_contains_13(&table->master, &table->slave, arg1, arg3);
}

bool tern_table_contains_23(TERN_TABLE *table, uint32 arg2, uint32 arg3) {
  return slave_tern_table_contains_23(&table->master, &table->slave, arg2, arg3);
}


uint32 tern_table_lookup_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return slave_tern_table_lookup_12(&table->master, &table->slave, arg1, arg2);
}

uint32 tern_table_lookup_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return slave_tern_table_lookup_13(&table->master, &table->slave, arg1, arg3);
}

uint32 tern_table_lookup_23(TERN_TABLE *table, uint32 arg2, uint32 arg3) {
  return slave_tern_table_lookup_23(&table->master, &table->slave, arg2, arg3);
}

////////////////////////////////////////////////////////////////////////////////

bool tern_table_cols_12_are_key(TERN_TABLE *table) {
  return bin_table_col_1_is_key(&table->slave);
}

bool tern_table_cols_13_are_key(TERN_TABLE *table) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool tern_table_cols_23_are_key(TERN_TABLE *table) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool tern_table_col_3_is_key(TERN_TABLE *table) {
  return bin_table_col_2_is_key(&table->slave);
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_copy_to(TERN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  slave_tern_table_copy_to(&table->master, &table->slave, surr_to_obj_1, store_1, surr_to_obj_2, store_2, surr_to_obj_3, store_3, strm_1, strm_2, strm_3);
}

void tern_table_write(WRITE_FILE_STATE *write_state, TERN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  slave_tern_table_write(write_state, &table->master, &table->slave, surr_to_obj_1, store_1, surr_to_obj_2, store_2, surr_to_obj_3, store_3, idx1, idx2, idx3);
}
