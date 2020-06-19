#include "lib.h"


void *alloc_mem_block(uint32 size);


static bool static_allocation = false;


void *new_obj(uint32 byte_size) {
  if (static_allocation)
    return malloc(byte_size);
  else
    return alloc_mem_block(byte_size);
}


bool is_already_in_place(void *ptr) {
  return false;
}


void switch_to_static_allocator() {
  static_allocation = true;
}

void switch_to_twin_stacks_allocator() {
  static_allocation = false;
}
