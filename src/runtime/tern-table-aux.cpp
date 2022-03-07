#include "lib.h"


const uint32 DELETE_12_TAG  = 1;
const uint32 DELETE_13_TAG  = 2;
const uint32 DELETE_23_TAG  = 3;
const uint32 DELETE_1_TAG   = 4;
const uint32 DELETE_2_TAG   = 5;
const uint32 DELETE_3_TAG   = 6;

const uint32 TAG_CLEAR_MASK = 0x1FFFFFFF;


void queue_u32_init(QUEUE_U32 *queue);
void queue_u32_insert(QUEUE_U32 *queue, uint32 value);
void queue_u32_prepare(QUEUE_U32 *queue);
void queue_u32_reset(QUEUE_U32 *queue);
bool queue_u32_contains(QUEUE_U32 *queue, uint32 value);

////////////////////////////////////////////////////////////////////////////////

void queue_u64_init(QUEUE_U64 *);
void queue_u64_insert(QUEUE_U64 *, uint64);
void queue_u64_prepare(QUEUE_U64 *);
void queue_u64_flip_words(QUEUE_U64 *);
void queue_u64_reset(QUEUE_U64 *);
bool queue_u64_contains(QUEUE_U64 *, uint64);

////////////////////////////////////////////////////////////////////////////////

void queue_3u32_insert(QUEUE_U32 *queue, uint32 value1, uint32 value2, uint32 value3);

void queue_2u32_insert(QUEUE_U32 *queue, uint32 value1, uint32 value2) {
  uint32 capacity = queue->capacity;
  uint32 count = queue->count;
  uint32 *array = queue->array;
  assert(count <= capacity);
  if (count + 2 >= capacity) {
    assert(2 * capacity > count + 2);
    array = resize_uint32_array(array, capacity, 2 * capacity);
    queue->capacity = 2 * capacity;
    queue->array = array;
  }
  array[count] = value1;
  array[count + 1] = value2;
  queue->count = count + 2;
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_init(TERN_TABLE_AUX *table_aux, STATE_MEM_POOL *mem_pool) {
  queue_u32_init(&table_aux->deletions);
  queue_u32_init(&table_aux->insertions);
  table_aux->clear = false;
}

void tern_table_aux_reset(TERN_TABLE_AUX *table_aux) {
  queue_u32_reset(&table_aux->deletions);
  queue_u32_reset(&table_aux->insertions);
  table_aux->clear = false;
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_clear(TERN_TABLE_AUX *table_aux) {
  table_aux->clear = false;
}

void tern_table_aux_delete(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  queue_3u32_insert(&table_aux->deletions, arg1, arg2, arg3);
}

void tern_table_aux_delete_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2) {
  queue_2u32_insert(&table_aux->deletions, (DELETE_12_TAG << 29) | arg1, arg2);
}

void tern_table_aux_delete_13(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg3) {
  queue_2u32_insert(&table_aux->deletions, (DELETE_13_TAG << 29) | arg1, arg3);
}

void tern_table_aux_delete_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg2, uint32 arg3) {
  queue_2u32_insert(&table_aux->deletions, (DELETE_23_TAG << 29) | arg2, arg3);
}

void tern_table_aux_delete_1(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg1) {
  queue_u32_insert(&table_aux->deletions, (DELETE_1_TAG << 29)| arg1);
}

void tern_table_aux_delete_2(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg2) {
  queue_u32_insert(&table_aux->deletions, (DELETE_2_TAG << 29)| arg2);
}

void tern_table_aux_delete_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, uint32 arg3) {
  queue_u32_insert(&table_aux->deletions, (DELETE_3_TAG << 29)| arg3);
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_insert(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3) {
  queue_3u32_insert(&table_aux->insertions, arg1, arg2, arg3);
}

////////////////////////////////////////////////////////////////////////////////

inline void delete_123(TERN_TABLE *table, uint32 arg1, uint32 arg2, uint32 arg3, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    if (bin_table_delete(&table->slave, surr12, arg3)) {
      if (bin_table_count_1(&table->slave, surr12) == 0) {
        bool found = master_bin_table_delete(&table->master, arg1, arg2); //## MAYBE IT WOULD BE FASTER TO PROVIDE THE SURROGATE INSTEAD?
        assert(found);
        decr_rc_1(store_1, store_aux_1, arg1);
        decr_rc_2(store_2, store_aux_2, arg2);
      }
      decr_rc_3(store_3, store_aux_3, arg3);
    }
  }
}

