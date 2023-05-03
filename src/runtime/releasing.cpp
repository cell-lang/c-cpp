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

////////////////////////////////////////////////////////////////////////////////

void single_key_bin_table_release_2_before_delete_1(SINGLE_KEY_BIN_TABLE *table, uint32 arg1, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = single_key_bin_table_mem_pool(table);
  uint32 count = single_key_bin_table_count_1(table, arg1);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = single_key_bin_table_range_restrict_1(table, arg1, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg2 = array.array[i];
      if (single_key_bin_table_count_2(table, arg2) <= 1)
        remove(store, arg2, mem_pool);
    }
  }
}

void single_key_bin_table_release_1_before_delete_2(SINGLE_KEY_BIN_TABLE *table, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = single_key_bin_table_mem_pool(table);
  uint32 count = single_key_bin_table_count_2(table, arg2);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = single_key_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg1 = array.array[i];
      if (single_key_bin_table_count_1(table, arg1) <= 1)
        remove(store, arg1, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void double_key_bin_table_release_2_before_delete_1(DOUBLE_KEY_BIN_TABLE *table, uint32 arg1, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = double_key_bin_table_mem_pool(table);
  uint32 count = double_key_bin_table_count_1(table, arg1);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = double_key_bin_table_range_restrict_1(table, arg1, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg2 = array.array[i];
      if (double_key_bin_table_count_2(table, arg2) <= 1)
        remove(store, arg2, mem_pool);
    }
  }
}

void double_key_bin_table_release_1_before_delete_2(DOUBLE_KEY_BIN_TABLE *table, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = double_key_bin_table_mem_pool(table);
  uint32 count = double_key_bin_table_count_2(table, arg2);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = double_key_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg1 = array.array[i];
      if (double_key_bin_table_count_1(table, arg1) <= 1)
        remove(store, arg1, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void master_bin_table_release_2_before_delete_1(MASTER_BIN_TABLE *table, uint32 arg1, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->table);
  uint32 count = master_bin_table_count_1(table, arg1);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = master_bin_table_range_restrict_1(table, arg1, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg2 = array.array[i];
      if (master_bin_table_count_2(table, arg2) <= 1)
        remove(store, arg2, mem_pool);
    }
  }
}

void master_bin_table_release_1_before_delete_2(MASTER_BIN_TABLE *table, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->table);
  uint32 count = master_bin_table_count_2(table, arg2);
  uint32 read = 0;
  while (read < count) {
    uint32 buffer[64];
    UINT32_ARRAY array = master_bin_table_range_restrict_2(table, arg2, read, buffer, 64);
    read += array.size;
    for (uint32 i=0 ; i < array.size ; i++) {
      uint32 arg1 = array.array[i];
      if (master_bin_table_count_1(table, arg1) <= 1)
        remove(store, arg1, mem_pool);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void slave_tern_table_release_3_before_delete_12(BIN_TABLE *table, uint32 surr12, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  bin_table_release_2_before_delete_1(table, surr12, remove, store);
}

////////////////////////////////////////////////////////////////////////////////

void tern_table_release_2_before_delete_1(TERN_TABLE *table, uint32 arg1, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 count = master_bin_table_count_1(&table->master, arg1);
  if (count > 0) {
    uint32 read = 0;
    while (read < count) {
      uint32 buffer[64];
      UINT32_ARRAY array = master_bin_table_range_restrict_1(&table->master, arg1, read, buffer, 64);
      read += array.size;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 arg2 = array.array[i];
        assert(master_bin_table_count_2(&table->master, arg2) > 0);
        if (master_bin_table_count_2(&table->master, arg2) == 1)
          remove(store, arg2, mem_pool);
      }
    }
  }
}

void tern_table_release_3_before_delete_1(TERN_TABLE *table, uint32 arg1, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 pseudo_count_1 = master_bin_table_count_1(&table->master, arg1);
  if (pseudo_count_1 > 0) {
    TRNS_MAP_SURR_U32 counters;
    trns_map_surr_u32_init(&counters);

    uint32 read1 = 0;
    while (read1 < pseudo_count_1) {
      uint32 buffer[128];
      UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(&table->master, arg1, read1, buffer, 64); //## BAD BAD BAD: ONLY NEED THE SURROGATES HERE
      read1 += array.size;
      uint32 *surrs = array.array + array.offset;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 surr = surrs[i];
        uint32 count_surr = bin_table_count_1(&table->slave, surr);
        if (count_surr > 0) {
          uint32 read_surr = 0;
          while (read_surr < count_surr) {
            uint32 buffer3[64];
            UINT32_ARRAY array = bin_table_range_restrict_1(&table->slave, surr, read_surr, buffer3, 64);
            read_surr += array.size;
            for (uint32 j=0 ; j < array.size ; j++) {
              uint32 arg3 = array.array[j];
              uint32 count3 = bin_table_count_2(&table->slave, arg3);
              assert(count3 > 0);
              if (count3 > 1)
                count3 = trns_map_surr_u32_lookup(&counters, arg3, count3);
              if (count3 == 1)
                remove(store, arg3, mem_pool);
              else
                trns_map_surr_u32_set(&counters, arg3, count3 - 1);
            }
          }
        }
      }
    }
  }
}

void tern_table_release_1_before_delete_2(TERN_TABLE *table, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 count = master_bin_table_count_2(&table->master, arg2);
  if (count > 0) {
    uint32 read = 0;
    while (read < count) {
      uint32 buffer[64];
      UINT32_ARRAY array = master_bin_table_range_restrict_2(&table->master, arg2, read, buffer, 64);
      read += array.size;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 arg1 = array.array[i];
        assert(master_bin_table_count_1(&table->master, arg1) > 0);
        if (master_bin_table_count_1(&table->master, arg1) == 1)
          remove(store, arg1, mem_pool);
      }
    }
  }
}

