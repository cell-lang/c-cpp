#include "lib.h"


bool slave_tern_table_insert(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  assert(surr12 != 0xFFFFFFFF);
  return bin_table_insert(slave_table, surr12, arg3, mem_pool);
}

bool slave_tern_table_insert(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  return bin_table_insert(slave_table, surr12, arg3, mem_pool);
}

bool slave_tern_table_delete(BIN_TABLE *slave_table, uint32 surr12, uint32 arg3) {
  return bin_table_delete(slave_table, surr12, arg3);
}

void slave_tern_table_clear(BIN_TABLE *slave_table, STATE_MEM_POOL *mem_pool) {
  bin_table_clear(slave_table, mem_pool);
}

//////////////////////////////////////////////////////////////////////////////

uint32 slave_tern_table_size(BIN_TABLE *slave_table) {
  return bin_table_size(slave_table);
}

bool slave_tern_table_contains(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains(slave_table, surr12, arg3);
}

bool slave_tern_table_contains_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains_1(slave_table, surr12);
}

bool slave_tern_table_contains_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS
  uint32 count = master_bin_table_count_1(master_table, arg1);
  if (count > 0) {
    uint32 args2_inline[16];
    uint32 *args2 = count > 16 ? new_uint32_array(count) : args2_inline;
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, args2);
    assert(_count == count);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, args2[i]);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_contains(slave_table, surr12, arg3))
        return true;
    }
  }
  return false;
}

bool slave_tern_table_contains_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS
  uint32 count = master_bin_table_count_2(master_table, arg2);
  if (count > 0) {
    uint32 args1_inline[16];
    uint32 *args1 = count > 16 ? new_uint32_array(count) : args1_inline;
    uint32 _count = master_bin_table_restrict_2(master_table, arg2, args1);
    assert(_count == count);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, args1[i], arg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_contains(slave_table, surr12, arg3))
        return true;
    }
  }
  return false;
}

bool slave_tern_table_contains_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1) {
  //## BUG BUG BUG: THIS IS WRONG, ISN'T IT?
  return master_bin_table_contains_1(master_table, arg1);
}

bool slave_tern_table_contains_2(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2) {
  //## BUG BUG BUG: THIS IS WRONG, ISN'T IT?
  return master_bin_table_contains_2(master_table, arg2);
}

bool slave_tern_table_contains_3(BIN_TABLE *slave_table, uint32 arg3) {
  return bin_table_contains_2(slave_table, arg3);
}

uint32 slave_tern_table_lookup_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 == 0xFFFFFFFF)
    soft_fail(NULL);
  return bin_table_lookup_1(slave_table, surr12);
}

uint32 slave_tern_table_lookup_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count = master_bin_table_count_1(master_table, arg1);
  if (count > 0) {
    uint32 args2_inline[16];
    uint32 *args2 = count > 16 ? new_uint32_array(count) : args2_inline;
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, args2);
    assert(_count == count);
    uint32 arg2 = -1;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 anArg2 = args2[i];
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, anArg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_contains(slave_table, surr12, arg3))
        if (arg2 == -1)
          arg2 = anArg2;
        else
          soft_fail(NULL);
    }
    if (arg2 != -1)
      return arg2;
  }
  soft_fail(NULL);
}

uint32 slave_tern_table_lookup_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count = master_bin_table_count_2(master_table, arg2);
  if (count > 0) {
    uint32 args1_inline[16];
    uint32 *args1 = count > 16 ? new_uint32_array(count) : args1_inline;
    uint32 _count = master_bin_table_restrict_2(master_table, arg2, args1);
    assert(_count == count);
    uint32 arg1 = -1;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 anArg1 = args1[i];
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, anArg1, arg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_contains(slave_table, surr12, arg3))
        if (arg1 == -1)
          arg1 = anArg1;
        else
          soft_fail(NULL);
    }
    if (arg1 != -1)
      return arg1;
  }
  soft_fail(NULL);
}

uint32 slave_tern_table_count_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF ? bin_table_count_1(slave_table, surr12) : 0;
}

