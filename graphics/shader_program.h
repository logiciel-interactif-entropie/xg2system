#pragma once

#include <bgfx/c99/bgfx.h>
#include <engine/resource.h>
#include <flecs.h>

struct shader_program_resource {
  bgfx_program_handle_t program;
  bool program_loaded;
};

typedef struct {
  struct resource* shader;
} shader_t;

extern ECS_COMPONENT_DECLARE(shader_t);

struct runtime;
void runtime_register_shader(struct runtime* runtime);
void resource_init_shader_program(struct resource* resource);
struct shader_program_resource* resource_get_shader_program(
    struct resource* resource);
