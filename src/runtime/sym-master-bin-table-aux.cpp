#include "lib.h"


bool is_locked(uint64 slot);

uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *table, uint32 last_idx);
void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *table, uint32 next_free);

bool master_bin_table_lock_surr(MASTER_BIN_TABLE *, uint32 surr);
bool master_bin_table_unlock_surr(MASTER_BIN_TABLE *, uint32 surr);

bool sym_master_bin_table_insert_with_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool);

uint32 master_bin_table_aux_lock_surrs(MASTER_BIN_TABLE *, QUEUE_U32 *);
void master_bin_table_aux_unlock_surrs(MASTER_BIN_TABLE *, QUEUE_U32 *);

////////////////////////////////////////////////////////////////////////////////

inline void sort_args(uint32 &arg1, uint32 &arg2) {
  if (arg1 > arg2) {
    uint32 tmp = arg1;
    arg1 = arg2;
    arg2 = tmp;
  }
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_init(SYM_MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  col_update_bit_map_init(&table_aux->bit_map);
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_3u32_init(&table_aux->insertions);
  queue_u32_init(&table_aux->locked_surrs);
  trns_map_surr_surr_surr_init(&table_aux->reserved_surrs);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void sym_master_bin_table_aux_reset(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_3u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->locked_surrs);
  trns_map_surr_surr_surr_clear(&table_aux->reserved_surrs);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

//## BUG BUG BUG: SHOULDN'T WE HAVE A sym_master_bin_table_aux_partial_reset(..) HERE?
//## MAYBE IT'S NOT EVEN GENERATED IN THE CODE. TEST THIS

// void sym_master_bin_table_aux_partial_reset(MASTER_BIN_TABLE_AUX *table_aux) {
//   throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
// }

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_clear(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void sym_master_bin_table_aux_delete(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  if (sym_master_bin_table_contains(table, arg1, arg2))
    queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void sym_master_bin_table_aux_delete_1(SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

uint32 sym_master_bin_table_aux_insert(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);

  uint32 surr = sym_master_bin_table_lookup_surr(table, arg1, arg2);
  if (surr != 0xFFFFFFFF) {
    queue_u32_insert(&table_aux->locked_surrs, surr);
    return surr;
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 curr_arg1 = (*ptr)[0];
      uint32 curr_arg2 = (*ptr)[1];
      if (curr_arg1 == arg1 & curr_arg2 == arg2) {
        surr = (*ptr)[2];
        return surr;
      }
      ptr++;
    }
  }

  surr = trns_map_surr_surr_surr_extract(&table_aux->reserved_surrs, arg1, arg2);
  if (surr == 0xFFFFFFFF) {
    surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
    table_aux->last_surr = surr;
  }

  queue_3u32_insert(&table_aux->insertions, arg1, arg2, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

uint32 sym_master_bin_table_aux_lookup_surr(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);

  uint32 surr = sym_master_bin_table_lookup_surr(table, arg1, arg2);
  if (surr != 0xFFFFFFFF)
    return surr;

  //## BAD BAD BAD: IMPLEMENT FOR REAL
  uint32 count = table_aux->insertions.count;
  uint32 (*ptr)[3] = table_aux->insertions.array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 curr_arg1 = (*ptr)[0];
    uint32 curr_arg2 = (*ptr)[1];
    if (curr_arg1 == arg1 & curr_arg2 == arg2)
      return (*ptr)[2];
    ptr++;
  }

  surr = trns_map_surr_surr_surr_lookup(&table_aux->reserved_surrs, arg1, arg2);
  if (surr != 0xFFFFFFFF)
    return surr;

  surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
  table_aux->last_surr = surr;
  trns_map_surr_surr_surr_insert_new(&table_aux->reserved_surrs, arg1, arg2, surr);
  return surr;
}

////////////////////////////////////////////////////////////////////////////////

static void sym_master_bin_table_aux_build_insertion_bitmap(SYM_MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  assert(!col_update_bit_map_is_dirty(&table_aux->bit_map));

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 minor_arg = (*ptr)[0];
      uint32 major_arg = (*ptr)[1];
      col_update_bit_map_set(&table_aux->bit_map, minor_arg, mem_pool);
      col_update_bit_map_set(&table_aux->bit_map, major_arg, mem_pool);
      ptr++;
    }
  }
}

void sym_master_bin_table_aux_apply_surrs_acquisition(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  //## I'M ASSUMING THAT FOREIGN KEY CHECKS WILL BE ENOUGH TO AVOID THE
  //## UNUSED SURROGATE ALLOCATIONS, BUT I'M NOT SURE
  assert(trns_map_surr_surr_surr_is_empty(&table_aux->reserved_surrs));

  // Removing from the surrogates already reserved for newly inserted tuples
  // from the list of free ones, so we can append to that list while deleting
  uint32 ins_count = table_aux->insertions.count;
  if (ins_count != 0) {
#ifndef NDEBUG
    if (table_aux->last_surr == 0xFFFFFFFF) {
      uint32 (*ptr)[3] = table_aux->insertions.array;
      for (uint32 i=0 ; i < ins_count ; i++) {
        uint32 arg1 = (*ptr)[0];
        uint32 arg2 = (*ptr)[1];
        uint32 surr = (*ptr)[2];
        assert(master_bin_table_contains(table, arg1, arg2));
        assert(master_bin_table_lookup_surr(table, arg1, arg2) == surr);
        ptr++;
      }
    }
#endif
    uint32 next_surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);
    master_bin_table_set_next_free_surr(table, next_surr);
  }
}

