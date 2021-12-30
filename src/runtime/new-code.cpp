#include "lib.h"



int32 cast_int32(int64 val64) {
  int32 val32 = (int32) val64;
  if (val32 != val64)
    soft_fail("Invalid 64 to 32 bit integer conversion");
  return val32;
}

OBJ set_insert(OBJ set, OBJ elt) {
  impl_fail("Not implemented yet");
}

OBJ set_key_value(OBJ map, OBJ key, OBJ value) {
  impl_fail("Not implemented yet");
}

OBJ drop_key(OBJ map, OBJ key) {
  impl_fail("Not implemented yet");
}
