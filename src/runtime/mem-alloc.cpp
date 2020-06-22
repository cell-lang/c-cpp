#include "lib.h"


typedef struct {
  uint8 *stack;
  uint8 *top;
  uint32 size;
  uint32 left;
} MEM_REGION;

typedef struct {
  MEM_REGION regions[64];
  int index = 0;
} MEM_STACK;

typedef struct {
  int    index;
  uint8 *top;
  uint32 left;
  MEM_STACK *stack;
} MEM_CHKPT;

////////////////////////////////////////////////////////////////////////////////

static MEM_STACK stackA, stackB;
static bool stackA_active = true;

static MEM_CHKPT checkpoints[1024];
static int chkpt_index = 0;

////////////////////////////////////////////////////////////////////////////////

int non_null_regions(MEM_STACK *stack) {
  for (int i=63 ; i >= 0 ; i--) {
    MEM_REGION *region = stack->regions + i;
    if (region->stack != NULL | region->top != NULL | region->size != 0 | region->left != 0)
      return i + 1;
  }
  return 0;
}

void dump_state() {
  printf("  Active stack: %s\n", stackA_active ? "A" : "B");
  printf("  Stack A @ %d\n", stackA.index);
  int count = non_null_regions(&stackA);
  for (int i=0 ; i < count ; i++) {
    MEM_REGION *r = stackA.regions + i;
    printf("    %16p  %16p  %8x  %12d  %12d\n", (void *) r->stack, (void *) r-> top, r->size, r->left, r->size - r->left);
  }
  count = non_null_regions(&stackB);
  printf("  Stack B @ %d\n", stackB.index);
  for (int i=0 ; i < count ; i++) {
    MEM_REGION *r = stackB.regions + i;
    printf("    %16p  %16p  %8x  %12d  %12d\n", (void *) r->stack, (void *) r-> top, r->size, r->left, r->size - r->left);
  }
  printf("  Checkpoints:\n");
  for (int i=0 ; i < chkpt_index ; i++) {
    MEM_CHKPT *cp = checkpoints + i;
    const char *stack_id = "?";
    if (cp->stack == &stackA)
      stack_id = "A";
    if (cp->stack == &stackB)
      stack_id = "B";
    printf("    %2d  %16p  %8x  %s\n", cp->index, cp->top, cp->left, stack_id);
  }

  double used = 0, wasted = 0, unused = 0;

  count = non_null_regions(&stackA);
  for (int i=0 ; i < count ; i++) {
    MEM_REGION *r = stackA.regions + i;
    used += r->size - r->left;
    if (i < count - 1)
      wasted += r->left;
    else
      unused += r->left;
  }

  count = non_null_regions(&stackB);
  for (int i=0 ; i < count ; i++) {
    MEM_REGION *r = stackB.regions + i;
    used += r->size - r->left;
    if (i < count - 1)
      wasted += r->left;
    else
      unused += r->left;
  }

  used /= 1024 * 1024;
  unused /= 1024 * 1024;
  wasted /= 1024 * 1024;

  printf("\n  used = %.2f, unused = %.2f, wasted = %.2f\n\n", used, unused, wasted);
}

////////////////////////////////////////////////////////////////////////////////

const uint32 MIN_ALLOC_SIZE = 256 * 1024 * 1024;


static MEM_STACK *active_stack() {
  return stackA_active ? &stackA : &stackB;
}

static MEM_STACK *inactive_stack() {
  return stackA_active ? &stackB : &stackA;
}

////////////////////////////////////////////////////////////////////////////////

void switch_mem_stacks() {
  // printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");
  // print_memory_in_use();

  stackA_active = !stackA_active;

  MEM_STACK *stack = active_stack();
  MEM_CHKPT *chkpt = checkpoints + chkpt_index++;

  int index = stack->index;

  MEM_REGION *region = stack->regions + index;

  chkpt->index = index;
  chkpt->top = region->top;
  chkpt->left = region->left;
  chkpt->stack = stack;


  // printf("\n");
  // print_memory_in_use();
  // printf("\n");
}

void unswitch_mem_stacks() {
  // printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
  // print_memory_in_use();

  MEM_STACK *stack = active_stack();
  MEM_CHKPT *chkpt = checkpoints + --chkpt_index;

  assert(chkpt->stack == stack);

  int index = chkpt->index;
  MEM_REGION *region = stack->regions + index;

  stack->index = index;

  if (chkpt->top != NULL) {
    assert(region->stack + region->size == chkpt->top + chkpt->left);

    region->top = chkpt->top;
    region->left = chkpt->left;

    assert(region->stack + region->size == region->top + region->left);
  }
  else {
    assert(chkpt->left == 0);

    region->top = region->stack;
    region->left = region->size;

    assert(region->stack + region->size == region->top + region->left);
  }

  assert(region->stack + region->size == region->top + region->left);

  stackA_active = !stackA_active;

  // printf("\n");
  // print_memory_in_use();
  // printf("\n");
}

void clear_unused_mem(bool clear_released) {
  // printf("\n-------------------------\n\n");
  // print_memory_in_use();

  MEM_STACK *stack = inactive_stack();
  int index = stack->index;
  MEM_REGION *region = stack->regions + index;

  if (region->stack != NULL) {
    assert(region->stack + region->size == region->top + region->left);

    if (clear_released)
      memset(region->top, 0xFF, region->left);

    for (int i = index + 1 ; ; i++) {
      MEM_REGION *region = stack->regions + i;
      void *stack = region->stack;

      if (stack == NULL)
        break;

      if (clear_released)
        memset(stack, 0xFF, region->size);

      free(stack);
      region->stack = NULL;
      region->top = NULL;
      region->size = 0;
      region->left = 0;
    }
  }

  // printf("\n");
  // print_memory_in_use();
  // printf("\n");
}


bool reset_released_mem = false;

void clear_unused_mem() {
  clear_unused_mem(reset_released_mem);
}

////////////////////////////////////////////////////////////////////////////////

bool is_in_released_mem(void *ptr) {
  MEM_STACK *stack = inactive_stack();
  int index = stack->index;
  MEM_REGION *region = stack->regions + index;

  if (region->stack != NULL) {
    assert(region->stack + region->size == region->top + region->left);

    if (ptr >= region->top & ptr < region->stack + region->size)
      return true;

    for (int i = index + 1 ; ; i++) {
      MEM_REGION *region = stack->regions + i;
      uint8 *start = region->stack;

      if (start == NULL)
        return false;

      if (ptr >= region->top & ptr < start + region->size);
        return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void *alloc_mem_block(uint32 size) {
  size = 8 * ((size + 7) / 8);

  MEM_STACK *stack = active_stack();
  MEM_REGION *region = &(stack->regions[stack->index]);

  if (size > region->left) {
    uint64 alloc_size = MIN_ALLOC_SIZE;
    while (alloc_size < size)
      alloc_size *= 2;

    if (region->stack == NULL) {
      region->stack = (uint8 *) malloc(alloc_size);
      region->top = region->stack;
      region->size = alloc_size;
      region->left = alloc_size;
    }
    else {
      stack->index++;
      return alloc_mem_block(size);
    }
  }

  void *ptr = region->top;
  region->top += size;
  region->left -= size;
  return ptr;
}
