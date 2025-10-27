#include "mem.h"

#include <glib.h>
#include <signal.h>
#include <stdlib.h>

#include "core/log.h"

#ifdef __linux
#include <execinfo.h>
#include <unistd.h>
#endif

#ifndef XG2S_DISABLE_HEAP_TRACKING
struct __attribute__((packed)) allocation {
  void* ptr;
  const char* name;
#ifndef NDEBUG
  const char* file;
  int line;
#endif
  size_t current_size;
};

struct heap_manager {
  GHashTable* allocations;
  size_t memory_usage;
  size_t max_memory_used;
}* heap_manager;
#endif

static void __segfaulthandler(int sig) {
  LOG(ll_error, "Segmentation fault");
#ifdef __linux
  void* array[20];
  size_t size = backtrace(array, 20);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
#endif
  heap_end();
  exit(EXIT_FAILURE);
}

void heap_init() {
#ifndef XG2S_DISABLE_HEAP_TRACKING
  heap_manager = (struct heap_manager*)malloc(sizeof(struct heap_manager));
  heap_manager->allocations = g_hash_table_new(g_direct_hash, g_direct_equal);
#endif

  signal(SIGSEGV, __segfaulthandler);
}

#ifndef XG2S_DISABLE_HEAP_TRACKING
static int leaked_bytes_freed = 0;

static void __heap_end_foreach(gpointer key, gpointer value,
                               gpointer user_data) {
  struct allocation* allocation = (struct allocation*)value;
  leaked_bytes_freed += allocation->current_size;

  free(allocation->ptr);
  free(allocation);
}
#endif

void heap_end() {
#ifndef XG2S_DISABLE_HEAP_TRACKING
  LOG(ll_debug, "memory usage at exit: %i bytes", heap_manager->memory_usage);

#ifndef NDEBUG
  heap_dump();
#endif

  g_hash_table_foreach(heap_manager->allocations, __heap_end_foreach, NULL);

  if (leaked_bytes_freed)
    LOG(ll_debug, "freed %i leaked bytes", leaked_bytes_freed);
  LOG(ll_debug, "memory usage max: %i bytes", heap_manager->max_memory_used);

  free(heap_manager);
#endif
}

#ifndef XG2S_DISABLE_HEAP_TRACKING
static void __heap_dump_foreach(gpointer key, gpointer value,
                                gpointer user_data) {
  struct allocation* allocation = (struct allocation*)value;
#ifndef NDEBUG
  LOG(ll_debug, "`%s`\n\t%016p - %016x\n\t%s:%i\n", allocation->name,
      allocation->ptr, allocation->current_size, allocation->file,
      allocation->line);
#else
  LOG(ll_debug, "`%s`\n\t%016p - %016x\n", allocation->name, allocation->ptr,
      allocation->current_size);
#endif
}
#endif

void heap_dump() {
#ifndef XG2S_DISABLE_HEAP_TRACKING
  LOG(ll_debug, "%i allocations leaked",
      g_hash_table_size(heap_manager->allocations));
  g_hash_table_foreach(heap_manager->allocations, __heap_dump_foreach, NULL);
#endif
}

#ifndef NDEBUG
void* __heap_manipulate(void* old, size_t t, const char* name, const char* file,
                        int line) {
#else
void* __heap_manipulate(void* old, size_t t, const char* name) {
#endif
  if (old) {
    if (t) {
#ifndef XG2S_DISABLE_HEAP_TRACKING
      struct allocation* allocation = (struct allocation*)g_hash_table_lookup(
          heap_manager->allocations, old);
#endif
      void* np = realloc(old, t);
#ifndef XG2S_DISABLE_HEAP_TRACKING
      heap_manager->memory_usage -= allocation->current_size;
      heap_manager->memory_usage += t;
      heap_manager->max_memory_used =
          MAX(heap_manager->max_memory_used, heap_manager->memory_usage);
      allocation->current_size = t;
      allocation->ptr = np;
      allocation->name = name;
#endif
      return np;
    } else {
#ifndef XG2S_DISABLE_HEAP_TRACKING
      struct allocation* allocation = (struct allocation*)g_hash_table_lookup(
          heap_manager->allocations, old);
      if (!allocation) {
        LOG(ll_error,
            "unable to remove allocation %p, treating as if it leaked", old);
      } else {
        g_hash_table_remove(heap_manager->allocations, old);
        heap_manager->memory_usage -= allocation->current_size;
        free(allocation);
      }
#endif
      free(old);
    }
  } else {
    if (!t) return NULL;
#ifndef XG2S_DISABLE_HEAP_TRACKING
    struct allocation* allocation =
        (struct allocation*)malloc(sizeof(struct allocation));
#endif
    void* np = malloc(t);
#ifndef XG2S_DISABLE_HEAP_TRACKING
    g_hash_table_insert(heap_manager->allocations, np, allocation);
    allocation->ptr = np;
    allocation->current_size = t;
    allocation->name = name;
#ifndef NDEBUG
    allocation->line = line;
    allocation->file = file;
#endif
    heap_manager->memory_usage += t;
    heap_manager->max_memory_used =
        MAX(heap_manager->max_memory_used, heap_manager->memory_usage);
#endif
    return np;
  }
  return NULL;
}
