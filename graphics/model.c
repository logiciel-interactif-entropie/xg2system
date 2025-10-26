#include "graphics/model.h"

#include <assimp/scene.h>

#include "bgfx/c99/bgfx.h"
#include "bgfx/defines.h"
#include "cglm/cglm.h"
#include "core/log.h"
#include "core/mem.h"
#include "engine/resource.h"
#include "engine/runtime.h"
#include "engine/transform.h"
#include "flecs.h"
#include "flecs/addons/flecs_c.h"
#include "glib.h"
#include "graphics/resource.h"
#include "graphics/shader_program.h"

ECS_COMPONENT_DECLARE(model_t);

enum vertex_layout_id {
  vli_regular_layout,
  vli_skinned_layout,
  vli_layout_count
};

struct vertex_layout layout_types[vli_layout_count] = {};

struct model_vertex {
  vec3 position;
  vec3 normal;
  vec4 color0;
  vec4 color1;
  vec2 texcoord;
};

static void __initialize_vli(enum vertex_layout_id id) {
  struct vertex_layout* layout = &layout_types[id];
  if (layout->initialized) return;
  layout->initialized = true;
  bgfx_vertex_layout_t layout_template;
  bgfx_vertex_layout_begin(&layout_template, BGFX_RENDERER_TYPE_NOOP);
  bgfx_vertex_layout_add(&layout_template, BGFX_ATTRIB_POSITION, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout_template, BGFX_ATTRIB_NORMAL, 3,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout_template, BGFX_ATTRIB_COLOR0, 4,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout_template, BGFX_ATTRIB_COLOR1, 4,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  bgfx_vertex_layout_add(&layout_template, BGFX_ATTRIB_TEXCOORD0, 2,
                         BGFX_ATTRIB_TYPE_FLOAT, false, false);
  switch (id) {
    case vli_skinned_layout:
      break;
    default:
      break;
  }
  bgfx_vertex_layout_end(&layout_template);
  layout->vertex_layout = bgfx_create_vertex_layout(&layout_template);
}

static void __model_render(ecs_iter_t* it) {
  model_t* model = ecs_field(it, model_t, 0);
  transform3d_t* transform = ecs_field(it, transform3d_t, 1);
  shader_t* shader = ecs_field(it, shader_t, 2);

  if (!model->resource) return;
  struct model_resource* model_resource = resource_get_model(model->resource);
  if (!model_resource->model_loaded) {
    return;
  }
  if (!shader->shader) return;
  struct shader_program_resource* shader_program_resource =
      resource_get_shader_program(shader->shader);
  if (!shader_program_resource->program_loaded) {
    LOG(ll_warn, "Shader resource %s not loaded yet",
        shader->shader->resource_name);
    return;
  }

  mat4 matrix;
  transform_mat4(transform, matrix);
  bgfx_set_transform(matrix, 1);

  for (int i = 0; i < model_resource->meshes->len; i++) {
    struct mesh* mesh = &g_array_index(model_resource->meshes, struct mesh, i);
    bgfx_set_index_buffer(mesh->index_buffer, 0, mesh->num_indices);
    bgfx_set_vertex_buffer_with_layout(
        0, mesh->vertex_buffer, 0, mesh->num_vertices,
        model_resource->selected_vertex_layout->vertex_layout);
    bgfx_submit(0, shader_program_resource->program, 0.f,
                BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
  }
  bgfx_discard(BGFX_DISCARD_ALL);
}

static void __model_free(ecs_iter_t* it) {
  model_t* model = ecs_field(it, model_t, it);
  resource_unref(model->resource);
}

void runtime_register_model(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, model_t);

  ECS_SYSTEM(runtime->ecs, __model_render, EcsOnUpdate, model_t, transform3d_t,
             shader_t);
  ECS_OBSERVER(runtime->ecs, __model_free, EcsOnRemove, model_t);
}

static void __resource_graphics_ready(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  struct model_resource* model_resource = resource_get_model(resource);
}

static void __resource_graphics_free(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  struct model_resource* model_resource = resource_get_model(resource);
  if (model_resource->model_loaded) {
    model_resource->selected_vertex_layout->references--;
    if (model_resource->selected_vertex_layout->references == 0) {
      model_resource->selected_vertex_layout->initialized = false;
      bgfx_destroy_vertex_layout(
          model_resource->selected_vertex_layout->vertex_layout);
    }
    // TODO
  }
  HEAP_FREE(model_resource);
}

void resource_init_model(struct resource* resource) {
  resource_init_graphics(resource);
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  graphics_resource->userdata = HEAP_ALLOC_TYPE(struct model_resource);
  graphics_resource->on_graphics_ready = __resource_graphics_ready;
  graphics_resource->free_data = __resource_graphics_free;

  resource_get_model(resource)->meshes =
      g_array_new(false, true, sizeof(struct mesh));
  resource_get_model(resource)->model_loaded = false;
  resource_get_model(resource)->selected_vertex_layout = NULL;
}

struct model_resource* resource_get_model(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  return (struct model_resource*)graphics_resource->userdata;
}
