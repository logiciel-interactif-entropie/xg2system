#include "camera.h"

#include "bgfx/c99/bgfx.h"
#include "cglm/cam.h"
#include "cglm/project.h"
#include "engine/runtime.h"
#include "engine/transform.h"
#include "flecs.h"
#include "flecs/private/addons.h"

ECS_COMPONENT_DECLARE(camera_t);
ECS_COMPONENT_DECLARE(camera_lookat_t);

static void __camera_set_transform(ecs_iter_t* it) {
  camera_t* camera = ecs_field(it, camera_t, 0);
  transform3d_t* transform = ecs_field(it, transform3d_t, 1);

  if (camera->dirty) {
    glm_perspective(camera->fov, 1.0f, 1000.f, 0.1f, camera->projection);

    camera->dirty = false;
  }

  mat4 view;
  transform_mat4(transform, view);
  bgfx_set_view_transform(0, camera->projection, view);
}

static void __camera_set_transform_lookat(ecs_iter_t* it) {
  camera_t* camera = ecs_field(it, camera_t, 0);
  camera_lookat_t* transform = ecs_field(it, camera_lookat_t, 1);

  if (camera->dirty) {
    glm_perspective(camera->fov, 1.0f, 1000.f, 0.1f, camera->projection);

    camera->dirty = false;
  }

  mat4 view;
  glm_lookat(transform->eye, transform->target, transform->up, view);
  bgfx_set_view_transform(0, camera->projection, view);
}

void runtime_register_camera(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_lookat_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_transform, EcsOnUpdate, camera_t,
             transform3d_t);
  ECS_SYSTEM(runtime->ecs, __camera_set_transform_lookat, EcsOnUpdate, camera_t,
             camera_lookat_t);
}
