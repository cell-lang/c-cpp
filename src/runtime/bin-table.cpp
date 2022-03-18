#include "lib.h"
#include "one-way-bin-table.h"


void bin_table_init(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_init(&table->forward, mem_pool);
  one_way_bin_table_init(&table->backward, mem_pool);
}

uint32 bin_table_size(BIN_TABLE *table) {
  return table->forward.count;
}

bool bin_table_contains(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  return one_way_bin_table_contains(&table->forward, arg1, arg2);
}

bool bin_table_contains_1(BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_contains_key(&table->forward, arg1);
}

bool bin_table_contains_2(BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_contains_key(&table->backward, arg2);
}

uint32 bin_table_count_1(BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_get_count(&table->forward, arg1);
}

uint32 bin_table_count_2(BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_get_count(&table->backward, arg2);
}

uint32 bin_table_restrict_1(BIN_TABLE *table, uint32 arg1, uint32 *args2) {
  return one_way_bin_table_restrict(&table->forward, arg1, args2);
}

uint32 bin_table_restrict_2(BIN_TABLE *table, uint32 arg2, uint32 *args1) {
  return one_way_bin_table_restrict(&table->backward, arg2, args1);
}

uint32 bin_table_lookup_1(BIN_TABLE *table, uint32 arg1) {
  return one_way_bin_table_lookup(&table->forward, arg1);
}

uint32 bin_table_lookup_2(BIN_TABLE *table, uint32 arg2) {
  return one_way_bin_table_lookup(&table->backward, arg2);
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_insert(BIN_TABLE *table, uint32 arg1, uint32 arg2, STATE_MEM_POOL *mem_pool) {
  //## WHY IS THIS VERSION FASTER?
  if (!one_way_bin_table_contains(&table->forward, arg1, arg2)) {
    one_way_bin_table_insert_unique(&table->forward, arg1, arg2, mem_pool);
    one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
    return true;
  }
  // if (one_way_bin_table_insert(&table->forward, arg1, arg2, mem_pool)) {
  //   //## NOT SURE IF THE UNIQUE VERSION IS ANY BETTER (OR MAYBE EVEN SLIGHTLY WORSE?) THAN THE STANDARD ONE
  //   one_way_bin_table_insert_unique(&table->backward, arg2, arg1, mem_pool);
  //   return true;
  // }
  return false;
}

bool bin_table_delete(BIN_TABLE *table, uint32 arg1, uint32 arg2) {
  bool found = one_way_bin_table_delete(&table->forward, arg1, arg2);
  if (found) {
    bool found_ = one_way_bin_table_delete(&table->backward, arg2, arg1);
    assert(found_ == found);
  }
  return found;
}

void bin_table_delete_1(BIN_TABLE *table, uint32 arg1) {
  uint32 count = bin_table_count_1(table, arg1);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *args2;
    if (count <= 1024)
      args2 = inline_array;
    else
      args2 = new_uint32_array(count);

    one_way_bin_table_delete_by_key(&table->forward, arg1, args2);

    for (uint32 i=0 ; i < count ; i++) {
      bool found = one_way_bin_table_delete(&table->backward, args2[i], arg1);
      assert(found);
    }
  }
  // return count;
}

void bin_table_delete_2(BIN_TABLE *table, uint32 arg2) {
  uint32 count = bin_table_count_2(table, arg2);
  if (count > 0) {
    uint32 inline_array[1024];
    uint32 *args1;
    if (count <= 1024)
      args1 = inline_array;
    else
      args1 = new_uint32_array(count);

    one_way_bin_table_delete_by_key(&table->backward, arg2, args1);

    for (uint32 i=0 ; i < count ; i++) {
      bool found = one_way_bin_table_delete(&table->forward, args1[i], arg2);
      assert(found);
    }
  }
  // return count;
}

void bin_table_clear(BIN_TABLE *table, STATE_MEM_POOL *mem_pool) {
  one_way_bin_table_clear(&table->forward);
  one_way_bin_table_clear(&table->backward);
}

////////////////////////////////////////////////////////////////////////////////

bool bin_table_col_1_is_key(BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->forward);
}

