#include "lib.h"


void *alloc_mem_block(uint32 size);


void *new_obj(uint32 byte_size) {
  return alloc_mem_block(byte_size);
}
