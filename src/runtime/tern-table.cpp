#include "lib.h"


void tern_table_init(TERN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  master_bin_table_init(&table->master, mem_pool);
  bin_table_init(&table->slave, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

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

// void tern_table_clear(TERN_TABLE *table, STATE_MEM_POOL *mem_pool) {
//   master_bin_table_clear(&table->master, mem_pool);
//   bin_table_clear(&table->slave, mem_pool);
// }

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

static bool tern_table_cols_13_or_23_are_key(TERN_TABLE *table, bool key_13) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->slave);
  COL_UPDATE_BIT_MAP bit_map;
  col_update_bit_map_init(&bit_map);

  uint32 size = bin_table_size(&table->slave);
  uint64 *slots = master_bin_table_slots(&table->master);

  uint32 read = 0;
  for (uint32 arg3=0 ; read < size ; arg3++) {
    uint32 count3 = bin_table_count_2(&table->slave, arg3);
    if (count3 > 0) {
      read += count3;
      uint32 buffer[64];
      uint32 read3 = 0;
      while (read3 < count3) {
        UINT32_ARRAY array = bin_table_range_restrict_2(&table->slave, arg3, read3, buffer, 64);
        read3 += array.size;
        for (uint32 i=0 ; i < array.size ; i++) {
          uint32 assoc_surr = array.array[i];
          uint64 slot = slots[assoc_surr];
          assert(unpack_arg1(slot) == master_bin_table_get_arg_1(&table->master, assoc_surr));
          assert(unpack_arg2(slot) == master_bin_table_get_arg_2(&table->master, assoc_surr));
          uint32 other_arg = key_13 ? unpack_arg1(slot) : unpack_arg2(slot);
          if (col_update_bit_map_check_and_set(&bit_map, other_arg, mem_pool))
            return false;
        }
      }
      col_update_bit_map_clear(&bit_map);
    }
  }
  col_update_bit_map_release(&bit_map, mem_pool);
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool tern_table_cols_12_are_key(TERN_TABLE *table) {
  return bin_table_col_1_is_key(&table->slave);
}

bool tern_table_cols_13_are_key(TERN_TABLE *table) {
  return tern_table_cols_13_or_23_are_key(table, true);
}

bool tern_table_cols_23_are_key(TERN_TABLE *table) {
  return tern_table_cols_13_or_23_are_key(table, false);
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

////////////////////////////////////////////////////////////////////////////////

void tern_table_iter_13_init(TERN_TABLE *table, TERN_TABLE_ITER_13_OR_23 *iter, uint32 arg1, uint32 arg3) {
  uint32 count1 = master_bin_table_count_1(&table->master, arg1);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  iter->offset = count1 == 0 | count3 == 3 ? 0xFFFFFFFF : 0;
}

bool tern_table_iter_13_done(TERN_TABLE_ITER_13_OR_23 *iter) {
  return iter->offset == 0xFFFFFFFF;
}

uint32 tern_table_iter_13_read(TERN_TABLE *table, TERN_TABLE_ITER_13_OR_23 *iter, uint32 arg1, uint32 arg3, uint32 *arg2s, uint32 capacity) {
  assert(capacity == 64);
  assert(!tern_table_iter_13_done(iter));

  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  uint32 write_idx = 0;

  uint32 read3 = iter->offset;
  assert(read3 == 0 || read3 >= 64);

  while (read3 < count3) {
    uint32 buffer[64];
    UINT32_ARRAY array = bin_table_range_restrict_2(&table->slave, arg3, read3, buffer, 64);
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 assoc_surr = array.array[i];
      read3++;
      uint32 curr_arg1 = master_bin_table_get_arg_1(&table->master, assoc_surr);
      if (curr_arg1 == arg1) {
        arg2s[write_idx++] = master_bin_table_get_arg_2(&table->master, assoc_surr);
        if (write_idx == capacity) {
          iter->offset = read3;
          return capacity;
        }
      }
    }
  }

  iter->offset = 0xFFFFFFFF;
  return write_idx;
}

void tern_table_iter_23_init(TERN_TABLE *table, TERN_TABLE_ITER_13_OR_23 *iter, uint32 arg2, uint32 arg3) {
  uint32 count2 = master_bin_table_count_2(&table->master, arg2);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  iter->offset = count2 == 0 | count3 == 3 ? 0xFFFFFFFF : 0;
}

bool tern_table_iter_23_done(TERN_TABLE_ITER_13_OR_23 *iter) {
  return iter->offset == 0xFFFFFFFF;
}

uint32 tern_table_iter_23_read(TERN_TABLE *table, TERN_TABLE_ITER_13_OR_23 *iter, uint32 arg2, uint32 arg3, uint32 *arg1s, uint32 capacity) {
  assert(capacity == 64);
  assert(!tern_table_iter_23_done(iter));

  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  uint32 write_idx = 0;

  uint32 read3 = iter->offset;
  assert(read3 == 0 || read3 >= 64);

  while (read3 < count3) {
    uint32 buffer[64];
    UINT32_ARRAY array = bin_table_range_restrict_2(&table->slave, arg3, read3, buffer, 64);
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 assoc_surr = array.array[i];
      read3++;
      uint32 curr_arg2 = master_bin_table_get_arg_2(&table->master, assoc_surr);
      if (curr_arg2 == arg2) {
        arg1s[write_idx++] = master_bin_table_get_arg_1(&table->master, assoc_surr);
        if (write_idx == capacity) {
          iter->offset = read3;
          return capacity;
        }
      }
    }
  }

  iter->offset = 0xFFFFFFFF;
  return write_idx;
}