bool bin_table_col_2_is_key(BIN_TABLE *table) {
  return one_way_bin_table_is_map(&table->backward);
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_copy_to(BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, STREAM *strm_1, STREAM *strm_2) {
  BIN_TABLE_ITER iter;
  bin_table_iter_init(table, &iter);
  while (!bin_table_iter_is_out_of_range(&iter)) {
    uint32 arg1 = bin_table_iter_get_1(&iter);
    uint32 arg2 = bin_table_iter_get_2(&iter);
    OBJ obj1 = surr_to_obj_1(store_1, arg1);
    OBJ obj2 = surr_to_obj_2(store_2, arg2);
    append(*strm_1, obj1);
    append(*strm_2, obj2);
    bin_table_iter_move_forward(&iter);
  }
}

void bin_table_write(WRITE_FILE_STATE *write_state, BIN_TABLE *table, OBJ (*surr_to_obj_1)(void *, uint32), void *store_1, OBJ (*surr_to_obj_2)(void *, uint32), void *store_2, bool as_map, bool flipped) {
  uint32 count = bin_table_size(table);
  uint32 idx = 0;

  BIN_TABLE_ITER iter;
  bin_table_iter_init(table, &iter);

  while (!bin_table_iter_is_out_of_range(&iter)) {
    uint32 arg1 = bin_table_iter_get_1(&iter);
    uint32 arg2 = bin_table_iter_get_2(&iter);
    OBJ obj1 = surr_to_obj_1(store_1, arg1);
    OBJ obj2 = surr_to_obj_2(store_2, arg2);

    write_str(write_state, "\n    ");
    write_obj(write_state, flipped ? obj2 : obj1);
    write_str(write_state, as_map ? " -> " : ", ");
    write_obj(write_state, flipped ? obj1 : obj2);
    if (++idx != count)
      write_str(write_state, as_map ? "," : ";");

    bin_table_iter_move_forward(&iter);
  }
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_iter_init_empty(BIN_TABLE_ITER *iter) {
  iter->left = 0;
  //## THESE ARE ALL UNNECESSARY
#ifndef NDEBUG
  iter->table = NULL;
  iter->arg2s = NULL;
  iter->capacity = 0;
  iter->idx_last = 0;
  iter->arg1 = 0xFFFFFFFFU;
  memset(iter->inline_array, 0, BIN_TABLE_ITER_INLINE_SIZE * sizeof(uint32));
#endif
}

void bin_table_iter_init(BIN_TABLE *table, BIN_TABLE_ITER *iter) {
#ifndef NDEBUG
  memset(iter->inline_array, 0, BIN_TABLE_ITER_INLINE_SIZE * sizeof(uint32));
#endif

  uint32 count = bin_table_size(table);
  if (count > 0) {
    iter->table = table;
    iter->left = count;

    uint32 arg1 = 0;
    uint32 count1;
    for ( ; ; ) {
      count1 = bin_table_count_1(table, arg1);
      if (count1 != 0)
        break;
      arg1++;
    }

    iter->arg1 = arg1;
    iter->idx_last = count1 - 1;

    uint32 *arg2s;
    if (count1 <= BIN_TABLE_ITER_INLINE_SIZE) {
      iter->capacity = BIN_TABLE_ITER_INLINE_SIZE;
      arg2s = iter->inline_array;
    }
    else {
      uint32 capacity = 2 * BIN_TABLE_ITER_INLINE_SIZE;
      while (count1 > capacity)
        capacity *= 2;
      iter->capacity = capacity;
      arg2s = new_uint32_array(capacity);
    }
    iter->arg2s = arg2s;

    uint32 count1_ = bin_table_restrict_1(table, arg1, arg2s);
    assert(count1_ == count1);
  }
  else
    bin_table_iter_init_empty(iter);
}

void bin_table_iter_move_forward(BIN_TABLE_ITER *iter) {
  assert(!bin_table_iter_is_out_of_range(iter));

  uint32 left = iter->left - 1;
  iter->left = left;
  if (left == 0)
    return;

  uint32 idx_last = iter->idx_last;
  if (idx_last > 0) {
    iter->idx_last = idx_last - 1;
    return;
  }

  BIN_TABLE *table = iter->table;

  uint32 arg1 = iter->arg1 + 1;
  uint32 count1;
  for ( ; ; ) {
    count1 = bin_table_count_1(table, arg1);
    if (count1 != 0)
      break;
    arg1++;
  }

  iter->arg1 = arg1;
  iter->idx_last = count1 - 1;

  uint32 capacity = iter->capacity;
  uint32 *arg2s;
  if (count1 <= capacity) {
    arg2s = iter->arg2s;
  }
  else {
    do
      capacity *= 2;
    while (count1 > capacity);
    iter->capacity = capacity;
    arg2s = new_uint32_array(capacity);
    iter->arg2s = arg2s;
  }

  uint32 count_ = bin_table_restrict_1(table, arg1, arg2s);
  assert(count_ == count1);
}

bool bin_table_iter_is_out_of_range(BIN_TABLE_ITER *iter) {
  return iter->left == 0;
}

uint32 bin_table_iter_get_1(BIN_TABLE_ITER *iter) {
  assert(!bin_table_iter_is_out_of_range(iter));
  return iter->arg1;
}

uint32 bin_table_iter_get_2(BIN_TABLE_ITER *iter) {
  assert(!bin_table_iter_is_out_of_range(iter));
  return iter->arg2s[iter->idx_last];
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_iter_1_init_empty(BIN_TABLE_ITER_1 *iter) {
  iter->args = NULL; //## UNNECESSARY
  iter->left = 0;
}

void bin_table_iter_1_init(BIN_TABLE *table, BIN_TABLE_ITER_1 *iter, uint32 arg1) {
  uint32 count = bin_table_count_1(table, arg1);
  if (count > 0) {
    uint32 *arg2s;
    if (count <= BIN_TABLE_ITER_INLINE_SIZE)
      arg2s = iter->inline_array;
    else
      arg2s = new_uint32_array(count);

    uint32 count_ = bin_table_restrict_1(table, arg1, arg2s);
    assert(count_ == count);

    iter->args = arg2s;
    iter->left = count;
  }
  else {
    iter->args = NULL; //## UNNECESSARY
    iter->left = 0;
  }
}

void bin_table_iter_1_move_forward(BIN_TABLE_ITER_1 *iter) {
  assert(!bin_table_iter_1_is_out_of_range(iter));
  iter->args++;
  iter->left--;
}

bool bin_table_iter_1_is_out_of_range(BIN_TABLE_ITER_1 *iter) {
  return iter->left == 0;
}

uint32 bin_table_iter_1_get_1(BIN_TABLE_ITER_1 *iter) {
  assert(!bin_table_iter_1_is_out_of_range(iter));
  return *iter->args;
}

////////////////////////////////////////////////////////////////////////////////

void bin_table_iter_2_init_empty(BIN_TABLE_ITER_2 *iter) {
  iter->args = NULL; //## UNNECESSARY
  iter->left = 0;
}

void bin_table_iter_2_init(BIN_TABLE *table, BIN_TABLE_ITER_2 *iter, uint32 arg2) {
  uint32 count = bin_table_count_2(table, arg2);
  if (count > 0) {
    uint32 *arg1s;
    if (count <= BIN_TABLE_ITER_INLINE_SIZE)
      arg1s = iter->inline_array;
    else
      arg1s = new_uint32_array(count);

    uint32 count_ = bin_table_restrict_2(table, arg2, arg1s);
    assert(count_ == count);

    iter->args = arg1s;
    iter->left = count;
  }
  else {
    iter->args = NULL; //## UNNECESSARY
    iter->left = 0;
  }
}

void bin_table_iter_2_move_forward(BIN_TABLE_ITER_2 *iter) {
  assert(!bin_table_iter_2_is_out_of_range(iter));
  iter->args++;
  iter->left--;
}

bool bin_table_iter_2_is_out_of_range(BIN_TABLE_ITER_2 *iter) {
  return iter->left == 0;
}

uint32 bin_table_iter_2_get_1(BIN_TABLE_ITER_2 *iter) {
  assert(!bin_table_iter_2_is_out_of_range(iter));
  return *iter->args;
}
