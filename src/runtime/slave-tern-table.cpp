#include "lib.h"


bool slave_tern_table_insert(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  uint32 surr12 = master_bin_table_lookup_surr(master_table, arg1, arg2);
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
  uint32 surr12 = master_bin_table_lookup_surr(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains(slave_table, surr12, arg3);
}

bool slave_tern_table_contains_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surr(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF && bin_table_contains_1(slave_table, surr12);
}

bool slave_tern_table_contains_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS
  uint32 count = master_bin_table_count_1(master_table, arg1);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *arg2s_unused = count > 512 ? new_uint32_array(2 * count) : inline_array;
    uint32 *surrs = arg2s_unused + count;
    //## BAD BAD BAD: ALLOCATING SPACE FOR THE VALUES OF THE SECOND ARGUMENTS EVEN THOUGH THEY'RE NOT USED
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, arg2s_unused, surrs);
    assert(_count == count);
    for (uint32 i=0 ; i < count ; i++)
      if (bin_table_contains(slave_table, surrs[i], arg3))
        return true;
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
      uint32 surr12 = master_bin_table_lookup_surr(master_table, args1[i], arg2);
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
  uint32 surr12 = master_bin_table_lookup_surr(master_table, arg1, arg2);
  if (surr12 == 0xFFFFFFFF)
    soft_fail(NULL);
  return bin_table_lookup_1(slave_table, surr12);
}

uint32 slave_tern_table_lookup_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count = master_bin_table_count_1(master_table, arg1);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *arg2s = count > 512 ? new_uint32_array(2 * count) : inline_array;
    uint32 *surrs = arg2s + count;
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, arg2s, surrs);
    assert(_count == count);
    uint32 arg2 = 0xFFFFFFFF;
    for (uint32 i=0 ; i < count ; i++) {
      if (bin_table_contains(slave_table, surrs[i], arg3))
        if (arg2 == 0xFFFFFFFF)
          arg2 = arg2s[i];
        else
          soft_fail(NULL);
    }
    if (arg2 != 0xFFFFFFFF)
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
      uint32 curr_arg_1 = args1[i];
      uint32 surr12 = master_bin_table_lookup_surr(master_table, curr_arg_1, arg2);
      assert(surr12 != 0xFFFFFFFF);
      if (bin_table_contains(slave_table, surr12, arg3))
        if (arg1 == -1)
          arg1 = curr_arg_1;
        else
          soft_fail(NULL);
    }
    if (arg1 != -1)
      return arg1;
  }
  soft_fail(NULL);
}

uint32 slave_tern_table_count_12(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surr(master_table, arg1, arg2);
  return surr12 != 0xFFFFFFFF ? bin_table_count_1(slave_table, surr12) : 0;
}

