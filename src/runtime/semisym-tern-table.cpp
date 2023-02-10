#include "lib.h"


void semisym_tern_table_init(TERN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  tern_table_init(table, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

bool semisym_tern_table_insert(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool) {
  int32 code = sym_master_bin_table_insert_ex(&table->master, arg1, arg2, mem_pool);
  uint32 surr12 = code >= 0 ? code : -code - 1;
  return bin_table_insert(&table->slave, surr12, arg3, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

static void semisym_tern_table_resolve_12(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3) {
  int32 surr12 = sym_master_bin_table_lookup_surr(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    uint32 curr_arg3 = bin_table_lookup_1(&table->slave, surr12);
    assert(curr_arg3 != 0xFFFFFFFF); // Because of the integrity constraints
    assert(curr_arg3 != arg3); // Because the new tuple is not already there
    bin_table_delete_1(&table->slave, surr12);
    if (remove3 != NULL && !bin_table_contains_2(&table->slave, curr_arg3))
      remove3(store3, curr_arg3, mem_pool);
  }
}

static void semisym_tern_table_resolve_3(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2) {
  uint32 curr_surr12 = bin_table_lookup_2(&table->slave, arg3);
  if (curr_surr12 != 0xFFFFFFFF) {
    assert(curr_surr12 != sym_master_bin_table_lookup_surr(&table->master, arg1, arg2)); // Because the new tuple is not already there
    bin_table_delete_2(&table->slave, arg3);
    assert(!bin_table_contains_1(&table->slave, curr_surr12)); // Because there must also be a key on the first two columns
    uint32 curr_arg1 = sym_master_bin_table_get_arg_1(&table->master, curr_surr12);
    uint32 curr_arg2 = sym_master_bin_table_get_arg_2(&table->master, curr_surr12);
    sym_master_bin_table_delete(&table->master, curr_arg1, curr_arg2);
    if (remove1 != NULL && curr_arg1 != arg1 && !sym_master_bin_table_contains_1(&table->master, curr_arg1))
      remove1(store1, curr_arg1, mem_pool);
    if (remove2 != NULL && curr_arg2 != arg2 && !sym_master_bin_table_contains_1(&table->master, curr_arg2))
      remove2(store2, curr_arg2, mem_pool);
  }
}

void semisym_tern_table_update_12(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3) {
  if (!semisym_tern_table_contains(table, arg1, arg2, arg3)) {
    semisym_tern_table_resolve_12(table, arg1, arg2, arg3, mem_pool, remove3, store3);
    semisym_tern_table_insert(table, arg1, arg2, arg3, mem_pool);
  }
}

void semisym_tern_table_update_12_3(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, STATE_MEM_POOL *mem_pool, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3) {
  if (!semisym_tern_table_contains(table, arg1, arg2, arg3)) {
    semisym_tern_table_resolve_12(table, arg1, arg2, arg3, mem_pool, remove3, store3);
    semisym_tern_table_resolve_3(table, arg1, arg2, arg3, mem_pool, remove1, store1, remove2, store2);
    semisym_tern_table_insert(table, arg1, arg2, arg3, mem_pool);
  }
}

////////////////////////////////////////////////////////////////////////////////

uint32 semisym_tern_table_size(TERN_TABLE *table) {
  return bin_table_size(&table->slave);
}

//## BAD BAD BAD: FOR SOME OF THESE OPERATION THE SLAVE VERSION ENDS UP DOING MORE WORK THAN NECESSARY

uint32 semisym_tern_table_count_1(TERN_TABLE *table, uint32 arg1) {
  return semisym_slave_tern_table_count_1(&table->master, &table->slave, arg1);
}

uint32 semisym_tern_table_count_3(TERN_TABLE *table, uint32 arg3) {
  return semisym_slave_tern_table_count_3(&table->slave, arg3);
}

uint32 semisym_tern_table_count_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return semisym_slave_tern_table_count_12(&table->master, &table->slave, arg1, arg2);
}

uint32 semisym_tern_table_count_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return semisym_slave_tern_table_count_13(&table->master, &table->slave, arg1, arg3);
}

bool semisym_tern_table_contains(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3) {
  return semisym_slave_tern_table_contains(&table->master, &table->slave, arg1, arg2, arg3);
}

bool semisym_tern_table_contains_1(TERN_TABLE *table, uint32 arg1) {
  return semisym_slave_tern_table_contains_1(&table->master, &table->slave, arg1);
}

bool semisym_tern_table_contains_3(TERN_TABLE *table, uint32 arg3) {
  return semisym_slave_tern_table_contains_3(&table->slave, arg3);
}

bool semisym_tern_table_contains_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return semisym_slave_tern_table_contains_12(&table->master, &table->slave, arg1, arg2);
}

bool semisym_tern_table_contains_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return semisym_slave_tern_table_contains_13(&table->master, &table->slave, arg1, arg3);
}

uint32 semisym_tern_table_lookup_12(TERN_TABLE *table, uint32 arg1, uint32 arg2) {
  return semisym_slave_tern_table_lookup_12(&table->master, &table->slave, arg1, arg2);
}

uint32 semisym_tern_table_lookup_13(TERN_TABLE *table, uint32 arg1, uint32 arg3) {
  return semisym_slave_tern_table_lookup_13(&table->master, &table->slave, arg1, arg3);
}

////////////////////////////////////////////////////////////////////////////////

bool semisym_tern_table_cols_12_are_key(TERN_TABLE *table) {
  return semisym_slave_tern_table_cols_12_are_key(&table->slave);
}

bool semisym_tern_table_col_3_is_key(TERN_TABLE *table) {
  return semisym_slave_tern_table_col_3_is_key(&table->slave);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_copy_to(TERN_TABLE *table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, STREAM *strm_1, STREAM *strm_2, STREAM *strm_3) {
  semisym_slave_tern_table_copy_to(&table->master, &table->slave, surr_to_obj_1_2, store_1_2, surr_to_obj_3, store_3, strm_1, strm_2, strm_3);
}

void semisym_tern_table_write(WRITE_FILE_STATE *write_state, TERN_TABLE *table, OBJ (*surr_to_obj_1_2)(void *, uint32), void *store_1_2, OBJ (*surr_to_obj_3)(void *, uint32), void *store_3, uint32 idx1, uint32 idx2, uint32 idx3) {
  semisym_slave_tern_table_write(write_state, &table->master, &table->slave, surr_to_obj_1_2, store_1_2, surr_to_obj_3, store_3, idx1, idx2, idx3);
}