void tern_table_release_3_before_delete_2(TERN_TABLE *table, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 pseudo_count_2 = master_bin_table_count_2(&table->master, arg2);
  if (pseudo_count_2 > 0) {
    TRNS_MAP_SURR_U32 counters;
    trns_map_surr_u32_init(&counters);

    uint32 read2 = 0;
    while (read2 < pseudo_count_2) {
      uint32 buffer[128];
      UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(&table->master, arg2, read2, buffer, 64); //## BAD BAD BAD: ONLY NEED THE SURROGATES HERE
      read2 += array.size;
      uint32 *surrs = array.array + array.offset;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 surr = surrs[i];
        uint32 count_surr = bin_table_count_1(&table->slave, surr);
        if (count_surr > 0) {
          uint32 read_surr = 0;
          while (read_surr < count_surr) {
            uint32 buffer3[64];
            UINT32_ARRAY array = bin_table_range_restrict_1(&table->slave, surr, read_surr, buffer3, 64);
            read_surr += array.size;
            for (uint32 j=0 ; j < array.size ; j++) {
              uint32 arg3 = array.array[j];
              uint32 count3 = bin_table_count_2(&table->slave, arg3);
              assert(count3 > 0);
              if (count3 > 1)
                count3 = trns_map_surr_u32_lookup(&counters, arg3, count3);
              if (count3 == 1)
                remove(store, arg3, mem_pool);
              else
                trns_map_surr_u32_set(&counters, arg3, count3 - 1);
            }
          }
        }
      }
    }
  }
}

void tern_table_release_1_before_delete_3(TERN_TABLE *table, uint32 arg3, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count3 > 0) {
    TRNS_MAP_SURR_U32 counters;
    trns_map_surr_u32_init(&counters);

    uint32 read3 = 0;
    while (read3 < count3) {
      uint32 buffer[64];
      UINT32_ARRAY array = bin_table_range_restrict_2(&table->slave, arg3, count3, buffer, 64);
      read3 += array.size;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 surr = array.array[i];
        uint32 arg1 = master_bin_table_get_arg_1(&table->master, surr);
        uint32 count1 = master_bin_table_count_1(&table->master, arg1);
        assert(count1 > 0);
        if (count1 > 1)
          count1 = trns_map_surr_u32_lookup(&counters, arg1, count1);
        if (count1 == 1)
          remove(store, arg1, mem_pool);
        else
          trns_map_surr_u32_set(&counters, arg1, count1 - 1);
      }
    }
  }
}

void tern_table_release_2_before_delete_3(TERN_TABLE *table, uint32 arg3, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (count3 > 0) {
    TRNS_MAP_SURR_U32 counters;
    trns_map_surr_u32_init(&counters);

    uint32 read3 = 0;
    while (read3 < count3) {
      uint32 buffer[64];
      UINT32_ARRAY array = bin_table_range_restrict_2(&table->slave, arg3, count3, buffer, 64);
      read3 += array.size;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 surr = array.array[i];
        uint32 arg2 = master_bin_table_get_arg_2(&table->master, surr);
        uint32 count2 = master_bin_table_count_2(&table->master, arg2);
        assert(count2 > 0);
        if (count2 > 1)
          count2 = trns_map_surr_u32_lookup(&counters, arg2, count2);
        if (count2 == 1)
          remove(store, arg2, mem_pool);
        else
          trns_map_surr_u32_set(&counters, arg2, count2 - 1);
      }
    }
  }
}