inline void delete_12(TERN_TABLE *table, uint32 arg1, uint32 arg2, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, arg2);
  if (surr12 != 0xFFFFFFFF) {
    master_bin_table_delete(&table->master, arg1, arg2);
    decr_rc_1(store_1, store_aux_1, arg1);
    decr_rc_2(store_2, store_aux_2, arg2);

    uint32 count = bin_table_count_1(&table->slave, surr12);
    assert(count > 0);
    uint32 inline_array[256];
    uint32 *arg3s = count <= 256 ? inline_array : new_uint32_array(count);
    bin_table_restrict_1(&table->slave, surr12, arg3s);
    bin_table_delete_1(&table->slave, surr12);
    for (uint32 i=0 ; i < count ; i++)
      decr_rc_3(store_3, store_aux_3, arg3s[i]);
  }
}

inline void delete_13(TERN_TABLE *table, uint32 arg1, uint32 arg3, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 count1 = master_bin_table_count_1(&table->master, arg1);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count1 > 0 && count3 > 0) {
    if (count1 < count3) {
      uint32 master_inline_array[512];
      uint32 *arg2s = count1 <= 256 ? master_inline_array : new_uint32_array(2 * count1);
      uint32 *surr12s = arg2s + count1;
      master_bin_table_restrict_1(&table->master, arg1, arg2s, surr12s);
      for (uint32 i=0 ; i < count1 ; i++) {
        uint32 surr12 = surr12s[i];
        if (bin_table_delete(&table->slave, surr12, arg3)) {
          decr_rc_3(store_3, store_aux_3, arg3);
          if (!bin_table_contains_1(&table->slave, surr12)) {
            uint32 arg2 = arg2s[i];
            master_bin_table_delete(&table->master, arg1, arg2);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
          }
        }
      }
    }
    else {
      uint32 slave_inline_array[256];
      uint32 *surr12s = count3 <= 256 ? slave_inline_array : new_uint32_array(count3);
      bin_table_restrict_2(&table->slave, arg3, surr12s);
      for (uint32 i=0 ; i < count3 ; i++) {
        uint32 surr12 = surr12s[i];
        if (master_bin_table_get_arg_1(&table->master, surr12) == arg1) {
          bin_table_delete(&table->slave, surr12, arg3);
          decr_rc_3(store_3, store_aux_3, arg3);
          if (!bin_table_contains_1(&table->slave, surr12)) {
            uint32 arg2 = master_bin_table_get_arg_2(&table->master, surr12);
            master_bin_table_delete(&table->master, arg1, arg2);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
          }
        }
      }
    }
  }
}

inline void delete_23(TERN_TABLE *table, uint32 arg2, uint32 arg3, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 count2 = master_bin_table_count_2(&table->master, arg2);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count2 > 0 && count3 > 0) {
    if (count2 < count3) {
      uint32 master_inline_array[256];
      uint32 *arg1s = count2 <= 256 ? master_inline_array : new_uint32_array(2 * count2);
      master_bin_table_restrict_2(&table->master, arg2, arg1s);
      for (uint32 i=0 ; i < count2 ; i++) {
        uint32 arg1 = arg1s[i];
        uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, arg2);
        if (bin_table_delete(&table->slave, surr12, arg3)) {
          decr_rc_3(store_3, store_aux_3, arg3);
          if (!bin_table_contains_1(&table->slave, surr12)) {
            uint32 arg1 = arg1s[i];
            master_bin_table_delete(&table->master, arg1, arg2);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
          }
        }
      }
    }
    else {
      uint32 slave_inline_array[256];
      uint32 *surr12s = count3 <= 256 ? slave_inline_array : new_uint32_array(count3);
      bin_table_restrict_2(&table->slave, arg3, surr12s);
      for (uint32 i=0 ; i < count3 ; i++) {
        uint32 surr12 = surr12s[i];
        if (master_bin_table_get_arg_2(&table->master, surr12) == arg2) {
          bin_table_delete(&table->slave, surr12, arg3);
          decr_rc_3(store_3, store_aux_3, arg3);
          if (!bin_table_contains_1(&table->slave, surr12)) {
            uint32 arg1 = master_bin_table_get_arg_1(&table->master, surr12);
            master_bin_table_delete(&table->master, arg1, arg2);
            decr_rc_1(store_1, store_aux_1, arg1);
            decr_rc_2(store_2, store_aux_2, arg2);
          }
        }
      }
    }
  }
}

