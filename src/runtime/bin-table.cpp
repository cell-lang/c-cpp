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

void bin_table_delete_1(BIN_TABLE *table, uint32 arg1) {
  unordered_set<uint32> &args2_set = table->forward[arg1];
  table->count -= args2_set.size();
  uint32 idx = 0;
  for (unordered_set<uint32>::iterator it = args2_set.begin() ; it != args2_set.end() ; it++)
    table->backward[*it].erase(arg1);
  table->forward.erase(arg1);
}

void bin_table_delete_2(BIN_TABLE *table, uint32 arg2) {
  unordered_set<uint32> &args1_set = table->backward[arg2];
  table->count -= args1_set.size();
  uint32 idx = 0;
  for (unordered_set<uint32>::iterator it = args1_set.begin() ; it != args1_set.end() ; it++)
    table->forward[*it].erase(arg2);
  table->backward.erase(arg2);
}

void bin_table_clear(BIN_TABLE *table) {
  table->forward.clear();
  table->backward.clear();
  table->count = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_col_1_is_key(BIN_TABLE *table) {
  for (unordered_map<uint32, unordered_set<uint32>>::iterator it = table->forward.begin() ; it != table->forward.end() ; it++)
    if (it->second.size() > 1)
      return false;
  return true;
}

bool bin_table_col_2_is_key(BIN_TABLE *table) {
  for (unordered_map<uint32, unordered_set<uint32>>::iterator it = table->backward.begin() ; it != table->backward.end() ; it++)
    if (it->second.size() > 1)
      return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_copy_to(BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool flipped, STREAM *strm_1, STREAM *strm_2) {
  for (unordered_map<uint32, unordered_set<uint32>>::iterator it1 = table->forward.begin() ; it1 != table->forward.end() ; it1++) {
    OBJ obj1 = surr_to_obj_1(store_1, it1->first);
    for (unordered_set<uint32>::iterator it2 = it1->second.begin() ; it2 != it1->second.end() ; it2++) {
      OBJ obj2 = surr_to_obj_2(store_2, *it2);
      append(*strm_1, flipped ? obj2 : obj1);
      append(*strm_2, flipped ? obj1 : obj2);
    }
  }
}

void bin_table_write(WRITE_FILE_STATE *write_state, BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool flipped) {
  uint32 count = table->count;
  bool is_map = flipped ? bin_table_col_2_is_key(table) : bin_table_col_1_is_key(table);
  uint32 idx = 0;
  for (unordered_map<uint32, unordered_set<uint32>>::iterator it1 = table->forward.begin() ; it1 != table->forward.end() ; it1++) {
    OBJ obj1 = surr_to_obj_1(store_1, it1->first);
    for (unordered_set<uint32>::iterator it2 = it1->second.begin() ; it2 != it1->second.end() ; it2++) {
      OBJ obj2 = surr_to_obj_2(store_2, *it2);
      write_str(write_state, "\n    ");
      write_obj(write_state, flipped ? obj2 : obj1);
      write_str(write_state, is_map ? " -> " : ", ");
      write_obj(write_state, flipped ? obj1 : obj2);
      if (++idx != count)
        write_str(write_state, is_map ? "," : ";");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_iter_init(BIN_TABLE *table, BIN_TABLE_ITER *iter) {
  uint32 count = table->count;
  if (count == 0) {
    iter->args = NULL;
    iter->left = 0;
    iter->single_col = false; //## UNNECESSARY
  }

  uint32 *args = new_uint32_array(count);
  iter->args = args;
  iter->left = count;
  iter->single_col = false;

  for (unordered_map<uint32, unordered_set<uint32>>::iterator it1 = table->forward.begin() ; it1 != table->forward.end() ; it1++) {
    uint32 arg1 = it1->first;
    for (unordered_set<uint32>::iterator it2 = it1->second.begin() ; it2 != it1->second.end() ; it2++) {
      uint32 arg2 = *it2;
      *(args++) = arg1;
      *(args++) = arg2;
    }
  }
}

void bin_table_iter_init_1(BIN_TABLE *table, BIN_TABLE_ITER *iter, uint32 arg1) {
  uint32 count = bin_table_count_1(table, arg1);
  uint32 *args;
  if (count > 0) {
    args = new_uint32_array(count);
    bin_table_restrict_1(table, arg1, args);
  }
  else
    args = NULL;
  iter->args = args;
  iter->left = count;
  iter->single_col = true;
}

void bin_table_iter_init_2(BIN_TABLE *table, BIN_TABLE_ITER *iter, uint32 arg2) {
  uint32 count = bin_table_count_2(table, arg2);
  uint32 *args;
  if (count > 0) {
    args = new_uint32_array(count);
    bin_table_restrict_2(table, arg2, args);
  }
  else
    args = NULL;
  iter->args = args;
  iter->left = count;
  iter->single_col = true;
}

bool bin_table_iter_is_out_of_range(BIN_TABLE_ITER *iter) {
  return iter->left == 0;
}

uint32 bin_table_iter_get_1(BIN_TABLE_ITER *iter) {
  assert(!bin_table_iter_is_out_of_range(iter));
  return *iter->args;
}

uint32 bin_table_iter_get_2(BIN_TABLE_ITER *iter) {
  assert(!bin_table_iter_is_out_of_range(iter));
  return *(iter->args + 1);
}

void bin_table_iter_move_forward(BIN_TABLE_ITER *iter) {
  assert(!bin_table_iter_is_out_of_range(iter));
  iter->args += iter->single_col ? 1 : 2;
  iter->left--;
}
