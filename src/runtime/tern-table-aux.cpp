#include "lib.h"


void tern_table_aux_init(TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  master_bin_table_aux_init(&table_aux->master, mem_pool);
  bin_table_aux_init(&table_aux->slave, mem_pool);
  queue_u32_init(&table_aux->surr12_follow_ups);
  queue_u32_init(&table_aux->insertions);
}

void tern_table_aux_reset(TERN_TABLE_AUX *table_aux) {
  master_bin_table_aux_reset(&table_aux->master);
  bin_table_aux_reset(&table_aux->slave);
  queue_u32_reset(&table_aux->surr12_follow_ups);
  queue_u32_reset(&table_aux->insertions); //## THIS SHOULD ONLY BE DONE WHEN THERE'S A 1-3 OR 2-3 KEY
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_clear(TERN_TABLE_AUX *table_aux) {
  master_bin_table_aux_clear(&table_aux->master);
  bin_table_aux_clear(&table_aux->slave);
}

void tern_table_aux_delete(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    assert(bin_table_contains_1(&table->slave, surr12));
    bin_table_aux_delete(&table_aux->slave, surr12, arg3);
    queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
  }
}

void tern_table_aux_delete_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    assert(bin_table_contains_1(&table->slave, surr12));
    master_bin_table_aux_delete(&table_aux->master, arg1, arg2); //# WOULD BE MORE EFFICIENT TO PASS THE SURROGATE INSTEAD
    bin_table_aux_delete_1(&table_aux->slave, surr12);
  }
}

void tern_table_aux_delete_13(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3) {
  uint32 count1 = master_bin_table_count_1(&table->master, arg1);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count1 > 0 && count3 > 0) {
    if (count1 < count3) {
      uint32 inline_array[512];
      uint32 *arg2s = count1 <= 256 ? inline_array : new_uint32_array(2 * count1); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
      uint32 *surr12s = arg2s + count1;
      master_bin_table_restrict_1(&table->master, arg1, arg2s, surr12s); //## I DON'T NEED THE SECOND ARGUMENTS, JUST THE SURROGATES
      for (uint32 i=0 ; i < count1 ; i++) {
        uint32 surr12 = surr12s[i];
        if (bin_table_contains(&table->slave, surr12, arg3)) {
          bin_table_aux_delete(&table_aux->slave, surr12, arg3);
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
        if (master_bin_table_get_arg_1(&table->master, surr12) == arg1) {
          bin_table_aux_delete(&table_aux->slave, surr12, arg3);
          queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
        }
      }
    }
  }
}

void tern_table_aux_delete_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg2, uint32 arg3) {
  uint32 count2 = master_bin_table_count_2(&table->master, arg2);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count2 != 0 & count3 != 0) {
    if (count2 < count3) {
      uint32 inline_array[256];
      uint32 *arg1s = count2 <= 256 ? inline_array : new_uint32_array(count2); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
      master_bin_table_restrict_2(&table->master, arg2, arg1s);
      for (uint32 i=0 ; i < count2 ; i++) {
        uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1s[i], arg2);
        if (bin_table_contains(&table->slave, surr12, arg3)) {
          bin_table_aux_delete(&table_aux->slave, surr12, arg3);
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
        if (master_bin_table_get_arg_2(&table->master, surr12) == arg2) {
          bin_table_aux_delete(&table_aux->slave, surr12, arg3);
          queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
        }
      }
    }
  }
}

void tern_table_aux_delete_1(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1) {
  uint32 count = master_bin_table_count_1(&table->master, arg1);
  if (count != 0) {
    uint32 inline_array[512];
    uint32 *arg2s = count <= 256 ? inline_array : new_uint32_array(2 * count); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
    uint32 *surr12s = arg2s + count;
    master_bin_table_restrict_1(&table->master, arg1, arg2s, surr12s); //## I DON'T NEED THE SECOND ARGUMENTS, JUST THE SURROGATES
    master_bin_table_aux_delete_1(&table_aux->master, arg1);
    for (uint32 i=0 ; i < count ; i++)
      bin_table_aux_delete_1(&table_aux->slave, surr12s[i]);
  }
}

