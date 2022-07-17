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

void sym_bin_table_aux_prepare(SYM_BIN_TABLE_AUX *table_aux) {

}