void tern_table_release_3_before_delete_12(TERN_TABLE *table, uint32 arg1, uint32 arg2, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 surr = master_bin_table_lookup_surr(&table->master, arg1, arg2);
  if (surr != 0xFFFFFFFF) {
    uint32 count = bin_table_count_1(&table->slave, surr);
    assert(count > 0);
    uint32 read = 0;
    while (read < count) {
      uint32 buffer[64];
      UINT32_ARRAY array = bin_table_range_restrict_1(&table->slave, surr, read, buffer, 64);
      read += array.size;
      for (uint32 i=0 ; i < array.size ; i++) {
        uint32 arg3 = array.array[i];
        uint32 count3 = bin_table_count_2(&table->slave, arg3);
        assert(count3 > 0);
        if (count3 == 1)
          remove(store, arg3, mem_pool);
      }
    }
  }
}

void tern_table_release_2_before_delete_13(TERN_TABLE *table, uint32 arg1, uint32 arg3, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 pseudo_count_1 = master_bin_table_count_1(&table->master, arg1);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (pseudo_count_1 > 0 && count3 > 0) {
    // if (pseudo_count_1 < count3) {
      TRNS_MAP_SURR_U32 counters;
      trns_map_surr_u32_init(&counters);

      uint32 read1 = 0;
      while (read1 < pseudo_count_1) {
        uint32 buffer[128];
        UINT32_ARRAY array = master_bin_table_range_restrict_1_with_surrs(&table->master, arg1, read1, buffer, 64);
        read1 += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 i=0 ; i < array.size ; i++) {
          uint32 surr = surrs[i];
          if (bin_table_contains(&table->slave, surr, arg3)) {
            uint32 arg2 = array.array[i];

            // This is the block that is executed for each (arg1, arg2?, arg3) triple
            uint32 count2 = trns_map_surr_u32_lookup(&counters, arg2, 0);
            if (count2 == 0)
              count2 = tern_table_count_2(table, arg2);
            assert(count2 > 0);
            if (count2 == 1)
              remove(store, arg2, mem_pool);
            else
              trns_map_surr_u32_set(&counters, arg2, count2 - 1);
          }
        }
      }
    // }
    // else {
    //   //## IMPLEMENT IMPLEMENT IMPLEMENT
    // }
  }
}

void tern_table_release_1_before_delete_23(TERN_TABLE *table, uint32 arg2, uint32 arg3, void (*remove)(void *, uint32, STATE_MEM_POOL *), void *store) {
  STATE_MEM_POOL *mem_pool = bin_table_mem_pool(&table->master.table);
  uint32 pseudo_count_2 = master_bin_table_count_2(&table->master, arg2);
  uint32 count3 = bin_table_count_2(&table->slave, arg3);
  if (pseudo_count_2 > 0 && count3 > 0) {
    // if (pseudo_count_2 < count3) {
      TRNS_MAP_SURR_U32 counters;
      trns_map_surr_u32_init(&counters);

      uint32 read2 = 0;
      while (read2 < pseudo_count_2) {
        uint32 buffer[128];
        UINT32_ARRAY array = master_bin_table_range_restrict_2_with_surrs(&table->master, arg2, read2, buffer, 64);
        read2 += array.size;
        uint32 *surrs = array.array + array.offset;
        for (uint32 i=0 ; i < array.size ; i++) {
          uint32 surr = surrs[i];
          if (bin_table_contains(&table->slave, surr, arg3)) {
            uint32 arg1 = array.array[i];

            // This is the block that is executed for each (arg1?, arg2, arg3) triple
            uint32 count1 = trns_map_surr_u32_lookup(&counters, arg1, 0);
            if (count1 == 0)
              count1 = tern_table_count_1(table, arg1);
            assert(count1 > 0);
            if (count1 == 1)
              remove(store, arg1, mem_pool);
            else
              trns_map_surr_u32_set(&counters, arg1, count1 - 1);
          }
        }
      }
    // }
    // else {
    //   //## IMPLEMENT IMPLEMENT IMPLEMENT
    // }
  }
}