uint32 slave_tern_table_count_13(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg1, uint32 arg3) {
  //## HERE WE SHOULD EVALUATE ALL THE POSSIBLE EXECUTION PATHS, AND ALSO CONSIDER AN INDEX
  uint32 count13 = 0;
  uint32 count1 = master_bin_table_count_1(master_table, arg1);
  if (count1 > 0) {
    uint32 inline_array[1024];
    uint32 *arg2s_unused = count1 > 512 ? new_uint32_array(2 * count1) : inline_array;
    uint32 *surrs = arg2s_unused + count1;
    //## BAD BAD BAD: ALLOCATING SPACE FOR THE VALUES OF THE SECOND ARGUMENTS EVEN THOUGH THEY'RE NOT USED
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, arg2s_unused, surrs);
    assert(_count == count1);
    for (uint32 i=0 ; i < count1 ; i++) {
      if (bin_table_contains(slave_table, surrs[i], arg3))
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
  uint32 full_count = 0;
  uint32 count1 = master_bin_table_count_1(master_table, arg1);
  if (count1 > 0) {
    uint32 inline_array[1024];
    uint32 *arg2s_unused = count1 > 512 ? new_uint32_array(2 * count1) : inline_array;
    uint32 *surrs = arg2s_unused + count1;
    //## BAD BAD BAD: ALLOCATING SPACE FOR THE VALUES OF THE SECOND ARGUMENTS EVEN THOUGH THEY'RE NOT USED
    uint32 _count = master_bin_table_restrict_1(master_table, arg1, arg2s_unused, surrs);
    assert(_count == count1);
    for (uint32 i=0 ; i < count1 ; i++)
      full_count += bin_table_count_1(slave_table, surrs[i]);
  }
  return full_count;
}

uint32 slave_tern_table_count_2(MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, uint32 arg2) {
  uint32 full_count = 0;
  uint32 count2 = master_bin_table_count_2(master_table, arg2);
  if (count2 > 0) {
    uint32 args1_inline[16];
    uint32 *args1 = count2 > 16 ? new_uint32_array(count2) : args1_inline;
    uint32 _count = master_bin_table_restrict_2(master_table, arg2, args1);
    assert(_count == count2);
    for (uint32 i=0 ; i < count2 ; i++) {
      uint32 surr12 = master_bin_table_lookup_surr(master_table, args1[i], arg2);
      assert(surr12 != 0xFFFFFFFF);
      full_count += bin_table_count_1(slave_table, surr12);
    }
  }
  return full_count;
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
  uint32 count = bin_table_size(slave_table);
  uint32 read = 0;
  for (uint32 surr12=0 ; read < count ; surr12++) {
    uint32 count1 = bin_table_count_1(slave_table, surr12);
    if (count1 > 0) {
      read += count1;
      uint32 read1 = 0;
      uint32 arg1 = master_bin_table_get_arg_1(master_table, surr12);
      uint32 arg2 = master_bin_table_get_arg_2(master_table, surr12);
      do {
        uint32 buffer[64];
        UINT32_ARRAY array = bin_table_range_restrict_1(slave_table, surr12, read1, buffer, 64);
        read1 += array.size;
        for (uint32 i=0 ; i < array.size ; i++) {
          uint32 arg3 = array.array[i];

          OBJ obj1 = surr_to_obj_1(store_1, arg1);
          OBJ obj2 = surr_to_obj_2(store_2, arg2);
          OBJ obj3 = surr_to_obj_3(store_3, arg3);
          append(*strm_1, obj1);
          append(*strm_2, obj2);
          append(*strm_3, obj3);

        }
      } while (read1 < count1);
    }
  }
}

void slave_tern_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *master_table, BIN_TABLE *slave_table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  assert(
    (idx1 == 0 && idx2 == 1 && idx3 == 2) ||
    (idx1 == 0 && idx2 == 2 && idx3 == 1) ||
    (idx1 == 1 && idx2 == 0 && idx3 == 2) ||
    (idx1 == 1 && idx2 == 2 && idx3 == 0) ||
    (idx1 == 2 && idx2 == 0 && idx3 == 1) ||
    (idx1 == 2 && idx2 == 1 && idx3 == 0)
  );

  uint32 count = bin_table_size(slave_table);
  uint32 read = 0;
  for (uint32 surr12=0 ; read < count ; surr12++) {
    uint32 count1 = bin_table_count_1(slave_table, surr12);
    if (count1 > 0) {
      uint32 read1 = 0;
      uint32 arg1 = master_bin_table_get_arg_1(master_table, surr12);
      uint32 arg2 = master_bin_table_get_arg_2(master_table, surr12);
      do {
        uint32 buffer[64];
        UINT32_ARRAY array = bin_table_range_restrict_1(slave_table, surr12, read1, buffer, 64);
        read1 += array.size;
        for (uint32 i=0 ; i < array.size ; i++) {
          read++;
          uint32 arg3 = array.array[i];

          OBJ obj1 = surr_to_obj_1(store_1, arg1);
          OBJ obj2 = surr_to_obj_2(store_2, arg2);
          OBJ obj3 = surr_to_obj_3(store_3, arg3);

          write_str(write_state, "\n    ");
          write_obj(write_state, idx1 == 0 ? obj1 : (idx2 == 0 ? obj2 : obj3));
          write_str(write_state, ", ");
          write_obj(write_state, idx1 == 1 ? obj1 : (idx2 == 1 ? obj2 : obj3));
          write_str(write_state, ", ");
          write_obj(write_state, idx1 == 2 ? obj1 : (idx2 == 2 ? obj2 : obj3));
          if (read < count) //## BUG BUG BUG: THIS DOESN'T WORK IF count == 1
            write_str(write_state, ";");

        }
      } while (read1 < count1);
    }
  }
}
