#include "lib.h"


void unary_table_aux_init(UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u32_init(&table_aux->deletions);
  queue_u32_init(&table_aux->insertions);
  table_aux->clear = false;
#ifndef NDEBUG
  table_aux->prepared = false;
#endif
}

void unary_table_aux_reset(UNARY_TABLE_AUX *table_aux) {
  queue_u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->deletions);
  table_aux->clear = false;
#ifndef NDEBUG
  table_aux->prepared = false;
#endif
}

////////////////////////////////////////////////////////////////////////////////

uint32 unary_table_aux_insert(UNARY_TABLE_AUX *table_aux, uint32 elt) {
  queue_u32_insert(&table_aux->insertions, elt);
}

void unary_table_aux_delete(UNARY_TABLE_AUX *table_aux, uint32 elt) {
  queue_u32_insert(&table_aux->deletions, elt);
}

void unary_table_aux_clear(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

////////////////////////////////////////////////////////////////////////////////

void unary_table_aux_apply(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  if (table_aux->clear) {
    UNARY_TABLE_ITER iter;
    unary_table_iter_init(table, &iter);
    while (!unary_table_iter_is_out_of_range(&iter)) {
      uint32 surr = unary_table_iter_get(&iter);
      decr_rc(store, store_aux, surr);
    }
  }
  else {
    uint32 count = table_aux->deletions.count;
    if (count > 0) {
      uint32 *array = table_aux->deletions.array;
      for (uint32 i=0 ; i < count ; i++) {
        uint32 elt = array[i];
        if (unary_table_delete(table, elt))
          decr_rc(store, store_aux, elt);
      }
    }
  }

  uint32 count = table_aux->insertions.count;
  if (count > 0) {
    uint32 *array = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 elt = array[i];
      if (unary_table_insert(table, elt, mem_pool))
        incr_rc(store, elt);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void remove_deletion_reinsertion_pairs(QUEUE_U32 *deletions, QUEUE_U32 *insertions) {
  uint32 num_dels = deletions->count;
  uint32 num_ins = insertions->count;
  assert(num_dels > 0 && num_ins > 0);

  uint32 del_idx = 0;
  uint32 ins_idx = 0;

  uint32 next_del_idx = 0;
  uint32 next_ins_idx = 0;

  uint32 *del_elts = deletions->array;
  uint32 *ins_elts = insertions->array;

  for ( ; ; ) {
    assert(del_idx < num_dels & ins_idx < num_ins);

    uint32 del_elt = del_elts[0];
    uint32 ins_elt = ins_elts[0];

    if (del_elt == ins_elt) {
      del_idx++;
      ins_idx++;
    }
    else if (del_elt < ins_elt) {
      if (del_idx != next_del_idx)
        del_elts[next_del_idx] = del_elt;
      del_idx++;
      next_del_idx++;
    }
    else {
      if (ins_idx != next_ins_idx)
        ins_elts[next_ins_idx] = ins_idx;
      ins_idx++;
      next_del_idx++;
    }

    assert((del_idx == next_del_idx) == (ins_idx == next_ins_idx));

    if (del_idx == num_dels) {
      if (del_idx != next_del_idx) {
        // There was at least one cancellation
        for (uint32 i=ins_idx ; i < num_ins ; i++)
          ins_elts[next_ins_idx++] = ins_elts[i];
        assert(next_ins_idx < num_ins);
        deletions->count = next_del_idx;
        insertions->count = next_ins_idx;
      }
      return;
    }

    if (ins_idx == num_ins) {
      if (ins_idx != next_ins_idx) {
        // There was at least one cancellation
        for (uint32 i=del_idx ; i < num_dels ; i++)
          del_elts[next_del_idx++] = del_elts[i];
        assert(next_del_idx < num_dels);
        deletions->count = next_del_idx;
        insertions->count = next_ins_idx;
      }
      return;
    }
  }
}

void unary_table_aux_prepare(UNARY_TABLE_AUX *table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  uint32 num_ins = table_aux->insertions.count;

  if (num_dels > 0) {
    queue_u32_sort_unique(&table_aux->deletions);
    if (num_ins > 0) {
      queue_u32_sort_unique(&table_aux->insertions);
      remove_deletion_reinsertion_pairs(&table_aux->deletions, &table_aux->insertions);
    }
  }
  else if (num_ins > 0)
    queue_u32_sort_unique(&table_aux->insertions);

#ifndef NDEBUG
  table_aux->prepared = true;
#endif
}

bool unary_table_aux_contains(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, uint32 elt) {
  assert(table_aux->prepared);

  if (unary_table_contains(table, elt) && !table_aux->clear && !queue_u32_sorted_contains(&table_aux->deletions, elt))
    return true;
  return queue_u32_sorted_contains(&table_aux->insertions, elt);
}

bool unary_table_aux_is_empty(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  assert(table_aux->prepared);

  if (table_aux->insertions.count > 0)
    return false;

  if (table_aux->clear)
    return true;

  uint32 size = table->count;
  if (size == 0)
    return true;

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels < size)
    return false;

  uint32 actual_dels = 0;
  uint32 *deletions = table_aux->deletions.array;
  for (uint32 i=0 ; i < num_dels ; i++)
    if (unary_table_contains(table, deletions[i])) {
      actual_dels++;
      if (actual_dels == size)
        return true;
    }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_aux_check_foreign_key_unary_table_forward(UNARY_TABLE_AUX *table_aux, UNARY_TABLE *target_table, UNARY_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!unary_table_aux_contains(target_table, target_table_aux, new_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_slave_tern_table_3_forward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *target_table, SLAVE_TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!bin_table_aux_contains_2(target_table, &target_table_aux->slave_table_aux, new_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_3_forward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *target_table, TERN_TABLE_AUX *target_table_aux) {
  uint32 num_ins = table_aux->insertions.count;
  if (num_ins > 0) {
    uint32 *new_elts = table_aux->insertions.array;
    for (uint32 i=0 ; i < num_ins ; i++)
      if (!tern_table_aux_contains_3(target_table, target_table_aux, new_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool unary_table_aux_check_foreign_key_unary_table_backward(UNARY_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (unary_table_aux_contains(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (bin_table_aux_contains_1(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (bin_table_aux_contains_2(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}


bool unary_table_aux_check_foreign_key_master_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (master_bin_table_aux_contains_1(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_master_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (master_bin_table_aux_contains_2(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_obj_col_1_backward(UNARY_TABLE_AUX *table_aux, OBJ_COL *src_col, OBJ_COL_AUX *src_col_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (obj_col_aux_contains_1(src_col, src_col_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_int_col_1_backward(UNARY_TABLE_AUX *table_aux, INT_COL *src_col, INT_COL_AUX *src_col_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (int_col_aux_contains_1(src_col, src_col_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_float_col_1_backward(UNARY_TABLE_AUX *table_aux, FLOAT_COL *src_col, FLOAT_COL_AUX *src_col_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (float_col_aux_contains_1(src_col, src_col_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_slave_tern_table_3_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, SLAVE_TERN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (bin_table_aux_contains_2(src_table, &src_table_aux->slave_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_1_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (tern_table_aux_contains_1(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_2_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (tern_table_aux_contains_2(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_3_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  assert(table_aux->prepared);

  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (tern_table_aux_contains_3(src_table, src_table_aux, del_elts[i])) {
        // No need to check for reinsertions (see prepare() method)
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}
