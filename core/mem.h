#pragma once

#include <stddef.h>

#ifndef NDEBUG
void* __heap_manipulate(void* old, size_t t, const char* name, const char* file,
                        int line);
#else
void* __heap_manipulate(void* old, size_t t, const char* name);
#endif

void heap_init();
void heap_end();
void heap_dump();

#ifndef NDEBUG
#define HEAP_ALLOC(S) \
  __heap_manipulate(NULL, S, "heap-alloc", __FILE__, __LINE__)
#define HEAP_ALLOC_NAMED(S, N) __heap_manipulate(NULL, S, N, __FILE__, __LINE__)
#define HEAP_ALLOC_TYPE(T) \
  (T*)__heap_manipulate(NULL, sizeof(T), #T, __FILE__, __LINE__)
#define HEAP_ALLOC_ARRAY(T, C) \
  (T*)__heap_manipulate(NULL, sizeof(T) * C, #T " array", __FILE__, __LINE__)
#define HEAP_FREE(P) __heap_manipulate(P, 0, NULL, __FILE__, __LINE__)
#else
#define HEAP_ALLOC(S) __heap_manipulate(NULL, S, "heap-alloc")
#define HEAP_ALLOC_NAMED(S, N) __heap_manipulate(NULL, S, N)
#define HEAP_ALLOC_TYPE(T) (T*)__heap_manipulate(NULL, sizeof(T), #T)
#define HEAP_ALLOC_ARRAY(T, C) \
  (T*)__heap_manipulate(NULL, sizeof(T) * C, #T " array")
#define HEAP_FREE(P) __heap_manipulate(P, 0, NULL)
#endif