inline void delete_1(TERN_TABLE *table, uint32 arg1, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 master_count = master_bin_table_count_1(&table->master, arg1);
  if (master_count > 0) {
    uint32 master_inline_array[512];
    uint32 *arg2s = master_count <= 256 ? master_inline_array : new_uint32_array(2 * master_count);
    uint32 *surr12s = arg2s + master_count;
    master_bin_table_restrict_1(&table->master, arg1, arg2s, surr12s);
    master_bin_table_delete_1(&table->master, arg1);
    for (uint32 i=0 ; i < master_count ; i++) {
      decr_rc_1(store_1, store_aux_1, arg1); //## WOULD BE BETTER IF WE COULD DO IT IN JUST A SINGLE OPERATION
      decr_rc_2(store_2, store_aux_2, arg2s[i]);
      uint32 surr12 = surr12s[i];
      uint32 slave_count = bin_table_count_1(&table->slave, surr12);
      assert(slave_count > 0);
      uint32 slave_inline_array[256];
      uint32 *arg3s = slave_count <= 256 ? slave_inline_array : new_uint32_array(slave_count); //## BAD BAD BAD
      bin_table_restrict_1(&table->slave, surr12, arg3s);
      bin_table_delete_1(&table->slave, surr12);
      for (uint32 j=0 ; j < slave_count ; j++)
        decr_rc_3(store_3, store_aux_3, arg3s[j]);
    }
  }
}

inline void delete_2(TERN_TABLE *table, uint32 arg2, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 master_count = master_bin_table_count_2(&table->master, arg2);
  if (master_count > 0) {
    uint32 master_inline_array[256];
    uint32 *arg1s = master_count <= 256 ? master_inline_array : new_uint32_array(master_count);
    master_bin_table_restrict_2(&table->master, arg2, arg1s);
    master_bin_table_delete_2(&table->master, arg2);
    for (uint32 i=0 ; i < master_count ; i++) {
      uint32 arg1 = arg1s[i];
      decr_rc_1(store_1, store_aux_1, arg1);
      decr_rc_2(store_2, store_aux_2, arg2); //## WOULD BE BETTER IF WE COULD DO IT IN JUST A SINGLE OPERATION
      uint32 surr12 = master_bin_table_lookup_surrogate(&table->master, arg1, arg2);
      uint32 slave_count = bin_table_count_1(&table->slave, surr12);
      assert(slave_count > 0);
      uint32 slave_inline_array[256];
      uint32 *arg3s = slave_count <= 256 ? slave_inline_array : new_uint32_array(slave_count); //## BAD BAD BAD
      bin_table_restrict_1(&table->slave, surr12, arg3s);
      bin_table_delete_1(&table->slave, surr12);
      for (uint32 j=0 ; j < slave_count ; j++)
        decr_rc_3(store_3, store_aux_3, arg3s[j]);
    }
  }
}