uint32 slave_tern_table_count_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count13 = 0;
  uint32 count1 = master_bin_table_count_1(master_table, arg1);
  if (count1 > 0) {
    uint32 args2_inline[16];
    uint32 *args2 = count1 > 16 ? new_uint32_array(count1) : args2_inline;
    //## COULD BE MORE EFFICIENT IF WE HAD A MasterBinaryTable.restrict1(uint32, uint32[], uint32[]) HERE
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, args2);
    assert(_count == count1);
    for (uint32 i=0 ; i < count1 ; i++) {
      uint32 arg2 = args2[i];
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_contains(slave_table, surr12, arg3))
        count13++;
    }
  }
  return count13;
}

uint32 slave_tern_table_count_23(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count23 = 0;
  uint32 count3 = bin_table_count_2(slave_table, arg3);
  if (count3 > 0) {
    uint32 surrs12_inline[16];
    uint32 *surrs12 = count3 > 16 ? new_uint32_array(count3) : surrs12_inline;
    uint32 _count = bin_table_restrict_2(slave_table, arg3, surrs12);
    assert(_count == count3);
    for (uint32 i=0 ; i < count3 ; i++) {
      uint32 surr12 = surrs12[i];
      if (master_bin_table_get_arg_2(master_table, surr12) == arg2)
        count23++;
    }
  }
  return count23;
}

uint32 slave_tern_table_count_1(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1) {
  uint32 fullCount = 0;
  uint32 count1 = master_bin_table_count_1(master_table, arg1);
  if (count1 > 0) {
    uint32 args2_inline[16];
    uint32 *args2 = count1 > 16 ? new_uint32_array(count1) : args2_inline;
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, args2);
    assert(_count == count1);
    for (uint32 i=0 ; i < count1 ; i++) {
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, args2[i]);
      assert(surr12 != 0xFFFFFFFF);
      fullCount += bin_table_count_1(slave_table, surr12);
    }
  }
  return fullCount;
}

uint32 slave_tern_table_count_2(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2) {
  uint32 fullCount = 0;
  uint32 count2 = master_bin_table_count_2(master_table, arg2);
  if (count2 > 0) {
    uint32 args1_inline[16];
    uint32 *args1 = count2 > 16 ? new_uint32_array(count2) : args1_inline;
    uint32 _count = master_bin_table_restrict_2(master_table, arg2, args1);
    assert(_count == count2);
    for (uint32 i=0 ; i < count2 ; i++) {
      uint32 surr12 = master_bin_table_lookup_surrogate(master_table, args1[i], arg2);
      assert(surr12 != 0xFFFFFFFF);
      fullCount += bin_table_count_1(slave_table, surr12);
    }
  }
  return fullCount;
}

uint32 slave_tern_table_count_3(BIN_TABLE *slave_table, uint32 arg3) {
  return bin_table_count_2(slave_table, arg3);
}

////////////////////////////////////////////////////////////////////////////////

bool slave_tern_table_cols_12_are_key(BIN_TABLE *slave_table) {
  return bin_table_col_1_is_key(slave_table);
}

