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

uint32 istream_read(void *ptr, uint8 *buffer, uint32 capacity) {
  std::istream &is = *static_cast<std::istream *>(ptr);
  is.read((char *) buffer, capacity);
  if (is.bad())
    throw std::exception(); //## TODO: ADD ERROR INFORMATION
  uint32 read = is.gcount();
  assert(read == capacity | is.eof());
  return read;
}

bool ostream_write(void *ptr, const uint8 *data, uint32 size) {
  std::ostream &os = *static_cast<std::ostream *>(ptr);
  os.write(reinterpret_cast<const char *>(data), size);
  return os.good();
}
