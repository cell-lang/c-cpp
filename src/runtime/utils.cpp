#include "lib.h"


bool _assert_(int exp, const char *exp_text, const char *file, int line) {
  if (!exp) {
    int idx = 0;
    while (file[idx] != '\0')
      idx++;

    while (idx >= 0 && file[idx] != '\\')
      idx--;

    fprintf(stderr, "Assertion \"%s\" failed, file: %s, line: %d\n", exp_text, file + idx + 1, line);
    fflush(stderr);

    (*((char *)0)) = 0;
  }

  return true;
}
