#pragma once

#include <stddef.h>

void* __heap_manipulate(void* old, size_t t, const char* name);

#define HEAP_ALLOC(S) __heap_manipulate(NULL, S, "heap-alloc")
#define HEAP_ALLOC_TYPE(T) (T*)__heap_manipulate(NULL, sizeof(T), #T)
#define HEAP_ALLOC_ARRAY(T, C) \
  (T*)__heap_manipulate(NULL, sizeof(T) * C, #T " array")
#define HEAP_FREE(P) __heap_manipulate(P, 0, NULL)
