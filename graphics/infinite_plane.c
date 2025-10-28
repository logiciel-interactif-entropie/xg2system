#include "infinite_plane.h"

#include <stdint.h>

#include "bgfx/c99/bgfx.h"
#include "bgfx/defines.h"
#include "engine/resource.h"
#include "engine/runtime.h"
#include "flecs/addons/flecs_c.h"
#include "graphics/shader_program.h"

ECS_COMPONENT_DECLARE(infinite_plane_t);

static void __infinite_plane_render(ecs_iter_t* it) {
  infinite_plane_t* _infinite_plane = ecs_field(it, infinite_plane_t, 0);
  for (int i = 0; i < it->count; i++) {
    infinite_plane_t* infinite_plane = &_infinite_plane[i];
    if (!infinite_plane->created_handle) {
      float vertex_buffer[] = {
          -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f, 0.5f, -0.5f, -0.5f, -0.5f,
      };
      uint16_t index_buffer[] = {0, 1, 2, 0, 2, 3};

      bgfx_vertex_layout_begin(&infinite_plane->layout,
                               BGFX_RENDERER_TYPE_NOOP);
      bgfx_vertex_layout_add(&infinite_plane->layout, BGFX_ATTRIB_POSITION, 3,
                             BGFX_ATTRIB_TYPE_FLOAT, false, false);
      bgfx_vertex_layout_end(&infinite_plane->layout);

      bgfx_create_vertex_buffer(bgfx_copy(vertex_buffer, sizeof(vertex_buffer)),
                                &infinite_plane->layout, BGFX_BUFFER_NONE);
      bgfx_create_index_buffer(bgfx_copy(index_buffer, sizeof(index_buffer)),
                               BGFX_BUFFER_NONE);

      infinite_plane->shader =
          resource_create("shaders://infinite_plane.cfg", rsc_shader_program);
    }

    struct shader_program_resource* shader =
        resource_get_shader_program(infinite_plane->shader);
    if (!shader->program_loaded) continue;

    bgfx_encoder_t* encoder = bgfx_encoder_begin(false);
    bgfx_encoder_set_vertex_buffer(encoder, 0, infinite_plane->handle, 0, 4);
    bgfx_encoder_set_index_buffer(encoder, infinite_plane->index_handle, 0, 6);
    bgfx_encoder_submit(encoder, 0, shader->program, 0, BGFX_DISCARD_ALL);
    bgfx_encoder_end(encoder);
  }
}

static void __infinite_plane_free(ecs_iter_t* it) {
  infinite_plane_t* _infinite_plane = ecs_field(it, infinite_plane_t, 0);
  for (int i = 0; i < it->count; i++) {
    infinite_plane_t* infinite_plane = &_infinite_plane[i];
    if (infinite_plane->created_handle) {
      bgfx_destroy_vertex_buffer(infinite_plane->handle);
    }
    resource_unref(infinite_plane->shader);
  }
}

void runtime_register_infinite_plane(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, infinite_plane_t);
  ECS_SYSTEM(runtime->ecs, __infinite_plane_render, EcsOnUpdate,
             infinite_plane_t);
  ECS_OBSERVER(runtime->ecs, __infinite_plane_free, EcsOnRemove,
               __infinite_plane_free);
}
