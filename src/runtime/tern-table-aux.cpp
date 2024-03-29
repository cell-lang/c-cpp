#include "lib.h"


void tern_table_aux_init(TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  master_bin_table_aux_init(&table_aux->master, mem_pool);
  bin_table_aux_init(&table_aux->slave, mem_pool);
  queue_u32_init(&table_aux->surr12_follow_ups);
  queue_3u32_init(&table_aux->insertions);
}

void tern_table_aux_reset(TERN_TABLE_AUX *table_aux) {
  master_bin_table_aux_reset(&table_aux->master);
  bin_table_aux_reset(&table_aux->slave);
  queue_u32_reset(&table_aux->surr12_follow_ups);
  queue_3u32_reset(&table_aux->insertions); //## THIS SHOULD ONLY BE DONE WHEN THERE'S A 1-3 OR 2-3 KEY
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_clear(TERN_TABLE_AUX *table_aux) {
  master_bin_table_aux_clear(&table_aux->master);
  bin_table_aux_clear(&table_aux->slave);
}

void tern_table_aux_delete(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  uint32 surr12 = master_bin_table_lookup_surr(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    assert(bin_table_contains_1(&table->slave, surr12));
    bin_table_aux_delete(&table->slave, &table_aux->slave, surr12, arg3);
    queue_u32_insert(&table_aux->surr12_follow_ups, surr12);
  }
}

void tern_table_aux_delete_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  uint32 surr12 = master_bin_table_lookup_surr(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    assert(bin_table_contains_1(&table->slave, surr12));
    master_bin_table_aux_delete(&table->master, &table_aux->master, arg1, arg2); //# WOULD BE MORE EFFICIENT TO PASS THE SURROGATE INSTEAD
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
        if (master_bin_table_get_arg_1(&table->master, surr12) == arg1) {
          bin_table_aux_delete(&table->slave, &table_aux->slave, surr12, arg3);
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
        uint32 surr12 = master_bin_table_lookup_surr(&table->master, arg1s[i], arg2);
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
        if (master_bin_table_get_arg_2(&table->master, surr12) == arg2) {
          bin_table_aux_delete(&table->slave, &table_aux->slave, surr12, arg3);
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
      uint32 surr12 = master_bin_table_lookup_surr(&table->master, arg1s[i], arg2);
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

void tern_table_aux_apply(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, void (*remove1)(void *, uint32, STATE_MEM_POOL *), void *store1, void (*remove2)(void *, uint32, STATE_MEM_POOL *), void *store2, void (*remove3)(void *, uint32, STATE_MEM_POOL *), void *store3, STATE_MEM_POOL *mem_pool) {
  master_bin_table_aux_apply_surrs_acquisition(&table->master, &table_aux->master);

  master_bin_table_aux_apply_deletions(&table->master, &table_aux->master, remove1, store1, remove2, store2, mem_pool);
  bin_table_aux_apply_deletions(&table->slave, &table_aux->slave, NULL, NULL, remove3, store3, mem_pool);

  master_bin_table_aux_apply_insertions(&table->master, &table_aux->master, mem_pool);
  bin_table_aux_apply_insertions(&table->slave, &table_aux->slave, mem_pool);

  uint32 count = table_aux->surr12_follow_ups.count;
  if (count > 0) {
    uint32 *surr12s = table_aux->surr12_follow_ups.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 surr12 = surr12s[i];
      if (!bin_table_contains_1(&table->slave, surr12))
        if (master_bin_table_contains_surr(&table->master, surr12)) {
          uint32 arg1 = master_bin_table_get_arg_1(&table->master, surr12);
          uint32 arg2 = master_bin_table_get_arg_2(&table->master, surr12);
          bool found = master_bin_table_delete(&table->master, arg1, arg2);
          assert(found);
          if (remove1 != NULL && master_bin_table_count_1(&table->master, arg1) == 0)
            remove1(store1, arg1, mem_pool);
          if (remove2 != NULL && master_bin_table_count_2(&table->master, arg2) == 0)
            remove2(store2, arg2, mem_pool);
        }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void tern_table_aux_sort_insertions_by_13(TERN_TABLE_AUX *table_aux) {
  sort_3u32_by_13(table_aux->insertions.array, table_aux->insertions.count);
}

static void tern_table_aux_sort_insertions_by_23(TERN_TABLE_AUX *table_aux) {
  sort_3u32_by_23(table_aux->insertions.array, table_aux->insertions.count);
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
  uint32 count = table_aux->insertions.count;
  if (count == 0)
    return true;

  TUPLE_3U32 *array = table_aux->insertions.array;

  // First we need to check that there are no conflicts among insertions.
  // That can be done with a sort based on first and third argument
  if (count > 1) {
    tern_table_aux_sort_insertions_by_13(table_aux);

    uint32 prev_arg1 = array[0].x;
    uint32 prev_arg3 = array[0].z;

    for (uint32 i=1 ; i < count ; i++) {
      uint32 arg1 = array[i].x;
      uint32 arg3 = array[i].z;
      if (arg1 == prev_arg1 & arg3 == prev_arg3) {
        uint32 arg2 = array[i].y;
        uint32 prev_arg2 = array[i-1].y;
        // tern_table_aux_record_cols_13_key_violation(table_aux, arg1, arg3, arg2, prev_arg2, true);
        return false;
      }
      prev_arg1 = arg1;
      prev_arg3 = arg3;
    }
  }

  // Now we need to check if the new tuples conflict with the existing ones
  for (uint32 i=0 ; i < count ; i++) {
    uint32 arg1 = array[i].x;
    uint32 arg3 = array[i].z;
    if (tern_table_contains_13(table, arg1, arg3)) {
      assert(tern_table_count_13(table, arg1, arg3) == 1);

      uint32 arg2 = array[i].y;
      //## tern_table_lookup_13 DOES SOME EXTRA WORK TO MAKE SURE THAT THE
      //## SECOND ARGUMENT IS UNIQUE, BUT THAT'S UNNECESSARY HERE. FIX THAT
      uint32 existing_arg2 = tern_table_lookup_13(table, arg1, arg3);

      if (arg2 != existing_arg2) {
        //## THIS COULD BE MADE MORE EFFICIENT, MAYBE USING AN INDEX
        uint32 other_surr12 = master_bin_table_lookup_surr(&table->master, arg1, existing_arg2);
        if (!bin_table_aux_was_deleted(&table_aux->slave, other_surr12, arg3)) {
          // tern_table_aux_record_cols_13_key_violation(table_aux, arg1, arg3, arg2, existing_arg2, false);
          return false;
        }
      }
    }
  }

  return true;
}

bool tern_table_aux_check_key_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  uint32 count = table_aux->insertions.count;
  if (count == 0)
    return true;

  TUPLE_3U32 *array = table_aux->insertions.array;

  // First we need to check that there are no conflicts among insertions.
  // That can be done with a sort based on second and third argument
  if (count > 1) {
    tern_table_aux_sort_insertions_by_23(table_aux);

    uint32 prev_arg2 = array[0].y;
    uint32 prev_arg3 = array[0].z;

    for (uint32 i=1 ; i < count ; i++) {
      uint32 arg2 = array[i].y;
      uint32 arg3 = array[i].z;
      if (arg2 == prev_arg2 & arg3 == prev_arg3) {
        uint32 arg1 = array[i].x;
        uint32 prev_arg1 = array[i-1].x;
        // tern_table_aux_record_cols_23_key_violation(table_aux, arg2, arg3, arg1, prev_arg1, true);
        return false;
      }
      prev_arg2 = arg2;
      prev_arg3 = arg3;
    }
  }

  // Now we need to check if the new tuples conflict with the existing ones
  for (uint32 i=0 ; i < count ; i++) {
    uint32 arg2 = array[i].y;
    uint32 arg3 = array[i].z;
    if (tern_table_contains_23(table, arg2, arg3)) {
      assert(tern_table_count_23(table, arg2, arg3) == 1);

      uint32 arg1 = array[i].x;
      //## tern_table_lookup_13 DOES SOME EXTRA WORK TO MAKE SURE THAT THE
      //## SECOND ARGUMENT IS UNIQUE, BUT THAT'S UNNECESSARY HERE. FIX THAT
      uint32 existing_arg1 = tern_table_lookup_23(table, arg2, arg3);

      if (arg1 != existing_arg1) {
        //## THIS COULD BE MADE MORE EFFICIENT, MAYBE USING AN INDEX
        uint32 existing_surr12 = master_bin_table_lookup_surr(&table->master, existing_arg1, arg2);
        if (!bin_table_aux_was_deleted(&table_aux->slave, existing_surr12, arg3)) {
          // tern_table_aux_record_cols_23_key_violation(table_aux, arg2, arg3, arg1, existing_arg1, false);
          return false;
        }
      }
    }
  }

  return true;
}

// bool tern_table_aux_check_keys_13_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
//   //## CAN THIS BE MADE MORE EFFICIENT?
//   if (!tern_table_aux_check_key_13(table, table_aux))
//     return false;
//  return tern_table_aux_check_key_23(table, table_aux);
// }

////////////////////////////////////////////////////////////////////////////////

bool tern_table_aux_contains_1(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1) {
  assert(table_aux->master.clear == table_aux->slave.clear);

  if (master_bin_table_aux_was_inserted_1(&table_aux->master, arg1))
    return true;

  if (!master_bin_table_contains_1(&table->master, arg1))
    return false;

  if (table_aux->slave.clear)
    return false;

  if (master_bin_table_aux_was_batch_deleted_1(&table_aux->master, arg1))
    return false;

  if (!bin_table_aux_has_deletions(&table_aux->slave))
    return true;

  // Here we know that no tuple with that specific value for the first argument was inserted
  // We just iterater through all the possible (arg1, ?) pairs, and check if the corresponding
  // surrogate is still in the slave table

  //## BAD BAD BAD: THIS IS VERY INEFFICIENT

  uint32 count = master_bin_table_count_1(&table->master, arg1);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[128];
    UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(&table->master, arg1, read, buffer, 64);
    read += array.size;
    uint32 *surrs = array.array + array.offset;
    for (uint32 i=0 ; i < array.size ; i++) {
      assert(bin_table_contains_1(&table->slave, surrs[i]));
      if (bin_table_aux_contains_1(&table->slave, &table_aux->slave, surrs[i]))
        return true;
    }
  }

  return false;
}

bool tern_table_aux_contains_2(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg2) {
  assert(table_aux->master.clear == table_aux->slave.clear);

  if (master_bin_table_aux_was_inserted_2(&table_aux->master, arg2))
    return true;

  if (!master_bin_table_contains_2(&table->master, arg2))
    return false;

  if (table_aux->slave.clear)
    return false;

  if (master_bin_table_aux_was_batch_deleted_2(&table_aux->master, arg2))
    return false;

  if (!bin_table_aux_has_deletions(&table_aux->slave))
    return true;

  // Here we know that no tuple with that specific value for the first argument was inserted
  // We just iterater through all the possible (?, arg2) pairs, and check if the corresponding
  // surrogate is still in the slave table

  //## BAD BAD BAD: THIS IS VERY INEFFICIENT

  uint32 count = master_bin_table_count_2(&table->master, arg2);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[128];
    UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(&table->master, arg2, read, buffer, 64);
    read += array.size;
    uint32 *surrs = array.array + array.offset;
    for (uint32 i=0 ; i < array.size ; i++) {
      assert(bin_table_contains_2(&table->slave, surrs[i]));
      if (bin_table_aux_contains_2(&table->slave, &table_aux->slave, surrs[i]))
        return true;
    }
  }

  return false;
}

bool tern_table_aux_contains_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg3) {
  return bin_table_aux_contains_2(&table->slave, &table_aux->slave, arg3);
}

bool tern_table_aux_contains_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  return master_bin_table_aux_contains(&table->master, &table_aux->master, arg1, arg2);
}

////////////////////////////////////////////////////////////////////////////////

bool tern_table_aux_check_foreign_key_unary_table_1_forward(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  return master_bin_table_aux_check_foreign_key_unary_table_1_forward(&table->master, &table_aux->master, target_table, target_table_aux);
}

bool tern_table_aux_check_foreign_key_unary_table_2_forward(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  return master_bin_table_aux_check_foreign_key_unary_table_2_forward(&table->master, &table_aux->master, target_table, target_table_aux);
}

bool tern_table_aux_check_foreign_key_unary_table_3_forward(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  return bin_table_aux_check_foreign_key_unary_table_2_forward(&table->slave, &table_aux->slave, target_table, target_table_aux);
}

////////////////////////////////////////////////////////////////////////////////

bool tern_table_aux_check_foreign_key_bin_table_backward(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  return master_bin_table_aux_check_foreign_key_bin_table_backward(&table->master, &table_aux->master, src_table, src_table_aux);
}

bool tern_table_aux_check_foreign_key_unary_table_3_backward(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  return bin_table_aux_check_foreign_key_unary_table_2_backward(&table->slave, &table_aux->slave, src_table, src_table_aux);
}