void sym_master_bin_table_aux_apply_deletions(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store, STATE_MEM_POOL *mem_pool) {
  //## I'M ASSUMING THAT FOREIGN KEY CHECKS WILL BE ENOUGH TO AVOID THE
  //## UNUSED SURROGATE ALLOCATIONS, BUT I'M NOT SURE
  assert(trns_map_surr_surr_surr_is_empty(&table_aux->reserved_surrs));

  bool locks_applied = table_aux->locked_surrs.count == 0;

  //## FROM HERE ON THIS IS BASICALLY IDENTICAL TO sym_bin_table_aux_apply_deletions(..). The most important difference is the locking of the surrogates

  if (table_aux->clear) {
    uint32 size = sym_master_bin_table_size(table);
    if (size > 0) {
      if (remove != NULL) {
        if (table_aux->insertions.count == 0) {
          remove(store, 0xFFFFFFFF, mem_pool);
        }
        else {
          sym_master_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
          uint32 read = 0;
          for (uint32 arg=0 ; read < size ; arg++) {
            uint32 count = sym_master_bin_table_count(table, arg);
            if (count > 0) {
              read += count;
              if (!col_update_bit_map_is_set(&table_aux->bit_map, arg))
                remove(store, arg, mem_pool);
            }
          }
          col_update_bit_map_clear(&table_aux->bit_map);
        }
      }

      uint32 highest_locked_surr = 0xFFFFFFFF;
      if (!locks_applied) {
        highest_locked_surr = master_bin_table_aux_lock_surrs(table, &table_aux->locked_surrs);
        locks_applied = true;
      }

      sym_master_bin_table_clear(table, highest_locked_surr, mem_pool);
    }
  }
  else {
    bool has_insertions = table_aux->insertions.count > 0;
    bool bit_map_built = !has_insertions;

    uint32 del_count = table_aux->deletions.count;
    uint32 del_1_count = table_aux->deletions_1.count;

    if (del_count > 0 | del_1_count > 0) {
      if (!locks_applied) {
        master_bin_table_aux_lock_surrs(table, &table_aux->locked_surrs);
        locks_applied = true;
      }
    }

    if (del_count > 0) {
      uint64 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < del_count ; i++) {
        uint64 args = array[i];
        uint32 minor_arg = get_low_32(args);
        uint32 major_arg = get_high_32(args);
        assert(minor_arg <= major_arg);
        if (sym_master_bin_table_delete(table, minor_arg, major_arg) && remove != NULL) {
          if (sym_master_bin_table_count(table, minor_arg) == 0) {
            if (!bit_map_built) {
              sym_master_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
              bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, minor_arg))
              remove(store, minor_arg, mem_pool);
          }
          if (major_arg != minor_arg && sym_master_bin_table_count(table, major_arg) == 0) {
            if (!bit_map_built) {
              sym_master_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
              bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, major_arg))
              remove(store, major_arg, mem_pool);
          }
        }
      }
    }

    if (del_1_count > 0) {
      uint32 *array = table_aux->deletions_1.array;
      for (uint32 i=0 ; i < del_1_count ; i++) {
        uint32 arg = array[i];

        if (remove != NULL) {
          uint32 arg_count = sym_master_bin_table_count(table, arg);
          uint32 read = 0;
          while (read < arg_count) {
            uint32 buffer[64];
            UINT32_ARRAY other_args_array = sym_master_bin_table_range_restrict(table, arg, read, buffer, 64);
            read += other_args_array.size;
            for (uint32 i1=0 ; i1 < other_args_array.size ; i1++) {
              uint32 other_arg = other_args_array.array[i1];
              assert(sym_master_bin_table_count(table, other_arg) > 0);
              if (other_arg != arg && sym_master_bin_table_count(table, other_arg) == 1) {
                if (!bit_map_built) {
                  sym_master_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
                  bit_map_built = true;
                }
                if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, other_arg))
                  remove(store, other_arg, mem_pool);
              }
            }
          }
        }

        if (sym_master_bin_table_contains_1(table, arg)) {
          sym_master_bin_table_delete_1(table, arg);
          assert(sym_master_bin_table_count(table, arg) == 0);
          if (remove != NULL) {
            if (!bit_map_built) {
              sym_master_bin_table_aux_build_insertion_bitmap(table_aux, mem_pool);
              bit_map_built = true;
            }
            if (!has_insertions || !col_update_bit_map_is_set(&table_aux->bit_map, arg))
              remove(store, arg, mem_pool);
          }
        }
      }
    }

    if (has_insertions && bit_map_built)
      col_update_bit_map_clear(&table_aux->bit_map);
  }

  if (locks_applied && table_aux->locked_surrs.count > 0)
    master_bin_table_aux_unlock_surrs(table, &table_aux->locked_surrs);
}

void sym_master_bin_table_aux_apply_insertions(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = (*ptr)[0];
      uint32 arg2 = (*ptr)[1];
      uint32 surr = (*ptr)[2];
      sym_master_bin_table_insert_with_surr(table, arg1, arg2, surr, mem_pool);
      ptr++;
    }
  }
}
