#include "lib.h"



void bin_table_release_2_before_delete_1(BIN_TABLE *table, uint32 arg1, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
  uint32 count = bin_table_count_1(table, arg1);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = bin_table_range_restrict_1(table, arg1, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg2 = array.array[i];
      if (bin_table_count_2(table, arg2) <= 1)
        remove(store, arg2, mem_pool);
    }
  }
}

void bin_table_release_1_before_delete_2(BIN_TABLE *table, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(table);
  uint32 count = bin_table_count_2(table, arg2);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = bin_table_range_restrict_2(table, arg2, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg1 = array.array[i];
      if (bin_table_count_1(table, arg1) <= 1)
        remove(store, arg1, mem_pool);
    }
  }
}
