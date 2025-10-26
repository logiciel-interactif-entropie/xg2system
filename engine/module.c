#include "module.h"

#include <errno.h>
#include <stddef.h>

#include "config.h"
#include "core/log.h"
#include "core/mem.h"
#include "core/module_api.h"

#ifdef __linux
#include <dlfcn.h>
#endif

struct module_private {
#ifdef __linux
  void *mod;
#endif
};

int module_init(struct module *module, const char *module_path) {
  module->private = HEAP_ALLOC_TYPE(struct module_private);
  module->private->mod = dlopen(module_path, RTLD_NOW);
  if (!module->private->mod) return EINVAL;
  return 0;
}

void module_destroy(struct module *module) { HEAP_FREE(module->private); }

struct module_api *module_get_api(struct module *module) {
  mapi_get_api_t f =
      ((mapi_get_api_t)dlsym(module->private->mod, "mapi_get_api"));
  return f((VERSION_MAJOR << 16) | VERSION_MINOR);
}
