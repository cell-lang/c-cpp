#include "lib.h"


void unary_table_aux_init(UNARY_TABLE_AUX *table_aux, STATE_MEM_POOL *) {
  queue_u32_init(&table_aux->deletions);
  queue_u32_init(&table_aux->insertions);
#ifndef NDEBUG
  table_aux->init_capacity = 0xFFFFFFFF;
#endif
  table_aux->clear = false;
}

void unary_table_aux_reset(UNARY_TABLE_AUX *table_aux) {
  queue_u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->deletions);
#ifndef NDEBUG
  table_aux->init_capacity = 0xFFFFFFFF;
#endif
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

uint32 unary_table_aux_insert(UNARY_TABLE_AUX *table_aux, uint32 elt) {
  queue_u32_insert(&table_aux->insertions, elt);
}

void unary_table_aux_delete(UNARY_TABLE_AUX *table_aux, uint32 elt) {
  queue_u32_insert(&table_aux->deletions, elt);
}

void unary_table_aux_clear(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  table_aux->init_capacity = table->capacity;
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

void unary_table_aux_prepare(UNARY_TABLE_AUX *table_aux) {
  //## REQUIRES CANCELLING OUT DELETION+REINSERSIONS
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool unary_table_aux_contains(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux, uint32 elt) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool unary_table_aux_is_empty(UNARY_TABLE *table, UNARY_TABLE_AUX *table_aux) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
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

////////////////////////////////////////////////////////////////////////////////

bool unary_table_aux_check_foreign_key_unary_table_backward(UNARY_TABLE_AUX *table_aux, UNARY_TABLE *src_table, UNARY_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (unary_table_aux_contains(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (bin_table_aux_contains_1(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, BIN_TABLE *src_table, BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (bin_table_aux_contains_2(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}


bool unary_table_aux_check_foreign_key_master_bin_table_1_backward(UNARY_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (master_bin_table_aux_contains_1(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_master_bin_table_2_backward(UNARY_TABLE_AUX *table_aux, MASTER_BIN_TABLE *src_table, MASTER_BIN_TABLE_AUX *src_table_aux) {
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
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (obj_col_aux_contains_1(src_col, src_col_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_int_col_1_backward(UNARY_TABLE_AUX *table_aux, INT_COL *src_col, INT_COL_AUX *src_col_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (int_col_aux_contains_1(src_col, src_col_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_float_col_1_backward(UNARY_TABLE_AUX *table_aux, FLOAT_COL *src_col, FLOAT_COL_AUX *src_col_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (float_col_aux_contains_1(src_col, src_col_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_1_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (tern_table_aux_contains_1(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_2_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (tern_table_aux_contains_2(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}

bool unary_table_aux_check_foreign_key_tern_table_3_backward(UNARY_TABLE_AUX *table_aux, TERN_TABLE *src_table, TERN_TABLE_AUX *src_table_aux) {
  uint32 num_dels = table_aux->deletions.count;
  if (num_dels > 0) {
    uint32 *del_elts = table_aux->deletions.array;
    for (uint32 i=0 ; i < num_dels ; i++) {
      if (tern_table_aux_contains_3(src_table, src_table_aux, del_elts[i])) {
        //## RECORD THE ERROR
        return false;
      }
    }
  }
  return true;
}
