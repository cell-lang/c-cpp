#include "lib.h"


bool semisym_slave_tern_table_insert(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
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

uint32 semisym_slave_tern_table_size(BIN_TABLE *slave_table) {
  return bin_table_size(slave_table);
}

bool semisym_slave_tern_table_contains(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains(slave_table, surr12, arg3);
}

bool semisym_slave_tern_table_contains_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
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
      uint32 surr = sym_master_bin_table_lookup_surrogate(master_table, arg12, other_args_12[i]);
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
      uint32 surr = sym_master_bin_table_lookup_surrogate(master_table, arg, other_args[i]);
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
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
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
      uint32 surr = sym_master_bin_table_lookup_surrogate(master_table, arg12, other_arg_12);
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
  uint32 surr12 = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
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
      uint32 surr = sym_master_bin_table_lookup_surrogate(master_table, arg12, other_args_12[i]);
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
      uint32 surr = sym_master_bin_table_lookup_surrogate(master_table, arg12, other_args_12[i]);
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

void semisym_slave_tern_table_copy_to(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_12)(void *, uint32), void *store_12, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  slave_tern_table_copy_to(master_table, slave_table, surr_to_obj_12, store_12, surr_to_obj_12, store_12, surr_to_obj_3, store_3, strm_1, strm_2, strm_3);
}

void semisym_slave_tern_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  slave_tern_table_write(write_state, master_table, slave_table, surr_to_obj_12, store_12, surr_to_obj_12, store_12, surr_to_obj_3, store_3, idx1, idx2, idx3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_iter_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER *iter) {
  slave_tern_table_iter_init(master_table, slave_table, iter);
}

void semisym_slave_tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *iter) {
  slave_tern_table_iter_move_forward(iter);
}

bool semisym_slave_tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *iter) {
  return slave_tern_table_iter_is_out_of_range(iter);
}

uint32 semisym_slave_tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *iter) {
  return slave_tern_table_iter_get_1(iter);
}

uint32 semisym_slave_tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *iter) {
  return slave_tern_table_iter_get_2(iter);
}

uint32 semisym_slave_tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *iter) {
  return slave_tern_table_iter_get_3(iter);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_iter_1_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter, uint32 arg1) {
  iter->slave_table = slave_table;
  sym_master_bin_table_iter_1_init(master_table, &iter->master_iter, arg1);
  while (!sym_master_bin_table_iter_1_is_out_of_range(&iter->master_iter)) {
    uint32 surr = sym_master_bin_table_iter_1_get_surr(&iter->master_iter);
    bin_table_iter_1_init(slave_table, &iter->slave_iter, surr);
    if (!bin_table_iter_1_is_out_of_range(&iter->slave_iter))
      return;
    sym_master_bin_table_iter_1_move_forward(&iter->master_iter);
  }
}

void semisym_slave_tern_table_iter_1_move_forward(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  assert(!semisym_slave_tern_table_iter_1_is_out_of_range(iter));

  bin_table_iter_1_move_forward(&iter->slave_iter);
  while (bin_table_iter_1_is_out_of_range(&iter->slave_iter)) {
    sym_master_bin_table_iter_1_move_forward(&iter->master_iter);
    if (sym_master_bin_table_iter_1_is_out_of_range)
      return;
    uint32 surr = sym_master_bin_table_iter_1_get_surr(&iter->master_iter);
    bin_table_iter_1_init(iter->slave_table, &iter->slave_iter, surr);
  }
}

bool semisym_slave_tern_table_iter_1_is_out_of_range(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  return sym_master_bin_table_iter_1_is_out_of_range(&iter->master_iter);
}

uint32 semisym_slave_tern_table_iter_1_get_1(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  assert(!semisym_slave_tern_table_iter_1_is_out_of_range(iter));
  return sym_master_bin_table_iter_1_get_1(&iter->master_iter);
}

uint32 semisym_slave_tern_table_iter_1_get_2(SEMISYM_SLAVE_TERN_TABLE_ITER_1 *iter) {
  assert(!semisym_slave_tern_table_iter_1_is_out_of_range(iter));
  return bin_table_iter_1_get_1(&iter->slave_iter);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_iter_3_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_3 *iter, uint32 arg3) {
  bin_table_iter_2_init(slave_table, &iter->slave_iter, arg3);
  iter->master_table = master_table;
}

void semisym_slave_tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *iter) {
  bin_table_iter_2_move_forward(&iter->slave_iter);
}

bool semisym_slave_tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *iter) {
  return bin_table_iter_2_is_out_of_range(&iter->slave_iter);
}

uint32 semisym_slave_tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *iter) {
  uint32 surr = bin_table_iter_2_get_1(&iter->slave_iter);
  return sym_master_bin_table_get_arg_1(iter->master_table, surr);
}

uint32 semisym_slave_tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *iter) {
  uint32 surr = bin_table_iter_2_get_1(&iter->slave_iter);
  return sym_master_bin_table_get_arg_2(iter->master_table, surr);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_slave_tern_table_iter_12_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_12 *iter, uint32 arg1, uint32 arg2) {
  uint32 surr = sym_master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr != 0xFFFFFFFF)
    bin_table_iter_1_init(slave_table, &iter->slave_iter, surr);
  else
    bin_table_iter_1_init_empty(&iter->slave_iter);
}

void semisym_slave_tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *iter) {
  bin_table_iter_1_move_forward(&iter->slave_iter);
}

bool semisym_slave_tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *iter) {
  return bin_table_iter_1_is_out_of_range(&iter->slave_iter);
}

uint32 semisym_slave_tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *iter) {
  return bin_table_iter_1_get_1(&iter->slave_iter);
}

////////////////////////////////////////////////////////////////////////////////

inline bool on_wrong_args(MASTER_BIN_TABLE *table, BIN_TABLE_ITER_2 *slave_iter, uint32 arg) {
  if (bin_table_iter_2_is_out_of_range(slave_iter))
    return false;
  uint32 surr = bin_table_iter_2_get_1(slave_iter);
  uint32 arg1 = sym_master_bin_table_get_arg_1(table, surr);
  uint32 arg2 = sym_master_bin_table_get_arg_2(table, surr);
  return arg != arg1 & arg != arg2;
}

void semisym_slave_tern_table_iter_13_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_13 *iter, uint32 arg1, uint32 arg3) {
  iter->master_table = master_table;
  iter->arg1 = arg1;
  bin_table_iter_2_init(slave_table, &iter->slave_iter, arg3);
  while (on_wrong_args(master_table, &iter->slave_iter, arg1))
    bin_table_iter_2_move_forward(&iter->slave_iter);
}

void semisym_slave_tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *iter) {
  do
    bin_table_iter_2_move_forward(&iter->slave_iter);
  while (on_wrong_args(iter->master_table, &iter->slave_iter, iter->arg1));
}

bool semisym_slave_tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *iter) {
  return bin_table_iter_2_is_out_of_range(&iter->slave_iter);
}

uint32 semisym_slave_tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *iter) {
  uint32 surr = bin_table_iter_2_get_1(&iter->slave_iter);
  uint32 arg1 = sym_master_bin_table_get_arg_1(iter->master_table, surr);
  uint32 arg2 = sym_master_bin_table_get_arg_2(iter->master_table, surr);
  uint32 arg = iter->arg1;
  assert(arg == arg1 | arg == arg2);
  return arg == arg1 ? arg2 : arg1;
}
