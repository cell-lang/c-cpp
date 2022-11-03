#include <iostream>
#include "lib.h"


void handler_wrapper_begin() {
  assert(total_stack_mem_alloc() == 0);
}

void handler_wrapper_finish() {
  clear_all_mem();
  assert(total_stack_mem_alloc() == 0);
}

void handler_wrapper_abort() {
  clear_all_mem();
  assert(total_stack_mem_alloc() == 0);
}

////////////////////////////////////////////////////////////////////////////////

void method_wrapper_begin() {
  assert(total_stack_mem_alloc() == 0);
  entering_transaction();
}

void method_wrapper_finish() {
  exiting_transaction();
  clear_all_mem();
  assert(total_stack_mem_alloc() == 0);
}

void method_wrapper_abort() {
  exiting_transaction();
  clear_all_mem();
  assert(total_stack_mem_alloc() == 0);
}

////////////////////////////////////////////////////////////////////////////////

bool ostream_write(void *ptr, const uint8 *data, uint32 size) {
  std::ostream &os = *static_cast<std::ostream *>(ptr);
  os.write(reinterpret_cast<const char *>(data), size);
  return os.good();
}
