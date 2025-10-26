#pragma once

#include "core/module_api.h"

// platform specific code

struct module_private;
struct module {
  struct module_private* private;
};

int module_init(struct module* module, const char* module_path);
void module_destroy(struct module* module);

struct module_api* module_get_api(struct module* module);
