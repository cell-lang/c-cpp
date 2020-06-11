#include "lib.h"

const uint32 MIN_ALLOC_SIZE = 256 * 1024 * 1024;

static uint8 *stack;
static uint64 left = 0;

////////////////////////////////////////////////////////////////////////////////

void *alloc_mem_block(uint32 size) {
  size = 8 * ((size + 7) / 8);

  if (size > left) {
    uint64 alloc_size = MIN_ALLOC_SIZE;
    while (alloc_size < size)
      alloc_size *= 2;
    stack = (uint8 *) malloc(alloc_size);
    left = alloc_size;
  }

  void *ptr = stack;
  stack += size;
  left -= size;
  return ptr;
}
