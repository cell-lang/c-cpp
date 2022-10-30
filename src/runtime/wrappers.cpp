#include "lib.h"


void handler_wrapper_begin() {
  // entering_transaction();
}

void handler_wrapper_finish() {
  // exiting_transaction();
}

void handler_wrapper_abort() {
  // exiting_transaction();
}

////////////////////////////////////////////////////////////////////////////////

void method_wrapper_begin() {
  entering_transaction();
}

void method_wrapper_finish() {
  exiting_transaction();
}

void method_wrapper_abort() {
  exiting_transaction();
}

