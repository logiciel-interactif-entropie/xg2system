#include "mem.h"

#include <stdlib.h>

void* __heap_manipulate(void* old, size_t t, const char* name) {
  if (old) {
    if (t) {
      return realloc(old, t);
    } else {
      free(old);
    }
  } else {
    if (!t) return NULL;
    return malloc(t);
  }
  return NULL;
}
