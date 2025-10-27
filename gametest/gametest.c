#include <core/module_api.h>
#include <engine/runtime.h>
#include <stddef.h>

#include "engine/debug.h"
#include "engine/resource.h"
#include "engine/transform.h"
#include "flecs.h"
#include "graphics/camera.h"
#include "graphics/model.h"
#include "graphics/shader_program.h"

static const char* __get_module_name() { return "gametest"; }
static const char* __get_module_copyright_long() { return "Public Domain"; }
static const char* __get_module_copyright_short() { return "Public Domain"; }

static void __runtime_create_hook(struct runtime* runtime) {
  ecs_set_name_prefix(runtime->ecs, "GameTest");

  struct resource* shader_test =
      resource_create("shaders://example_instanced.cfg", rsc_shader_program);
  struct resource* model_test = resource_create("test.obj", rsc_model);

  ecs_entity_t model_group = ecs_insert(
      runtime->ecs, ecs_value(model_group_t, {.resource = model_test}),
      ecs_value(shader_t, {.shader = shader_test}));
  int count = 15;
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
      for (int k = 0; k < count; k++) {
        ecs_entity_t entity =
            ecs_insert(runtime->ecs, ecs_value(transform3d_t, {}),
                       ecs_value(model_instance_t, {.group_id = model_group}));
        transform3d_t* tf =
            (transform3d_t*)ecs_get(runtime->ecs, entity, transform3d_t);
        transform_identity(tf);
        tf->translation[0] = (i - ((float)count / 2)) * 4;
        tf->translation[1] = (j - ((float)count / 2)) * 4;
        tf->translation[2] = (k - ((float)count / 2)) * 4;
      }
    }
  }

  ecs_entity_t camera = ecs_insert(
      runtime->ecs,
      ecs_value(camera_lookat_t, {.eye = {5.f, 5.f, 0.f},
                                  .target = {0.f, 0.f, 0.f},
                                  .up = {0.f, 0.f, 1.0f}}),
      ecs_value(camera_t, {.fov = 40.f}),
      ecs_value(
          camera_lookat_spin_t,
          {.distance = count * 2.f, .center = {0.f, 0.f, 0.f}, .time = 0.f}));
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
