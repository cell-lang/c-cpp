#include "lib.h"


uint32 master_bin_table_get_next_free_surr(MASTER_BIN_TABLE *table, uint32 last_idx);
void master_bin_table_set_next_free_surr(MASTER_BIN_TABLE *table, uint32 next_free);

bool master_bin_table_insert_with_surr(MASTER_BIN_TABLE *table, uint32 arg1, uint32 arg2, uint32 surr, STATE_MEM_POOL *mem_pool);
bool master_bin_table_lock_surr(MASTER_BIN_TABLE *table, uint32 surr);

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
  queue_u64_init(&table_aux->deletions);
  queue_u32_init(&table_aux->deletions_1);
  queue_3u32_init(&table_aux->insertions);
  queue_u32_init(&table_aux->reinsertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

void sym_master_bin_table_aux_reset(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  queue_u64_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->deletions_1);
  queue_3u32_reset(&table_aux->insertions);
  queue_u32_reset(&table_aux->reinsertions);
  table_aux->last_surr = 0xFFFFFFFF;
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_clear(SYM_MASTER_BIN_TABLE_AUX *table_aux) {
  table_aux->clear = true;
}

void sym_master_bin_table_aux_delete(SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_u64_insert(&table_aux->deletions, pack_args(arg1, arg2));
}

void sym_master_bin_table_aux_delete_1(SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions_1, arg1);
}

uint32 sym_master_bin_table_aux_insert(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  sort_args(arg1, arg2);
  uint32 surr = sym_master_bin_table_lookup_surr(table, arg1, arg2);

  if (surr != 0xFFFFFFFF) {
    queue_u32_insert(&table_aux->reinsertions, surr);
    return surr;
  }

  uint32 count = table_aux->insertions.count_;
  if (count > 0) {
    //## BAD BAD BAD: IMPLEMENT FOR REAL
    uint32 (*ptr)[3] = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 curr_arg1 = (*ptr)[0];
      uint32 curr_arg2 = (*ptr)[1];
      if (curr_arg1 == arg1 & curr_arg2 == arg2) {
        surr = (*ptr)[2];
        break;
      }
      ptr++;
    }
  }

  if (surr == 0xFFFFFFFF)
    surr = master_bin_table_get_next_free_surr(table, table_aux->last_surr);

  table_aux->last_surr = surr;
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
  uint32 count = table_aux->insertions.count_;
  uint32 (*ptr)[3] = table_aux->insertions.array;
  for (uint32 i=0 ; i < count ; i++) {
    uint32 curr_arg1 = (*ptr)[0];
    uint32 curr_arg2 = (*ptr)[1];
    if (curr_arg1 == arg1 & curr_arg2 == arg2)
      return (*ptr)[2];
    ptr++;
  }

  //## BUG BUG BUG: NOT SURE THIS CANNOT HAPPEN
  //## TRY TO TRICK THE CODE ALL THE WAY HERE
  internal_fail();
}

////////////////////////////////////////////////////////////////////////////////

void sym_master_bin_table_aux_apply(MASTER_BIN_TABLE *table, SYM_MASTER_BIN_TABLE_AUX *table_aux, void (*incr_rc)(void *, uint32), void (*decr_rc)(void *, void *, uint32), void *store, void *store_aux, STATE_MEM_POOL *mem_pool) {
  //## COPY AND ADJUST THE NON-SYMMETRIC VERSION AFTER GETTING RID OF ITERATORS
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}