bool slave_tern_table_col_3_is_key(BIN_TABLE *slave_table) {
  return bin_table_col_2_is_key(slave_table);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_copy_to(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  BIN_TABLE_ITER iter;
  bin_table_iter_init(slave_table, &iter);
  while (!bin_table_iter_is_out_of_range(&iter)) {
    uint32 surr12 = bin_table_iter_get_1(&iter);
    uint32 arg1 = master_bin_table_get_arg_1(master_table, surr12);
    uint32 arg2 = master_bin_table_get_arg_2(master_table, surr12);
    uint32 arg3 = bin_table_iter_get_2(&iter);
    OBJ obj1 = surr_to_obj_1(store_1, arg1);
    OBJ obj2 = surr_to_obj_2(store_2, arg2);
    OBJ obj3 = surr_to_obj_3(store_3, arg3);
    append(*strm_1, obj1);
    append(*strm_2, obj2);
    append(*strm_3, obj3);
    bin_table_iter_move_forward(&iter);
  }
}

void slave_tern_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  uint32 count = bin_table_size(slave_table);
  uint32 idx = 0;
  BIN_TABLE_ITER iter;
  bin_table_iter_init(slave_table, &iter);
  while (!bin_table_iter_is_out_of_range(&iter)) {
    uint32 surr12 = bin_table_iter_get_1(&iter);
    uint32 arg1 = master_bin_table_get_arg_1(master_table, surr12);
    uint32 arg2 = master_bin_table_get_arg_2(master_table, surr12);
    uint32 arg3 = bin_table_iter_get_2(&iter);
    OBJ obj1 = surr_to_obj_1(store_1, arg1);
    OBJ obj2 = surr_to_obj_2(store_2, arg2);
    OBJ obj3 = surr_to_obj_3(store_3, arg3);

    assert(
      (idx1 == 0 && idx2 == 1 && idx3 == 2) ||
      (idx1 == 0 && idx2 == 2 && idx3 == 1) ||
      (idx1 == 1 && idx2 == 0 && idx3 == 2) ||
      (idx1 == 1 && idx2 == 2 && idx3 == 0) ||
      (idx1 == 2 && idx2 == 0 && idx3 == 1) ||
      (idx1 == 2 && idx2 == 1 && idx3 == 0)
    );

    write_str(write_state, "\n    ");
    write_obj(write_state, idx1 == 0 ? obj1 : (idx2 == 0 ? obj2 : obj3));
    write_str(write_state, ", ");
    write_obj(write_state, idx1 == 1 ? obj1 : (idx2 == 1 ? obj2 : obj3));
    write_str(write_state, ", ");
    write_obj(write_state, idx1 == 2 ? obj1 : (idx2 == 2 ? obj2 : obj3));
    if (++idx != count)
      write_str(write_state, ";");

    bin_table_iter_move_forward(&iter);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_iter_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER *iter) {
  iter->slave_table = slave_table;
  master_bin_table_iter_init(master_table, &iter->master_iter);
  while (!master_bin_table_iter_is_out_of_range(&iter->master_iter)) {
    uint32 surr12 = master_bin_table_iter_get_surr(&iter->master_iter);
    bin_table_iter_1_init(slave_table, &iter->slave_iter, surr12);
    if (!bin_table_iter_1_is_out_of_range(&iter->slave_iter))
      return;
    master_bin_table_iter_move_forward(&iter->master_iter);
  }
}

void slave_tern_table_iter_move_forward(SLAVE_TERN_TABLE_ITER *iter) {
  assert(!slave_tern_table_iter_is_out_of_range(iter));

  bin_table_iter_1_move_forward(&iter->slave_iter);
  while (bin_table_iter_1_is_out_of_range(&iter->slave_iter)) {
    master_bin_table_iter_move_forward(&iter->master_iter);
    if (master_bin_table_iter_is_out_of_range(&iter->master_iter))
      return;
    uint32 surr12 = master_bin_table_iter_get_surr(&iter->master_iter);
    bin_table_iter_1_init(iter->slave_table, &iter->slave_iter, surr12);
  }
}

bool slave_tern_table_iter_is_out_of_range(SLAVE_TERN_TABLE_ITER *iter) {
  return master_bin_table_iter_is_out_of_range(&iter->master_iter);
}

uint32 slave_tern_table_iter_get_1(SLAVE_TERN_TABLE_ITER *iter) {
  assert(!slave_tern_table_iter_is_out_of_range(iter));
  return master_bin_table_iter_get_1(&iter->master_iter);
}

uint32 slave_tern_table_iter_get_2(SLAVE_TERN_TABLE_ITER *iter) {
  assert(!slave_tern_table_iter_is_out_of_range(iter));
  return master_bin_table_iter_get_2(&iter->master_iter);
}

uint32 slave_tern_table_iter_get_3(SLAVE_TERN_TABLE_ITER *iter) {
  assert(!slave_tern_table_iter_is_out_of_range(iter));
  return bin_table_iter_1_get_1(&iter->slave_iter);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_iter_1_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_1 *iter, uint32 arg1) {
  iter->slave_table = slave_table;
  master_bin_table_iter_1_init(master_table, &iter->master_iter, arg1);
  while (!master_bin_table_iter_1_is_out_of_range(&iter->master_iter)) {
    uint32 surr12 = master_bin_table_iter_1_get_surr(&iter->master_iter);
    bin_table_iter_1_init(slave_table, &iter->slave_iter, surr12);
    if (!bin_table_iter_1_is_out_of_range(&iter->slave_iter))
      return;
    master_bin_table_iter_1_move_forward(&iter->master_iter);
  }
}

void slave_tern_table_iter_1_move_forward(SLAVE_TERN_TABLE_ITER_1 *iter) {
  assert(!slave_tern_table_iter_1_is_out_of_range(iter));

  bin_table_iter_1_move_forward(&iter->slave_iter);
  while (bin_table_iter_1_is_out_of_range(&iter->slave_iter)) {
    master_bin_table_iter_1_move_forward(&iter->master_iter);
    if (master_bin_table_iter_1_is_out_of_range(&iter->master_iter))
      return;
    uint32 surr12 = master_bin_table_iter_1_get_surr(&iter->master_iter);
    bin_table_iter_1_init(iter->slave_table, &iter->slave_iter, surr12);
  }
}

bool slave_tern_table_iter_1_is_out_of_range(SLAVE_TERN_TABLE_ITER_1 *iter) {
  return master_bin_table_iter_1_is_out_of_range(&iter->master_iter);
}

uint32 slave_tern_table_iter_1_get_1(SLAVE_TERN_TABLE_ITER_1 *iter) {
  assert(!slave_tern_table_iter_1_is_out_of_range(iter));
  return master_bin_table_iter_1_get_1(&iter->master_iter);
}

uint32 slave_tern_table_iter_1_get_2(SLAVE_TERN_TABLE_ITER_1 *iter) {
  assert(!slave_tern_table_iter_1_is_out_of_range(iter));
  return bin_table_iter_1_get_1(&iter->slave_iter);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_iter_2_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_2 *iter, uint32 arg2) {
  iter->slave_table = slave_table;
  master_bin_table_iter_2_init(master_table, &iter->master_iter, arg2);
  while (!master_bin_table_iter_2_is_out_of_range(&iter->master_iter)) {
    uint32 surr12 = master_bin_table_iter_2_get_surr(&iter->master_iter);
    bin_table_iter_1_init(slave_table, &iter->slave_iter, surr12);
    if (!bin_table_iter_1_is_out_of_range(&iter->slave_iter))
      return;
    master_bin_table_iter_2_move_forward(&iter->master_iter);
  }
}

void slave_tern_table_iter_2_move_forward(SLAVE_TERN_TABLE_ITER_2 *iter) {
  assert(!slave_tern_table_iter_2_is_out_of_range(iter));

  bin_table_iter_1_move_forward(&iter->slave_iter);
  while (bin_table_iter_1_is_out_of_range(&iter->slave_iter)) {
    master_bin_table_iter_2_move_forward(&iter->master_iter);
    if (master_bin_table_iter_2_is_out_of_range(&iter->master_iter))
      return;
    uint32 surr12 = master_bin_table_iter_2_get_surr(&iter->master_iter);
    bin_table_iter_1_init(iter->slave_table, &iter->slave_iter, surr12);
  }
}

bool slave_tern_table_iter_2_is_out_of_range(SLAVE_TERN_TABLE_ITER_2 *iter) {
  return master_bin_table_iter_2_is_out_of_range(&iter->master_iter);
}

uint32 slave_tern_table_iter_2_get_1(SLAVE_TERN_TABLE_ITER_2 *iter) {
  assert(!slave_tern_table_iter_2_is_out_of_range(iter));
  return master_bin_table_iter_2_get_1(&iter->master_iter);
}

uint32 slave_tern_table_iter_2_get_2(SLAVE_TERN_TABLE_ITER_2 *iter) {
  assert(!slave_tern_table_iter_2_is_out_of_range(iter));
  return bin_table_iter_1_get_1(&iter->slave_iter);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_iter_12_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_12 *iter, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(master_table, arg1, arg2);
  if (surr12 != 0xFFFFFFFF)
    bin_table_iter_1_init(slave_table, &iter->slave_iter, surr12);
  else
    bin_table_iter_1_init_empty(&iter->slave_iter);
}

void slave_tern_table_iter_12_move_forward(SLAVE_TERN_TABLE_ITER_12 *iter) {
  bin_table_iter_1_move_forward(&iter->slave_iter);
}

bool slave_tern_table_iter_12_is_out_of_range(SLAVE_TERN_TABLE_ITER_12 *iter) {
  return bin_table_iter_1_is_out_of_range(&iter->slave_iter);
}

uint32 slave_tern_table_iter_12_get_1(SLAVE_TERN_TABLE_ITER_12 *iter) {
  return bin_table_iter_1_get_1(&iter->slave_iter);
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_iter_3_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_3 *iter, uint32 arg3) {
  bin_table_iter_2_init(slave_table, &iter->slave_iter, arg3);
  iter->master_table = master_table;
}

void slave_tern_table_iter_3_move_forward(SLAVE_TERN_TABLE_ITER_3 *iter) {
  bin_table_iter_2_move_forward(&iter->slave_iter);
}

bool slave_tern_table_iter_3_is_out_of_range(SLAVE_TERN_TABLE_ITER_3 *iter) {
  return bin_table_iter_2_is_out_of_range(&iter->slave_iter);
}

uint32 slave_tern_table_iter_3_get_1(SLAVE_TERN_TABLE_ITER_3 *iter) {
  uint32 surr12 = bin_table_iter_2_get_1(&iter->slave_iter);
  return master_bin_table_get_arg_1(iter->master_table, surr12);
}

uint32 slave_tern_table_iter_3_get_2(SLAVE_TERN_TABLE_ITER_3 *iter) {
  uint32 surr12 = bin_table_iter_2_get_1(&iter->slave_iter);
  return master_bin_table_get_arg_2(iter->master_table, surr12);
}

////////////////////////////////////////////////////////////////////////////////

inline bool on_wrong_arg_1(MASTER_BIN_TABLE *table, BIN_TABLE_ITER_2 *slave_iter, uint32 arg1) {
  if (bin_table_iter_2_is_out_of_range(slave_iter))
    return false;
  uint surr12 = bin_table_iter_2_get_1(slave_iter);
  return master_bin_table_get_arg_1(table, surr12) != arg1;
}

void slave_tern_table_iter_13_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_13 *iter, uint32 arg1, uint32 arg3) {
  iter->master_table = master_table;
  iter->arg1 = arg1;
  bin_table_iter_2_init(slave_table, &iter->slave_iter, arg3);
  while (on_wrong_arg_1(master_table, &iter->slave_iter, arg1))
    bin_table_iter_2_move_forward(&iter->slave_iter);
}

