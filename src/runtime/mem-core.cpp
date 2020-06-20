#include "lib.h"


void *alloc_mem_block(uint32 size);
bool is_in_released_mem(void *ptr);


static bool static_allocation = false;

static uint64 total_long_lived = 0;
static uint64 total_temp = 0;


void *new_obj(uint32 byte_size) {
  if (static_allocation)
    return malloc(byte_size);
  else {
    total_long_lived += 8 * ((byte_size + 7) / 8);
    return alloc_mem_block(byte_size);
  }
}

void *new_raw_mem(uint32 byte_size) {
  total_temp += 8 * ((byte_size + 7) / 8);
  return alloc_mem_block(byte_size);
}

bool needs_copying(void *ptr) {
  if (static_allocation)
    return true;
  else
    return is_in_released_mem(ptr);
}


void switch_to_static_allocator() {
  static_allocation = true;
}

void switch_to_twin_stacks_allocator() {
  static_allocation = false;
}

void print_mem_alloc_stats() {
  printf("Total allocation:\n");
  printf("  dynamic:   %llu\n", total_long_lived);
  printf("  temporary: %llu\n", total_temp);
}