#include "lib.h"
#include "os-interface.h"


const uint64 STACK_ALLOC_BLOCK_SIZE = 1048576;

static uint64 stack_alloc_round_up_by_block_size(uint64 size) {
  return ((size + STACK_ALLOC_BLOCK_SIZE - 1) / STACK_ALLOC_BLOCK_SIZE) * STACK_ALLOC_BLOCK_SIZE;
}

static uint64 stack_alloc_round_up_by_dealloc_size(uint64 size) {
  const uint64 DEALLOC_SIZE = 2 * STACK_ALLOC_BLOCK_SIZE;
  return ((size + DEALLOC_SIZE - 1) / DEALLOC_SIZE) * DEALLOC_SIZE;
}

////////////////////////////////////////////////////////////////////////////////

void stack_alloc_init(STACK_ALLOC *alloc, uint64 size) {
  assert(size % (1024 * 1024) == 0);

  void *ptr = os_interface_reserve(size);

  alloc->ptr = ptr;
  alloc->size = size;
  alloc->committed = 0;
  alloc->allocated = 0;
}

void stack_alloc_cleanup(STACK_ALLOC *alloc) {
  os_interface_release(alloc->ptr, alloc->size);
}

////////////////////////////////////////////////////////////////////////////////

void *stack_alloc_allocate(STACK_ALLOC *alloc, uint64 size) {
  assert(size % 8 == 0);

  uint8 *ptr = (uint8 *) alloc->ptr;
  uint64 committed = alloc->committed;
  uint64 allocated = alloc->allocated;

  if (allocated + size > alloc->size)
    impl_fail(NULL);

  uint64 needed = allocated + size;
  alloc->allocated = needed;

  if (needed > committed) {
    uint64 new_commit_size = stack_alloc_round_up_by_block_size(needed - committed);
    os_interface_alloc(ptr, committed, new_commit_size);
    alloc->committed = committed + new_commit_size;
  }

  return ptr + allocated;
}

void stack_alloc_rewind(STACK_ALLOC *alloc, uint64 bookmark) {
  assert(bookmark <= alloc->allocated);
  alloc->allocated = bookmark;
}

void stack_alloc_clear(STACK_ALLOC *alloc) {
  uint64 committed = alloc->committed;
  uint64 allocated = alloc->allocated;
  uint64 reduced_committed = stack_alloc_round_up_by_dealloc_size(allocated);
  if (reduced_committed < committed) {
    void *ptr = ((uint8 *) alloc->ptr) + reduced_committed;
    os_interface_dealloc(alloc->ptr, reduced_committed, committed - reduced_committed);
    alloc->committed = reduced_committed;
  }
}

////////////////////////////////////////////////////////////////////////////////

uint64 stack_alloc_bookmark(STACK_ALLOC *alloc) {
  return alloc->allocated;
}

bool stack_alloc_is_unallocated_memory(STACK_ALLOC *alloc, void *ptr) {
  uint8 *start_ptr = (uint8 *) alloc->ptr;
  void *free_start_ptr = start_ptr + alloc->allocated;
  void *end_ptr = start_ptr + alloc->size;
  return free_start_ptr <= ptr && ptr < end_ptr;
}
