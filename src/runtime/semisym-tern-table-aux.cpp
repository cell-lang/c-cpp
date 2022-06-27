#include "lib.h"


void semisym_tern_table_aux_init(SEMISYM_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  sym_master_bin_table_aux_init(&table_aux->master, mem_pool);
  bin_table_aux_init(&table_aux->slave, mem_pool);
  queue_u32_init(&table_aux->surr12_follow_ups);
}

void semisym_tern_table_aux_reset(SEMISYM_TERN_TABLE_AUX *table_aux) {
  sym_master_bin_table_aux_reset(&table_aux->master);
  bin_table_aux_reset(&table_aux->slave);
  queue_u32_reset(&table_aux->surr12_follow_ups);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_aux_clear(SEMISYM_TERN_TABLE_AUX *table_aux) {
  sym_master_bin_table_aux_clear(&table_aux->master);
  bin_table_aux_clear(&table_aux->slave);
}

void semisym_tern_table_aux_delete(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    assert(bin_table_contains_1(&table->slave, surr12));
    bin_table_aux_delete(&table->slave, &table_aux->slave, surr12, arg3);
    queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
  }
}

void semisym_tern_table_aux_delete_12(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr12 = sym_master_bin_table_lookup_surr(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    assert(bin_table_contains_1(&table->slave, surr12));
    sym_master_bin_table_aux_delete(&table->master, &table_aux->master, arg1, arg2); //# WOULD BE MORE EFFICIENT TO PASS THE SURROGATE INSTEAD
    bin_table_aux_delete_1(&table_aux->slave, surr12);
  }
}

void semisym_tern_table_aux_delete_13_23(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg12, uint32 arg3) {
  uint32 count12 = sym_master_bin_table_count(&table->master, arg12);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count12 > 0 && count3 > 0) {
    if (count12 < count3) {
      uint32 inline_array[256];
      uint32 *other_args_12 = count12 <= 256 ? inline_array : new_uint32_array(count12); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
      sym_master_bin_table_restrict(&table->master, arg12, other_args_12);
      for (uint32 i=0 ; i < count12 ; i++) {
        uint32 surr12 = sym_master_bin_table_lookup_surr(&table->master, arg12, other_args_12[i]);
        if (bin_table_contains(&table->slave, surr12, arg3)) {
          bin_table_aux_delete(&table->slave, &table_aux->slave, surr12, arg3);
          queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
        }
      }
    }
    else {
      uint32 inline_array[256];
      uint32 *surr12s = count3 <= 256 ? inline_array : new_uint32_array(count3); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
      bin_table_restrict_2(&table->slave, arg3, surr12s);
      for (uint32 i=0 ; i < count3 ; i++) {
        uint32 surr12 = surr12s[i];
        if (sym_master_bin_table_get_arg_1(&table->master, surr12) == arg12 || sym_master_bin_table_get_arg_2(&table->master, surr12) == arg12) {
          bin_table_aux_delete(&table->slave, &table_aux->slave, surr12, arg3);
          queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
        }
      }
    }
  }
}

void semisym_tern_table_aux_delete_1(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg) {
  uint32 count = sym_master_bin_table_count(&table->master, arg);
  if (count != 0) {
    uint32 inline_array[256];
    uint32 *other_args = count <= 256 ? inline_array : new_uint32_array(count); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
    sym_master_bin_table_restrict(&table->master, arg, other_args); //## I SHOULD RETRIEVE THE SURROGATES DIRECTLY
    sym_master_bin_table_aux_delete_1(&table_aux->master, arg);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = sym_master_bin_table_lookup_surr(&table->master, arg, other_args[i]);
      bin_table_aux_delete_1(&table_aux->slave, surr12);
    }
  }
}

