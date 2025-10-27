#pragma once

#include <flecs.h>

struct runtime {
  ecs_world_t* ecs;
  struct renderer* renderer;
};

struct runtime* runtime_create();
void runtime_main(struct runtime* runtime);
void runtime_deinit(struct runtime* runtime);
void runtime_destroy(struct runtime* runtime);

struct runtime_module_api {
  void (*runtime_create_hook)(struct runtime*);
};
