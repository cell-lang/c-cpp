#include "lib.h"


bool semisym_slave_tern_table_insert(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(master_table, arg1, arg2);
  assert(surr12 != 0xFFFFFFFF);
  return bin_table_insert(slave_table, surr12, arg3, mem_pool);
}

bool semisym_slave_tern_table_insert(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  return bin_table_insert(slave_table, surr12, arg3, mem_pool);
}

bool semisym_slave_tern_table_delete(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3) {
  return bin_table_delete(slave_table, surr12, arg3);
}

void semisym_slave_tern_table_clear(BIN_TABLE *slave_table, STATE_MEM_POOL *mem_pool) {
  bin_table_clear(slave_table, mem_pool);
}

//////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_update_12(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3, STATE_MEM_POOL *mem_pool, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3) {
  return slave_tern_table_update_12(slave_table, surr12, arg3, mem_pool, remove3, store3);
}

void semisym_slave_tern_table_update_12_3(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3, STATE_MEM_POOL *mem_pool, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3) {
  return slave_tern_table_update_12_3(slave_table, surr12, arg3, mem_pool, remove3, store3);
}

//////////////////////////////////////////////////////////////////////////////

uint32 semisym_slave_tern_table_size(BIN_TABLE *slave_table) {
  return bin_table_size(slave_table);
}

bool semisym_slave_tern_table_contains(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains(slave_table, surr12, arg3);
}

bool semisym_slave_tern_table_contains_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains_1(slave_table, surr12);
}

bool semisym_slave_tern_table_contains_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg12, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS
  uint32 count = sym_master_bin_table_count(master_table, arg12);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *other_args_12 = count > 1024 ? new_uint32_array(count) : inline_array;
    sym_master_bin_table_restrict(master_table, arg12, other_args_12);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr = sym_master_bin_table_lookup_surr(master_table, arg12, other_args_12[i]);
      if (bin_table_contains(slave_table, surr, arg3))
        return true;
    }
  }
  return false;
}

bool semisym_slave_tern_table_contains_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg) {
  //## BAD BAD BAD: THIS IMPLEMENTATION IS PRETTY INEFFICIENT IN MOST CASES
  uint32 count = sym_master_bin_table_count(master_table, arg);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *other_args = count > 1024 ? new_uint32_array(count) : inline_array;
    sym_master_bin_table_restrict(master_table, arg, other_args);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr = sym_master_bin_table_lookup_surr(master_table, arg, other_args[i]);
      if (bin_table_contains_1(slave_table, surr))
        return true;
    }
  }
  return false;

  //## THIS ONE OUGHT TO WORK FOR A (NON-SLAVE) TERNARY TABLE
  // return sym_master_bin_table_contains_1(master_table, arg1);
}

bool semisym_slave_tern_table_contains_3(BIN_TABLE *slave_table, uint32 arg3) {
  return bin_table_contains_2(slave_table, arg3);
}

uint32 semisym_slave_tern_table_lookup_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(master_table, arg1, arg2);
  if (surr12 == 0xFFFFFFFF)
    soft_fail(NULL);
  return bin_table_lookup_1(slave_table, surr12);
}

uint32 semisym_slave_tern_table_lookup_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg12, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count = sym_master_bin_table_count(master_table, arg12);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *other_args_12 = count > 1024 ? new_uint32_array(count) : inline_array;
    sym_master_bin_table_restrict(master_table, arg12, other_args_12);
    uint32 result = 0xFFFFFFFF;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 other_arg_12 = other_args_12[i];
      uint32 surr = sym_master_bin_table_lookup_surr(master_table, arg12, other_arg_12);
      if (bin_table_contains(slave_table, surr, arg3))
        if (result == 0xFFFFFFFF)
          result = other_arg_12;
        else
          soft_fail(NULL);
    }
    if (result != 0xFFFFFFFF)
      return result;
  }
  soft_fail(NULL);
}

uint32 semisym_slave_tern_table_count_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF ? bin_table_count_1(slave_table, surr12) : 0;
}

uint32 semisym_slave_tern_table_count_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg12, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count_13;
  uint32 count12 = sym_master_bin_table_count(master_table, arg12);
  if (count12 > 0) {
    uint32 inline_array[1024];
    uint32 *other_args_12 = count12 > 1024 ? new_uint32_array(count12) : inline_array;
    sym_master_bin_table_restrict(master_table, arg12, other_args_12);
    for (uint32 i=0 ; i < count12 ; i++) {
      uint32 surr = sym_master_bin_table_lookup_surr(master_table, arg12, other_args_12[i]);
      if (bin_table_contains(slave_table, surr, arg3))
        count_13++;
    }
  }
  return count_13;
}

uint32 semisym_slave_tern_table_count_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg12) {
  uint32 full_count = 0;
  uint32 count12 = sym_master_bin_table_count(master_table, arg12);
  if (count12 > 0) {
    uint32 inline_array[1024];
    uint32 *other_args_12 = count12 > 1024 ? new_uint32_array(count12) : inline_array;
    sym_master_bin_table_restrict(master_table, arg12, other_args_12);
    for (uint32 i=0 ; i < count12 ; i++) {
      uint32 surr = sym_master_bin_table_lookup_surr(master_table, arg12, other_args_12[i]);
      full_count += bin_table_count_1(slave_table, surr);
    }
  }
  return full_count;
}

uint32 semisym_slave_tern_table_count_3(BIN_TABLE *slave_table, uint32 arg3) {
  return bin_table_count_2(slave_table, arg3);
}

////////////////////////////////////////////////////////////////////////////////

bool semisym_slave_tern_table_cols_12_are_key(BIN_TABLE *slave_table) {
  return bin_table_col_1_is_key(slave_table);
}

bool semisym_slave_tern_table_col_3_is_key(BIN_TABLE *slave_table) {
  return bin_table_col_2_is_key(slave_table);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_copy_to(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  slave_tern_table_copy_to(master_table, slave_table, surr_to_obj_1_2, store_1_2, surr_to_obj_1_2, store_1_2, surr_to_obj_3, store_3, strm_1, strm_2, strm_3);
}

void semisym_slave_tern_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  slave_tern_table_write(write_state, master_table, slave_table, surr_to_obj_1_2, store_1_2, surr_to_obj_1_2, store_1_2, surr_to_obj_3, store_3, idx1, idx2, idx3);
}
