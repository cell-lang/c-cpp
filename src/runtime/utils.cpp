#include "lib.h"


bool _assert_(int expr, const char *expr_text, const char *file, int line) {
  if (!expr) {
    int idx = 0;
    while (file[idx] != '\0')
      idx++;

    while (idx >= 0 && file[idx] != '\\')
      idx--;

    fprintf(stderr, "Assertion \"%s\" failed, file: %s, line: %d\n", expr_text, file + idx + 1, line);
    fflush(stderr);

    (*((char *)0)) = 0;
  }

  return true;
}

int32 cast_int32(int64 val64) {
  int32 val32 = (int32) val64;
  if (val32 != val64)
    soft_fail("Invalid 64 to 32 bit integer conversion");
  return val32;
}

//## NOT THE BEST PLACES FOR THESE FUNCTIONS PROBABLY

void null_incr_rc(void *, uint32) {

}

void null_decr_rc(void *, void *, uint32) {

}
