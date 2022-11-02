#include "lib.h"
#include "os-interface.h"


static STACK_ALLOC stack_A, stack_B;
static bool stack_A_active = true;

static uint64 checkpoints[1024];
static uint32 chkpt_index = 0;

////////////////////////////////////////////////////////////////////////////////

static STACK_ALLOC *active_stack() {
  return stack_A_active ? &stack_A : &stack_B;
}

static STACK_ALLOC *inactive_stack() {
  return stack_A_active ? &stack_B : &stack_A;
}

////////////////////////////////////////////////////////////////////////////////

void init_twin_stacks() {
  uint64 mem_size = phys_mem_byte_size();
  uint64 stack_size = pow_2_ceiling_u64(mem_size, 1024 * 1024);
  stack_alloc_init(&stack_A, stack_size);
  stack_alloc_init(&stack_B, stack_size);
}

////////////////////////////////////////////////////////////////////////////////

void switch_mem_stacks() {
  stack_A_active = !stack_A_active;
  STACK_ALLOC *stack = active_stack();
  uint64 bookmark = stack_alloc_bookmark(stack);
  checkpoints[chkpt_index++] = bookmark;
}

void unswitch_mem_stacks() {
  STACK_ALLOC *stack = active_stack();
  uint64 bookmark = checkpoints[--chkpt_index];
  stack_alloc_rewind(stack, bookmark);
  stack_A_active = !stack_A_active;
}

void clear_unused_mem() {
  stack_alloc_clear(&stack_A);
  stack_alloc_clear(&stack_B);
}

void clear_all_mem() {
  stack_A_active = true;
  stack_alloc_reset(&stack_A);
  stack_alloc_reset(&stack_B);
}

uint64 total_stack_mem_alloc() {
  return stack_A.allocated + stack_B.allocated;
}

////////////////////////////////////////////////////////////////////////////////

bool is_in_released_mem(void *ptr) {
  STACK_ALLOC *stack = inactive_stack();
  return stack_alloc_is_unallocated_memory(stack, ptr);
}

////////////////////////////////////////////////////////////////////////////////

void *alloc_mem_block(uint32 size) {
  STACK_ALLOC *stack = active_stack();
  uint32 rounded_up_size = round_up_8(size);
  return stack_alloc_allocate(stack, rounded_up_size);
}