void tern_table_aux_delete_2(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg2) {
  uint32 count = master_bin_table_count_2(&table->master, arg2);
  if (count != 0) {
    uint32 inline_array[256];
    uint32 *arg1s = count <= 256 ? inline_array : new_uint32_array(count); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
    master_bin_table_restrict_2(&table->master, arg2, arg1s);
    master_bin_table_aux_delete_2(&table_aux->master, arg2);
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1s[i], arg2);
      bin_table_aux_delete_1(&table_aux->slave, surr12);
    }
  }
}

void tern_table_aux_delete_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg3) {
  uint32 count = bin_table_count_2(&table->slave, arg3);
  if (count != 0) {
    uint32 inline_array[512];
    uint32 *surr12s = count <= 512 ? inline_array : new_uint32_array(count); //## THE MEMORY ALLOCATED HERE SHOULD BE RELEASED ASAP
    bin_table_restrict_2(&table->slave, arg3, surr12s);
    bin_table_aux_delete_2(&table_aux->slave, arg3);
    for (uint32 i=0 ; i < count ; i++)
      queue_u32_insert(&table_aux->surr12_follow_ups, surr12s[i]);
  }
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_insert(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_aux_insert(&table->master, &table_aux->master, arg1, arg2);
  bin_table_aux_insert(&table_aux->slave, surr12, arg3);
  queue_3u32_insert(&table_aux->insertions, arg1, arg2, arg3); //## THIS SHOULD ONLY BE DONE WHEN THERE'S A 1-3 OR 2-3 KEY
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_apply(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool) {
  master_bin_table_aux_apply(&table->master, &table_aux->master, incr_rc_1, decr_rc_1, store_1, store_aux_1, incr_rc_2, decr_rc_2, store_2, store_aux_2, mem_pool);
  bin_table_aux_apply(&table->slave, &table_aux->slave, null_incr_rc, null_decr_rc, NULL, NULL, incr_rc_3, decr_rc_3, store_3, store_aux_3, mem_pool);

  uint32 count = table_aux->surr12_follow_ups.count;
  if (count > 0) {
    uint32 *surr12s = table_aux->surr12_follow_ups.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = surr12s[i];
      if (!bin_table_contains_1(&table->slave, surr12))
        if (master_bin_table_contains_surr(&table->master, surr12)) {
          uint32 arg1 = master_bin_table_get_arg_1(&table->master, surr12);
          uint32 arg2 = master_bin_table_get_arg_2(&table->master, surr12);
          master_bin_table_delete(&table->master, arg1, arg2);
          decr_rc_1(store_1, store_aux_1, arg1);
          decr_rc_2(store_2, store_aux_2, arg2);
        }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void tern_table_aux_record_cols_13_key_violation(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3, uint32 arg2, uint32 other_arg2, bool between_new) {

}

static void tern_table_aux_record_cols_23_key_violation(TERN_TABLE_AUX *table_aux, uint32 arg2, uint32 arg3, uint32 arg1, uint32 other_arg1, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

bool tern_table_aux_check_key_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  // THE VIOLATION OF THE KEY IS DETECTED CORRECTLY, BUT THE ERROR MESSAGE MAY BE WRONG/UNHELPFUL
  return bin_table_aux_check_key_2(&table->slave, &table_aux->slave, mem_pool);
}

bool tern_table_aux_check_key_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  // THE VIOLATION OF THE KEY IS DETECTED CORRECTLY, BUT THE ERROR MESSAGE MAY BE WRONG/UNHELPFUL
  return bin_table_aux_check_key_1(&table->slave, &table_aux->slave, mem_pool);
}

bool tern_table_aux_check_key_13(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  uint32 count = table_aux->insertions.count % 3;
  if (count == 0)
    return true;

  uint32 *array = table_aux->insertions.array;

  queue_3u32_permute_132(&table_aux->insertions);

  // First we need to check that there are no conflicts among insertions.
  // That can be done with a sort based on first and third argument
  if (count > 1) {
    queue_3u32_sort_unique(&table_aux->insertions);

    uint32 *ptr = array;
    uint32 prev_arg1 = ptr[0];
    uint32 prev_arg3 = ptr[1];

    for (uint32 i=1 ; i < count ; i++) {
      ptr += 3;
      uint32 arg1 = ptr[0];
      uint32 arg3 = ptr[1];
      if (arg1 == prev_arg1 & arg3 == prev_arg3) {
        uint32 arg2 = ptr[2];
        uint32 prev_arg2 = ptr[-1];
        tern_table_aux_record_cols_13_key_violation(table_aux, arg1, arg3, arg2, prev_arg2, true);
        return false;
      }

      prev_arg1 = arg1;
      prev_arg3 = arg3;
    }
  }

  // Now we need to check if the new tuples conflict with the existing ones
  uint32 *ptr = array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 arg1 = ptr[0];
    uint32 arg3 = ptr[1];
    if (tern_table_contains_13(table, arg1, arg3)) {
      assert(tern_table_count_13(table, arg1, arg3) == 1);

      uint32 arg2 = ptr[2];
      //## tern_table_lookup_13 DOES SOME EXTRA WORK TO MAKE SURE THAT THE
      //## SECOND ARGUMENT IS UNIQUE, BUT THAT'S UNNECESSARY HERE. FIX THAT
      uint32 existing_arg2 = tern_table_lookup_13(table, arg1, arg3);

      if (arg2 != existing_arg2) {
        //## THIS COULD BE MADE MORE EFFICIENT, MAYBE USING AN INDEX
        uint32 other_surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, existing_arg2);
        if (!bin_table_aux_was_deleted(&table_aux->slave, other_surr12, arg3)) {
          tern_table_aux_record_cols_13_key_violation(table_aux, arg1, arg3, arg2, existing_arg2, false);
          return false;
        }
      }
    }
  }

  return true;
}

bool tern_table_aux_check_key_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  uint32 count = table_aux->insertions.count % 3;
  if (count == 0)
    return true;

  uint32 *array = table_aux->insertions.array;

  // First we need to check that there are no conflicts among insertions.
  // That can be done with a sort based on second and third argument
  queue_3u32_permute_231(&table_aux->insertions);
  if (count > 1) {
    queue_3u32_sort_unique(&table_aux->insertions);

    uint32 *ptr = array;
    uint32 prev_arg2 = ptr[0];
    uint32 prev_arg3 = ptr[1];

    for (uint32 i=1 ; i < count ; i++) {
      ptr += 3;
      uint32 arg2 = ptr[0];
      uint32 arg3 = ptr[1];
      if (arg2 == prev_arg2 & arg3 == prev_arg3) {
        uint32 arg1 = ptr[2];
        uint32 prev_arg1 = ptr[-1];
        tern_table_aux_record_cols_23_key_violation(table_aux, arg2, arg3, arg1, prev_arg1, true);
        // No need to permute back to the normal order, since the 2-3 key is checked after the 1-3 one, if this is present
        return false;
      }

      prev_arg2 = arg2;
      prev_arg3 = arg3;
    }
  }

  // Now we need to check if the new tuples conflict with the existing ones
  uint32 *ptr = array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 arg2 = ptr[0];
    uint32 arg3 = ptr[1];
    if (tern_table_contains_23(table, arg2, arg3)) {
      assert(tern_table_count_23(table, arg2, arg3) == 1);

      uint32 arg1 = ptr[2];
      //## tern_table_lookup_13 DOES SOME EXTRA WORK TO MAKE SURE THAT THE
      //## SECOND ARGUMENT IS UNIQUE, BUT THAT'S UNNECESSARY HERE. FIX THAT
      uint32 existing_arg1 = tern_table_lookup_23(table, arg2, arg3);

      if (arg1 != existing_arg1) {
        //## THIS COULD BE MADE MORE EFFICIENT, MAYBE USING AN INDEX
        uint32 existing_surr12 = master_bin_table_lookup_surrogate(&table->master, existing_arg1, arg2);
        if (!bin_table_aux_was_deleted(&table_aux->slave, existing_surr12, arg3)) {
          tern_table_aux_record_cols_23_key_violation(table_aux, arg2, arg3, arg1, existing_arg1, false);
          return false;
        }
      }
    }
  }

  return true;
}

bool tern_table_aux_check_keys_13_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  //## CAN THIS BE MADE MORE EFFICIENT?
  if (!tern_table_aux_check_key_13(table, table_aux))
    return false;
  queue_3u32_permute_132(&table_aux->insertions);
  return tern_table_aux_check_key_23(table, table_aux);
}