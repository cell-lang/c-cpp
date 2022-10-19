#include "lib.h"


void init_twin_stacks();


void init_runtime() {
  bool initialized = false;
  if (!initialized) {
    init_twin_stacks();
    initialized = true;
  }
}
