#include "lib.h"


inline uint64 pack_sym_args(uint32 arg1, uint32 arg2) {
  return arg1 <= arg2 ? pack(arg1, arg2) : pack(arg2, arg1);
}

////////////////////////////////////////////////////////////////////////////////

void sym_bin_table_aux_init(SYM_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_bit_map_init(&table_aux->bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_u64_init(&table_aux->insertions);
  table_aux->clear = false;
}

void sym_bin_table_aux_reset(SYM_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_u64_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void sym_bin_table_aux_insert(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->insertions, pack_sym_args(arg1, arg2));
}

void sym_bin_table_aux_delete(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_sym_args(arg1, arg2));
}

void sym_bin_table_aux_delete_1(SYM_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

void sym_bin_table_aux_clear(SYM_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void sym_bin_table_aux_build_insertion_bitmap(SYM_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *args_array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = args_array[i];
      uint32 minor_arg = get_low_32(args);
      uint32 major_arg = get_high_32(args);
      col_update_bit_map_set(&table_aux->bit_map, minor_arg, mem_pool);
      col_update_bit_map_set(&table_aux->bit_map, major_arg, mem_pool);
    }
  }
}

void sym_bin_table_aux_apply_deletions(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    uint32 size = sym_bin_table_size(table);
    if (size > 0) {
      if (remove != NULL) {
        if (table_aux->insertions.count == 0) {
          remove(store, 0xFFFFFFFF, mem_pool);
        }
        else {
          sym_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
          uint32 read = 0;
          for (uint32 arg=0 ; read < size ; arg++) {
            uint32 count = sym_bin_table_count(table, arg);
            if (count > 0) {
              read += count;
              if (!col_update_bit_map_is_set(&table_aux->bit_map, arg))
                remove(store, arg, mem_pool);
            }
          }
          col_update_bit_map_clear(&table_aux->bit_map);
        }
      }

      sym_bin_table_clear(table, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0;
    bool bit_map_built = !has_insertions;

    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint64 args = array[i];
        uint32 minor_arg = get_low_32(args);
        uint32 major_arg = get_high_32(args);
        assert(minor_arg <= major_arg);
        if (sym_bin_table_delete(table, minor_arg, major_arg) && remove != NULL) {
          if (sym_bin_table_count(table, minor_arg) == 0) {
            if (!bit_map_built) {
              sym_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
              bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, minor_arg))
              remove(store, minor_arg, mem_pool);
          }
          if (major_arg != minor_arg && sym_bin_table_count(table, major_arg) == 0) {
            if (!bit_map_built) {
              sym_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
              bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, major_arg))
              remove(store, major_arg, mem_pool);
          }
        }
      }
    }

    count = table_aux->deletions_1.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 arg = array[i];

        if (remove != NULL) {
          uint32 arg_count = sym_bin_table_count(table, arg);
          uint32 read = 0;
          while (read < arg_count) {
            uint32 buffer[64];
            UINT32_ARRAY other_args_array = sym_bin_table_range_restrict(table, arg, read, buffer, 64);
            read += other_args_array.size;
            for (uint32 i1=0 ; i1 < other_args_array.size ; i1++) {
              uint32 other_arg = other_args_array.array[i1];
              assert(sym_bin_table_count(table, other_arg) > 0);
              if (other_arg != arg && sym_bin_table_count(table, other_arg) == 1) {
                if (!bit_map_built) {
                  sym_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
                  bit_map_built = true;
                }
                if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, other_arg))
                  remove(store, other_arg, mem_pool);
              }
            }
          }
        }

        sym_bin_table_delete_1(table, arg);
        assert(sym_bin_table_count(table, arg) == 0);
        if (remove != NULL) {
          if (!bit_map_built) {
            sym_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
            bit_map_built = true;
          }
          if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, arg))
            remove(store, arg, mem_pool);
        }
      }
    }

    if (has_insertions && bit_map_built)
      col_update_bit_map_clear(&table_aux->bit_map);
  }
}

void sym_bin_table_aux_apply_insertions(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint64 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint64 args = array[i];
      uint32 minor_arg = get_low_32(args);
      uint32 major_arg = get_high_32(args);
      assert(minor_arg <= major_arg);
      sym_bin_table_insert(table, minor_arg, major_arg, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool sym_bin_table_aux_was_inserted(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count == 0)
    return false;
  uint64 args = pack_sym_args(arg1, arg2);
  return queue_u64_contains(&table_aux->insertions, args);
}

bool sym_bin_table_aux_contains(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint64 args = pack_sym_args(arg1, arg2);
  if (queue_u64_contains(&table_aux->insertions, args))
    return true;

  if (!sym_bin_table_contains(table, arg1, arg2))
    return false;

  if (queue_u64_contains(&table_aux->deletions, args))
    return false;

  QUEUE_U32 *deletions_1 = &table_aux->deletions_1;
  uint32 count = deletions_1->count;
  if (count > 0) {
    uint32 *array = deletions_1->array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 del_arg = array[i];
      if (del_arg == arg1 || del_arg == arg2)
        return false;
    }
  }

  return true;
}