void semisym_tern_table_aux_delete_3(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg3) {
  uint32 count = bin_table_count_2(&table->slave, arg3);
  if (count != 0) {
    uint32 inline_array[256];
    uint32 *surr12s = count <= 256 ? inline_array : new_uint32_array(count); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
    bin_table_restrict_2(&table->slave, arg3, surr12s);
    bin_table_aux_delete_2(&table_aux->slave, arg3);
    for (uint32 i=0 ; i < count ; i++)
      queue_u32_insert(&table_aux->surr12_follow_ups, surr12s[i]);
  }
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_aux_insert(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = sym_master_bin_table_aux_insert(&table->master, &table_aux->master, arg1, arg2);
  bin_table_aux_insert(&table_aux->slave, surr12, arg3);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_aux_apply(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, void (*incr_rc_12)(void *, uint32), void (*decr_rc_12)(void *, void *, uint32), void *store_12, void *store_aux_12, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool) {
  sym_master_bin_table_aux_apply(&table->master, &table_aux->master, incr_rc_12, decr_rc_12, store_12, store_aux_12, mem_pool);
  bin_table_aux_apply(&table->slave, &table_aux->slave, null_incr_rc, null_decr_rc, NULL, NULL, incr_rc_3, decr_rc_3, store_3, store_aux_3, mem_pool);

  uint32 count = table_aux->surr12_follow_ups.count;
  if (count > 0) {
    uint32 *surr12s = table_aux->surr12_follow_ups.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = surr12s[i];
      if (!bin_table_contains_1(&table->slave, surr12))
        if (sym_master_bin_table_contains_surr(&table->master, surr12)) {
          uint32 arg1 = sym_master_bin_table_get_arg_1(&table->master, surr12);
          uint32 arg2 = sym_master_bin_table_get_arg_2(&table->master, surr12);
          sym_master_bin_table_delete(&table->master, arg1, arg2);
          decr_rc_12(store_12, store_aux_12, arg1);
          decr_rc_12(store_12, store_aux_12, arg2);
        }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void semisym_tern_table_aux_record_cols_13_key_violation(SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3, uint32 arg2, uint32 other_arg2, bool between_new) {

}

static void semisym_tern_table_aux_record_cols_23_key_violation(SEMISYM_TERN_TABLE_AUX *table_aux, uint32 arg2, uint32 arg3, uint32 arg1, uint32 other_arg1, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

bool semisym_tern_table_aux_check_key_12(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  // THE VIOLATION OF THE KEY IS DETECTED CORRECTLY, BUT THE ERROR MESSAGE MAY BE WRONG/UNHELPFUL
  return bin_table_aux_check_key_1(&table->slave, &table_aux->slave, mem_pool);
}

bool semisym_tern_table_aux_check_key_3(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  // THE VIOLATION OF THE KEY IS DETECTED CORRECTLY, BUT THE ERROR MESSAGE MAY BE WRONG/UNHELPFUL
  return bin_table_aux_check_key_2(&table->slave, &table_aux->slave, mem_pool);
}

////////////////////////////////////////////////////////////////////////////////

void semisym_tern_table_aux_prepare(SEMISYM_TERN_TABLE_AUX *) {

}

////////////////////////////////////////////////////////////////////////////////

bool semisym_tern_table_aux_check_foreign_key_sym_bin_table_forward(TERN_TABLE *, SEMISYM_TERN_TABLE_AUX *, BIN_TABLE *, SYM_BIN_TABLE_AUX *) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool semisym_tern_table_aux_check_foreign_key_unary_table_3_forward(TERN_TABLE *, SEMISYM_TERN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool semisym_tern_table_aux_check_foreign_key_sym_bin_table_backward(TERN_TABLE *, SEMISYM_TERN_TABLE_AUX *, BIN_TABLE *, SYM_BIN_TABLE_AUX *) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool semisym_tern_table_aux_check_foreign_key_unary_table_3_backward(TERN_TABLE *, SEMISYM_TERN_TABLE_AUX *, UNARY_TABLE *, UNARY_TABLE_AUX *) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}