inline void delete_3(TERN_TABLE *table, uint32 arg3, void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3) {
  uint32 slave_count = bin_table_count_2(&table->slave, arg3);
  if (slave_count > 0) {
    uint32 slave_inline_array[256];
    uint32 *surr12s = slave_count <= 256 ? slave_inline_array : new_uint32_array(slave_count);
    bin_table_restrict_2(&table->slave, arg3, surr12s);
    bin_table_delete_2(&table->slave, arg3);
    for (uint32 i=0 ; i < slave_count ; i++)
    for (uint32 i=0 ; i < slave_count ; i++) {
      uint32 surr12 = surr12s[i];
      if (!bin_table_contains_1(&table->slave, surr12)) {
        uint32 arg1 = master_bin_table_get_arg_1(&table->master, surr12);
        uint32 arg2 = master_bin_table_get_arg_2(&table->master, surr12);
        master_bin_table_delete(&table->master, arg1, arg2);
        decr_rc_1(store_1, store_aux_1, arg1);
        decr_rc_2(store_2, store_aux_2, arg2);
      }
      decr_rc_3(store_3, store_aux_3, arg3); //## WOULD BE BETTER IF WE COULD DO IT IN JUST A SINGLE OPERATION
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_aux_apply(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, void (*incr_rc_1)(void *, uint32), void (*decr_rc_1)(void *, void *, uint32), void *store_1, void *store_aux_1, void (*incr_rc_2)(void *, uint32), void (*decr_rc_2)(void *, void *, uint32), void *store_2, void *store_aux_2, void (*incr_rc_3)(void *, uint32), void (*decr_rc_3)(void *, void *, uint32), void *store_3, void *store_aux_3, STATE_MEM_POOL *mem_pool) {
  uint32 count = table_aux->deletions.count;
  if (count > 0) {
    uint32 *ptr = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; ) {
      uint32 word1 = ptr[i++];
      uint32 tag = word1 >> 29;
      word1 = word1 & TAG_CLEAR_MASK;

      if (tag <= DELETE_2_TAG) {
        assert(tag == 0 | tag == DELETE_12_TAG | tag == DELETE_13_TAG | tag == DELETE_23_TAG);
        uint32 word2 = ptr[i++];

        if (tag <= DELETE_12_TAG) {
          if (tag == 0) {
            uint32 word3 = ptr[i++];
            delete_123(table, word1, word2, word3, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
          }
          else {
            assert(tag == DELETE_12_TAG);
            delete_12(table, word1, word2, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
          }
        }
        else {
          if (tag == DELETE_13_TAG) {
            delete_13(table, word1, word2, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
          }
          else {
            assert(tag == DELETE_23_TAG);
            delete_23(table, word1, word2, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
          }
        }
      }
      else {
        assert(tag == DELETE_1_TAG | tag == DELETE_2_TAG | tag == DELETE_3_TAG);

        if (tag == DELETE_1_TAG) {
          delete_1(table, word1, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
        }
        else if (tag == DELETE_2_TAG) {
          delete_2(table, word1, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
        }
        else {
          assert(tag == DELETE_3_TAG);
          delete_3(table, word1, decr_rc_1, store_1, store_aux_1, decr_rc_2, store_2, store_aux_2, decr_rc_3, store_3, store_aux_3);
        }
      }
    }
  }

  count = table_aux->insertions.count / 3;
  if (count > 0) {
    uint32 *ptr = table_aux->insertions.array;
    for (uint32 i=0 ; i < count ; i++) {
      uint32 arg1 = *(ptr++);
      uint32 arg2 = *(ptr++);
      uint32 arg3 = *(ptr++);

      int32 code = master_bin_table_insert_ex(&table->master, arg1, arg2, mem_pool);
      uint32 surr12;
      if (code >= 0) {
        surr12 = code;
        incr_rc_1(store_1, arg1);
        incr_rc_2(store_2, arg2);
      }
      else
        surr12 = -code - 1;

      if (bin_table_insert(&table->slave, surr12, arg3, mem_pool))
        incr_rc_3(store_3, arg3);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void tern_table_aux_record_col_3_key_violation(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

}

static void tern_table_aux_record_cols_12_key_violation(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

}

static void tern_table_aux_record_cols_13_key_violation(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

}

static void tern_table_aux_record_cols_23_key_violation(TERN_TABLE_AUX *table_aux, uint32 arg1, uint32 arg2, uint32 arg3, bool between_new) {

}

////////////////////////////////////////////////////////////////////////////////

bool tern_table_aux_check_key_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool tern_table_aux_check_key_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool tern_table_aux_check_key_13(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

bool tern_table_aux_check_key_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux) {
  return true; //## IMPLEMENT IMPLEMENT IMPLEMENT
}
