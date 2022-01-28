#include "lib.h"


void bin_table_init(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  table->count = 0;
}

uint32 bin_table_size(BIN_TABLE *table) {
  return table->count;
}

bool bin_table_contains(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return table->forward[arg1].count(arg2) != 0;
}

bool bin_table_contains_1(BIN_TABLE *table, uint32 arg1) {
  return table->forward.count(arg1) != 0;
}

bool bin_table_contains_2(BIN_TABLE *table, uint32 arg2) {
  return table->backward.count(arg2) != 0;
}

uint32 bin_table_count_1(BIN_TABLE *table, uint32 arg1) {
  return table->forward[arg1].size();
}

uint32 bin_table_count_2(BIN_TABLE *table, uint32 arg2) {
  return table->backward[arg2].size();
}

// uint32 *bin_table_restrict_1(BIN_TABLE *table, uint32 arg) {

// }

uint32 bin_table_restrict_1(BIN_TABLE *table, uint32 arg1, uint32 *args2) {
  unordered_set<uint32> &args2_set = table->forward[arg1];
  uint32 count = 0;
  for (unordered_set<uint32>::iterator it = args2_set.begin() ; it != args2_set.end() ; it++)
    args2[count++] = *it;
  return count;
}

// uint32 *bin_table_restrict_2(BIN_TABLE *table, uint32 arg) {

// }

uint32 bin_table_restrict_2(BIN_TABLE *table, uint32 arg2, uint32 *args1) {
  unordered_set<uint32> &args1_set = table->backward[arg2];
  uint32 count = 0;
  for (unordered_set<uint32>::iterator it = args1_set.begin() ; it != args1_set.end() ; it++)
    args1[count++] = *it;
  return count;
}

uint32 bin_table_lookup_1(BIN_TABLE *table, uint32 arg1) {
  unordered_set<uint32> &args2_set = table->forward[arg1];
  if (args2_set.size() == 0)
    return 0xFFFFFFFF;
  if (args2_set.size() > 1)
    soft_fail(NULL);
  return *args2_set.begin();
}

uint32 bin_table_lookup_2(BIN_TABLE *table, uint32 arg2) {
  unordered_set<uint32> &args1_set = table->backward[arg2];
  if (args1_set.size() == 0);
    return 0xFFFFFFFF;
  if (args1_set.size() > 1)
    soft_fail(NULL);
  return *args1_set.begin();
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_insert(BIN_TABLE *table, STATE_MEM_POOL *, uint32 arg1, uint32 arg2) {
  if (table->forward[arg1].count(arg2) == 0) {
    table->forward[arg1].insert(arg2);
    table->backward[arg2].insert(arg1);
    table->count++;
  }
}

bool bin_table_delete(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  if (table->forward[arg1].count(arg2) != 0) {
    table->forward[arg1].erase(arg2);
    table->backward[arg2].erase(arg1);
    table->count--;
    return true;
  }
  return false;
}

void bin_table_delete_1(BIN_TABLE *table, uint32 arg1, uint32 *args2) {
  unordered_set<uint32> &args2_set = table->forward[arg1];
  table->count -= args2_set.size();
  uint32 idx = 0;
  for (unordered_set<uint32>::iterator it = args2_set.begin() ; it != args2_set.end() ; it++) {
    args2[idx++] = *it;
    table->backward[*it].erase(arg1);
  }
  table->forward.erase(arg1);
}

void bin_table_delete_2(BIN_TABLE *table, uint32 arg2, uint32 *args1) {
  unordered_set<uint32> &args1_set = table->backward[arg2];
  table->count -= args1_set.size();
  uint32 idx = 0;
  for (unordered_set<uint32>::iterator it = args1_set.begin() ; it != args1_set.end() ; it++) {
    args1[idx++] = *it;
    table->forward[*it].erase(arg2);
  }
  table->backward.erase(arg2);
}

void bin_table_clear(BIN_TABLE *table) {
  table->forward.clear();
  table->backward.clear();
  table->count = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_col_1_is_key(BIN_TABLE *table) {

}

bool bin_table_col_2_is_key(BIN_TABLE *table) {

}

////////////////////////////////////////////////////////////////////////////////

void bin_table_copy_to(BIN_TABLE *, OBJ (*)(void *, uint32), void *, OBJ (*)(void *, uint32), void *, bool flipped, STREAM *, STREAM *) {

}

void bin_table_write(WRITE_FILE_STATE *, BIN_TABLE *, OBJ (*)(void *, uint32), void *, OBJ (*)(void *, uint32), void *, bool flipped) {

}

////////////////////////////////////////////////////////////////////////////////


void bin_table_iter_init(BIN_TABLE *table, BIN_TABLE_ITER *iter) {

}

void bin_table_iter_init_1(BIN_TABLE *table, BIN_TABLE_ITER *iter, uint32 arg1) {

}

void bin_table_iter_init_2(BIN_TABLE *table, BIN_TABLE_ITER *iter, uint32 arg2) {

}

bool bin_table_iter_is_out_of_range(BIN_TABLE *table) {

}

uint32 bin_table_iter_get_1(BIN_TABLE *table) {

}

uint32 bin_table_iter_get_2(BIN_TABLE *table) {

}

void bin_table_iter_move_forward(BIN_TABLE *table) {

}
