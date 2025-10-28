#pragma once

#include <bgfx/c99/bgfx.h>
#include <cglm/cglm.h>
#include <engine/resource.h>
#include <flecs.h>

typedef struct {
  vec3 color_a;
  vec3 color_b;
  vec4 plane;  // XYZ, normal. W, distance

  struct resource* shader;

  bool created_handle;
  bgfx_vertex_buffer_handle_t handle;
  bgfx_index_buffer_handle_t index_handle;
  bgfx_vertex_layout_t layout;
} infinite_plane_t;

extern ECS_COMPONENT_DECLARE(infinite_plane_t);

struct runtime;
void runtime_register_infinite_plane(struct runtime* runtime);
