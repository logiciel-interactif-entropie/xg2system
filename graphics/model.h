#pragma once

#include <bgfx/c99/bgfx.h>
#include <engine/resource.h>
#include <flecs.h>
#include <glib.h>

struct vertex_layout {
  bool initialized;
  bgfx_vertex_layout_handle_t vertex_layout;
  int references;
};

struct mesh {
  bgfx_vertex_buffer_handle_t vertex_buffer;
  bgfx_index_buffer_handle_t index_buffer;
  int num_indices;
  int num_vertices;
};

struct model_resource {
  GArray* meshes;

  bool model_loaded;

  struct vertex_layout* selected_vertex_layout;
};

typedef struct {
  struct resource* resource;
} model_t;

extern ECS_COMPONENT_DECLARE(model_t);

struct runtime;
void runtime_register_model(struct runtime* runtime);
void resource_init_model(struct resource* resource);
struct model_resource* resource_get_model(struct resource* resource);
