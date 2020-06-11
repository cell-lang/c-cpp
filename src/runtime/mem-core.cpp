#include "lib.h"


void *alloc_mem_block(uint32 size);


void *new_obj(uint32 byte_size) {
  return alloc_mem_block(byte_size);
}

void *new_obj(uint32 byte_size_requested, uint32 &byte_size_returned) {
  byte_size_returned = byte_size_requested;
  return new_obj(byte_size_requested);
}

