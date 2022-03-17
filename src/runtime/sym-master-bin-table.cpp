#include "lib.h"
#include "one-way-bin-table.h"


uint32 master_bin_table_alloc_surr(MASTER_BIN_TABLE *, uint32 arg1, uint32 arg2, STATE_MEM_POOL *);
void master_bin_table_release_surr(MASTER_BIN_TABLE *, uint32 surr);

////////////////////////////////////////////////////////////////////////////////

inline void sort_args(uint32 &arg1, uint32 &arg2) {
  if (arg1 > arg2) {
    uint32 tmp = arg1;
    arg1 = arg2;
    arg2 = tmp;
  }
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_init(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  master_bin_table_init(table, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

uint32 sym_master_bin_table_size(MASTER_BIN_TABLE *table) {
  return sym_bin_table_size(&table->table);
}

bool sym_master_bin_table_contains(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return sym_bin_table_contains(&table->table, arg1, arg2);
}

bool sym_master_bin_table_contains_1(MASTER_BIN_TABLE *table, uint32 arg) {
  return sym_bin_table_contains_1(&table->table, arg);
}

uint32 sym_master_bin_table_count(MASTER_BIN_TABLE *table, uint32 arg) {
  return sym_bin_table_count(&table->table, arg);
}

uint32 sym_master_bin_table_restrict(MASTER_BIN_TABLE *table, uint32 arg, uint32 *other_args) {
  return sym_bin_table_restrict(&table->table, arg, other_args);
}

uint32 sym_master_bin_table_lookup(MASTER_BIN_TABLE *table, uint32 arg) {
  return sym_bin_table_lookup(&table->table, arg);
}

uint32 sym_master_bin_table_lookup_surrogate(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);
  return master_bin_table_lookup_surrogate(table, arg1, arg2);
}

uint32 sym_master_bin_table_get_arg_1(MASTER_BIN_TABLE *table, uint32 surr) {
  return master_bin_table_get_arg_1(table, surr);
}

uint32 sym_master_bin_table_get_arg_2(MASTER_BIN_TABLE *table, uint32 surr) {
  return master_bin_table_get_arg_2(table, surr);
}

////////////////////////////////////////////////////////////////////////////////

// Code to recover the surrogate:
//   int32 code = master_bin_table_insert_ex(table, arg1, arg2, mem_pool);
//   uint32 surr12 = code >= 0 ? code : -code - 1;
//   bool was_new = code >= 0;
int32 sym_master_bin_table_insert_ex(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  sort_args(arg1, arg2);

  if (one_way_bin_table_contains(&table->table.forward, arg1, arg2)) {
    uint32 surr = master_bin_table_lookup_surrogate(table, arg1, arg2);
    assert(surr != 0xFFFFFFFF);
    return -((int32) surr) - 1;
  }

  assert(!one_way_bin_table_contains(&table->table.forward, arg1, arg2));
  assert(arg1 == arg2 || !one_way_bin_table_contains(&table->table.backward, arg2, arg1));

  uint32 surr = master_bin_table_alloc_surr(table, arg1, arg2, mem_pool);
  loaded_one_way_bin_table_insert_unique(&table->table.forward, arg1, arg2, surr, mem_pool);
  if (arg1 != arg2)
    one_way_bin_table_insert_unique(&table->table.backward, arg2, arg1, mem_pool);

  return surr;
}

bool sym_master_bin_table_insert(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  int32 code = sym_master_bin_table_insert_ex(table, arg1, arg2, mem_pool);
  return code >= 0;
}

bool sym_master_bin_table_delete(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);
  uint32 surr = loaded_one_way_bin_table_delete(&table->table.forward, arg1, arg2);
  if (surr != 0xFFFFFFFF & arg1 != arg2) {
    bool found = one_way_bin_table_delete(&table->table.backward, arg2, arg1);
    assert(found);
    master_bin_table_release_surr(table, surr);
    return true;
  }
  else
    return false;
}

void sym_master_bin_table_delete_1(MASTER_BIN_TABLE *table, uint32 arg) {
  uint32 inline_array[1024];

  uint32 fwd_count = one_way_bin_table_get_count(&table->table.forward, arg);
  if (fwd_count > 0) {
    uint32 *other_args = fwd_count <= 512 ? inline_array : new_uint32_array(2 * fwd_count);
    uint32 *surrs = other_args + fwd_count;
    loaded_one_way_bin_table_delete_by_key(&table->table.forward, arg, other_args, surrs);

    for (uint32 i=0 ; i < fwd_count ; i++) {
      bool found = one_way_bin_table_delete(&table->table.backward, other_args[i], arg);
      assert(found);
      master_bin_table_release_surr(table, surrs[i]);
    }
  }

  uint32 bkwd_count = one_way_bin_table_get_count(&table->table.backward, arg);
  if (bkwd_count > 0) {
    uint32 *other_args = (bkwd_count <= 1024 | bkwd_count <= 2 * fwd_count) ? inline_array : new_uint32_array(bkwd_count);
    one_way_bin_table_delete_by_key(&table->table.backward, arg, other_args);

    for (uint32 i=0 ; i < bkwd_count ; i++) {
      uint32 surr = loaded_one_way_bin_table_delete(&table->table.forward, other_args[i], arg);
      assert(surr != 0xFFFFFFFF);
      master_bin_table_release_surr(table, surr);
    }
  }
}

void sym_master_bin_table_clear(MASTER_BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  master_bin_table_clear(table, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_copy_to(MASTER_BIN_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store, STREAM *strm_1, STREAM *strm_2) {
  bin_table_copy_to(&table->table, surr_to_obj, store, surr_to_obj, store, strm_1, strm_2);
}

void sym_master_bin_table_write(WRITE_FILE_STATE *write_state, MASTER_BIN_TABLE *table, OBJ (*surr_to_obj)(void *, uint32), void *store) {
  bin_table_write(write_state, &table->table, surr_to_obj, store, surr_to_obj, store, false);
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_iter_init_empty(MASTER_BIN_TABLE_ITER *iter) {
  master_bin_table_iter_init_empty(iter);
}

void sym_master_bin_table_iter_init(MASTER_BIN_TABLE *table, MASTER_BIN_TABLE_ITER *iter) {
  master_bin_table_iter_init(table, iter);
}

void sym_master_bin_table_iter_move_forward(MASTER_BIN_TABLE_ITER *iter) {
  master_bin_table_iter_move_forward(iter);
}

bool sym_master_bin_table_iter_is_out_of_range(MASTER_BIN_TABLE_ITER *iter) {
  return master_bin_table_iter_is_out_of_range(iter);
}

uint32 sym_master_bin_table_iter_get_1(MASTER_BIN_TABLE_ITER *iter) {
  return master_bin_table_iter_get_1(iter);
}

uint32 sym_master_bin_table_iter_get_2(MASTER_BIN_TABLE_ITER *iter) {
  return master_bin_table_iter_get_2(iter);
}

uint32 sym_master_bin_table_iter_get_surr(MASTER_BIN_TABLE_ITER *iter) {
  return master_bin_table_iter_get_surr(iter);
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_iter_1_init_empty(SYM_MASTER_BIN_TABLE_ITER_1 *iter) {
#ifndef NDEBUG
  uint32 inline_array[BIN_TABLE_ITER_INLINE_SIZE];
  iter->forward = NULL;
  iter->other_args = NULL;
  iter->arg = 0xFFFFFFFF;
#endif
  iter->left = 0;
}

void sym_master_bin_table_iter_1_init(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_ITER_1 *iter, uint32 arg) {
  uint32 count = sym_master_bin_table_count(table, arg);

  if (count > 0) {
    uint32 *other_args = count <= BIN_TABLE_ITER_INLINE_SIZE ? iter->inline_array : new_uint32_array(count);
    sym_master_bin_table_restrict(table, arg, other_args);
    iter->other_args = other_args;
    iter->arg = arg;
    iter->left = count;
    iter->forward = &table->table.forward;
  }
  else
    sym_master_bin_table_iter_1_init_empty(iter);
}

void sym_master_bin_table_iter_1_move_forward(SYM_MASTER_BIN_TABLE_ITER_1 *iter) {
  assert(!sym_master_bin_table_iter_1_is_out_of_range(iter));
  iter->other_args++;
  iter->left--;
}

bool sym_master_bin_table_iter_1_is_out_of_range(SYM_MASTER_BIN_TABLE_ITER_1 *iter) {
  return iter->left > 0;
}

uint32 sym_master_bin_table_iter_1_get_1(SYM_MASTER_BIN_TABLE_ITER_1 *iter) {
  assert(!sym_master_bin_table_iter_1_is_out_of_range(iter));
  return *iter->other_args;
}

uint32 sym_master_bin_table_iter_1_get_surr(SYM_MASTER_BIN_TABLE_ITER_1 *iter) {
  assert(!sym_master_bin_table_iter_1_is_out_of_range(iter));
  uint32 arg1 = iter->arg;
  uint32 arg2 = *iter->other_args;
  sort_args(arg1, arg2);
  return loaded_one_way_bin_table_payload(iter->forward, arg1, arg2); //## THIS COULD BE AVOIDED FOR THE TUPLES WHERE arg <= other_arg
}
