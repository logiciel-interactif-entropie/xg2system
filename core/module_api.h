#pragma once

struct module_api {
  const char* (*get_module_name)();
  const char* (*get_module_copyright_short)();
  const char* (*get_module_copyright_long)();

  struct runtime_module_api* runtime_module_api;
};

typedef struct module_api* (*mapi_get_api_t)(int engine_version);
