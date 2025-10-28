#include <core/module_api.h>
#include <engine/runtime.h>
#include <stddef.h>

#include "engine/debug.h"
#include "engine/physics.h"
#include "engine/resource.h"
#include "engine/transform.h"
#include "flecs.h"
#include "graphics/camera.h"
#include "graphics/infinite_plane.h"
#include "graphics/model.h"
#include "graphics/shader_program.h"
#include "pthread.h"

static const char* __get_module_name() { return "gametest"; }
static const char* __get_module_copyright_long() { return "Public Domain"; }
static const char* __get_module_copyright_short() { return "Public Domain"; }

static void __runtime_create_hook(struct runtime* runtime) {
  ecs_set_name_prefix(runtime->ecs, "GameTest");

  struct resource* shader_test =
      resource_create("shaders://example_instanced.cfg", rsc_shader_program);
  ecs_entity_t physics_world_entity = ecs_entity(runtime->ecs, {});
  ecs_add(runtime->ecs, physics_world_entity, physics_world_t);
  ecs_add_id(runtime->ecs, physics_world_entity, debug_t);

  ecs_entity_t infinite_plane_entity = ecs_insert(
      runtime->ecs,
      ecs_value(infinite_plane_t, {
                                      .plane = {0.f, 0.f, 1.f, -20.f},
                                      .color_a = {1.0f, 0.0f, 0.0f},
                                      .color_b = {0.f, 0.f, 1.0f},
                                  }));

  ecs_entity_t cube_model_group = ecs_insert(
      runtime->ecs,
      ecs_value(model_group_t,
                {.resource = resource_create("assets://cube.obj", rsc_model)}),
      ecs_value(shader_t, {.shader = resource_ref(shader_test)}));
  ecs_entity_t sphere_model_group = ecs_insert(
      runtime->ecs,
      ecs_value(model_group_t, {.resource = resource_create(
                                    "assets://sphere.obj", rsc_model)}),
      ecs_value(shader_t, {.shader = resource_ref(shader_test)}));
  resource_unref(shader_test);
  int count = 10;
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
      for (int k = 0; k < count; k++) {
        ecs_entity_t entity;
        if (k % 2 == 0) {
          entity = ecs_insert(
              runtime->ecs, ecs_value(transform3d_t, {}),
              ecs_value(model_instance_t, {.group_id = cube_model_group}),
              ecs_value(physics_object_t, {.mass = 1.f}),
              ecs_value(physics_object_box_t, {.size = {1.0, 1.0, 1.0}}));
        } else {
          entity = ecs_insert(
              runtime->ecs, ecs_value(transform3d_t, {}),
              ecs_value(model_instance_t, {.group_id = sphere_model_group}),
              ecs_value(physics_object_t, {.mass = 1.f}),
              ecs_value(physics_object_sphere_t, {.radius = 1.0}));
        }
        transform3d_t* tf =
            (transform3d_t*)ecs_get(runtime->ecs, entity, transform3d_t);
        transform_identity(tf);
        tf->translation[0] = (i - ((float)count / 2)) * 4;
        tf->translation[1] = (j - ((float)count / 2)) * 4;
        tf->translation[2] = (k - ((float)count / 2)) * 4;
        tf->dirty = true;
      }
    }
  }

  ecs_entity_t camera =
      ecs_insert(runtime->ecs,
                 ecs_value(camera_lookat_t, {.eye = {5.f, 5.f, 0.f},
                                             .target = {0.f, 0.f, 0.f},
                                             .up = {0.f, 0.f, 1.0f}}),
                 ecs_value(camera_t, {.fov = 40.f}),
                 ecs_value(camera_debug_controller_t,
                           {.speed = 25.f, .mouse_sensitivity = 1.f}));
  ecs_add_id(runtime->ecs, camera, debug_t);
}

struct runtime_module_api m_rt_api_v0 = {
    .runtime_create_hook = __runtime_create_hook,
};

struct module_api m_api_v0 = {
    .get_module_name = __get_module_name,
    .get_module_copyright_long = __get_module_copyright_long,
    .get_module_copyright_short = __get_module_copyright_short,

    .runtime_module_api = &m_rt_api_v0,
};

struct module_api* mapi_get_api(int engine_version) {
  if (engine_version >= (0 << 16)) return &m_api_v0;
  return NULL;
}
