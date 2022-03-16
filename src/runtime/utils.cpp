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

//## NOT THE BEST PLACES FOR THESE FUNCTIONS PROBABLY

void null_incr_rc(void *, uint32) {

}

void null_decr_rc(void *, void *, uint32) {

}