bool sym_bin_table_aux_contains_1(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, uint32 arg) {
  //## TODO: IMPLEMENT FOR REAL
  //## SO FAR THIS IS ONLY USED INSIDE unary_table_aux_check_foreign_key_sym_bin_table_backward(..),
  //## SO IT MIGHT BE POSSIBLE TO SAVE SOME REPEATED WORK BY CHECKING ALL THE ARGUMENTS TOGETHER

  QUEUE_U64 *insertions = &table_aux->insertions;
  uint32 count = insertions->count;
  for (uint32 i=0 ; i < count ; i++) {
    uint64 packed_args = insertions->array[i];
    uint32 arg1 = unpack_arg1(packed_args);
    uint32 arg2 = unpack_arg2(packed_args);
    if (arg1 == arg || arg2 == arg)
      return true;
  }

  uint32 left = sym_bin_table_count(table, arg);
  if (left == 0)
    return false;

  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
  COL_UPDATE_BIT_MAP *bit_map = &table_aux->bit_map;
  assert(!col_update_bit_map_is_dirty(bit_map));

  QUEUE_U64 *deletions = &table_aux->deletions;
  count = deletions->count;
  for (uint32 i=0 ; i < count ; i++) {
    uint64 packed_args = deletions->array[i];
    uint32 arg1 = unpack_arg1(packed_args);
    uint32 arg2 = unpack_arg2(packed_args);
    if (arg1 == arg) {
      if (!col_update_bit_map_check_and_set(bit_map, arg2, mem_pool))
        if (--left == 0) {
          col_update_bit_map_clear(bit_map);
          return false;
        }
    }
    else if (arg2 == arg) {
      if (!col_update_bit_map_check_and_set(bit_map, arg1, mem_pool))
        if (--left == 0) {
          col_update_bit_map_clear(bit_map);
          return false;
        }
    }
  }

  QUEUE_U32 *deletions_1 = &table_aux->deletions_1;
  count = deletions_1->count;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 del_arg = deletions_1->array[i];
    if (del_arg == arg) {
      col_update_bit_map_clear(bit_map);
      return false;
    }
    if (sym_bin_table_contains(table, del_arg, arg))
      if (!col_update_bit_map_check_and_set(bit_map, del_arg, mem_pool))
        if (--left == 0) {
          col_update_bit_map_clear(bit_map);
          return false;
        }
  }

  col_update_bit_map_clear(bit_map);

  assert(left > 0);
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool sym_bin_table_aux_check_foreign_key_unary_table_forward(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint64 *packed_args = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++) {
      uint64 args = packed_args[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      if (!unary_table_aux_contains(target_table, target_table_aux, arg1) || !unary_table_aux_contains(target_table, target_table_aux, arg2))
        return false;
    }
  }
  return true;
}

bool sym_bin_table_aux_check_foreign_key_semisym_tern_table_backward(BIN_TABLE *table, SYM_BIN_TABLE_AUX *table_aux, TERN_TABLE *src_table, SEMISYM_TERN_TABLE_AUX *src_table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  if (table_aux->clear) {
    uint32 ins_count = table_aux->insertions.count;
    if (ins_count == 0)
      return semisym_tern_table_aux_is_empty(src_table, src_table_aux);

    STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
    queue_u64_deduplicate(&table_aux->insertions);
    uint64 *args_array = table_aux->insertions.array;
    uint32 found = 0;
    for (uint32 i=0 ; i < ins_count ; i++) {
      uint64 args = args_array[i];
      uint32 arg1 = unpack_arg1(args);
      uint32 arg2 = unpack_arg2(args);
      found += semisym_tern_table_aux_count_12(src_table, src_table_aux, arg1, arg2);
    }
    uint32 src_size = semisym_tern_table_aux_size(src_table, src_table_aux);
    assert(found <= src_size);
    return src_size == found;
  }
  else {
    uint32 num_dels = table_aux->deletions.count;
    if (num_dels > 0) {
      uint64 *args_array = table_aux->deletions.array;
      for (uint32 i=0 ; i < num_dels ; i++) {
        uint64 args = args_array[i];
        uint32 arg1 = unpack_arg1(args);
        uint32 arg2 = unpack_arg2(args);
        if (!sym_bin_table_aux_was_inserted(table, table_aux, arg1, arg2))
          if (semisym_tern_table_aux_contains_12(src_table, src_table_aux, arg1, arg2))
            return false;
      }
    }

    uint32 num_dels_1 = table_aux->deletions_1.count;
    if (num_dels_1 > 0) {
      uint32 *del_args = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < num_dels_1 ; i++) {
        uint32 del_arg = del_args[i];
        uint32 count = sym_bin_table_count(table, del_arg);
        uint32 read = 0;
        while (read < count) {
          uint32 buffer[64];
          UINT32_ARRAY array = sym_bin_table_range_restrict(table, del_arg, read, buffer, 64);
          read += array.size;
          for (uint32 i=0 ; i < array.size ; i++) {
            uint32 other_arg = array.array[i];
            if (!sym_bin_table_aux_was_inserted(table, table_aux, del_arg, other_arg))
              if (semisym_tern_table_aux_contains_12(src_table, src_table_aux, del_arg, other_arg))
                return false;
          }
        }
      }
    }

    return true;
  }
}