void slave_tern_table_iter_13_move_forward(SLAVE_TERN_TABLE_ITER_13 *iter) {
  do
    bin_table_iter_2_move_forward(&iter->slave_iter);
  while (on_wrong_arg_1(iter->master_table, &iter->slave_iter, iter->arg1));
}

bool slave_tern_table_iter_13_is_out_of_range(SLAVE_TERN_TABLE_ITER_13 *iter) {
  return bin_table_iter_2_is_out_of_range(&iter->slave_iter);
}

uint32 slave_tern_table_iter_13_get_1(SLAVE_TERN_TABLE_ITER_13 *iter) {
  uint32 surr12 = bin_table_iter_2_get_1(&iter->slave_iter);
  return master_bin_table_get_arg_1(iter->master_table, surr12);
}

////////////////////////////////////////////////////////////////////////////////

inline bool on_wrong_arg_2(MASTER_BIN_TABLE *table, BIN_TABLE_ITER_2 *slave_iter, uint32 arg2) {
  if (bin_table_iter_2_is_out_of_range(slave_iter))
    return false;
  uint surr12 = bin_table_iter_2_get_1(slave_iter);
  return master_bin_table_get_arg_2(table, surr12) != arg2;
}

void slave_tern_table_iter_23_init(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, SLAVE_TERN_TABLE_ITER_23 *iter, uint32 arg2, uint32 arg3) {
  iter->master_table = master_table;
  iter->arg2 = arg2;
  bin_table_iter_2_init(slave_table, &iter->slave_iter, arg3);
  while (on_wrong_arg_2(master_table, &iter->slave_iter, arg2))
    bin_table_iter_2_move_forward(&iter->slave_iter);
}

void slave_tern_table_iter_23_move_forward(SLAVE_TERN_TABLE_ITER_23 *iter) {
  do
    bin_table_iter_2_move_forward(&iter->slave_iter);
  while (on_wrong_arg_2(iter->master_table, &iter->slave_iter, iter->arg2));
}

bool slave_tern_table_iter_23_is_out_of_range(SLAVE_TERN_TABLE_ITER_23 *iter) {
  return bin_table_iter_2_is_out_of_range(&iter->slave_iter);
}

uint32 slave_tern_table_iter_23_get_1(SLAVE_TERN_TABLE_ITER_23 *iter) {
  uint32 surr12 = bin_table_iter_2_get_1(&iter->slave_iter);
  return master_bin_table_get_arg_1(iter->master_table, surr12);
}
