#include "camera.h"

#include <flecs.h>
#include <math.h>

#include "bgfx/c99/bgfx.h"
#include "cglm/cam.h"
#include "cglm/project.h"
#include "core/log.h"
#include "engine/runtime.h"
#include "engine/transform.h"
#include "graphics/renderer.h"
#include "graphics/window.h"

ECS_COMPONENT_DECLARE(camera_t);
ECS_COMPONENT_DECLARE(camera_lookat_t);
ECS_COMPONENT_DECLARE(camera_lookat_spin_t);

static void __camera_update_perspective(ecs_iter_t* it) {
  camera_t* camera = ecs_field(it, camera_t, 0);
  for (int i = 0; i < it->count; i++) {
    struct runtime* runtime = (struct runtime*)ecs_get_ctx(it->world);
    ivec2 window_resolution;
    window_get_resolution(runtime->renderer->window, window_resolution);
    glm_perspective(camera[i].fov,
                    (float)window_resolution[0] / (float)window_resolution[1],
                    0.1f, 1000.f, camera[i].projection);
  }
}

static void __camera_set_transform(ecs_iter_t* it) {
  camera_t* camera = ecs_field(it, camera_t, 0);
  transform3d_t* transform = ecs_field(it, transform3d_t, 1);
  for (int i = 0; i < it->count; i++) {
    mat4 view;
    transform_mat4(&transform[i], view);
    bgfx_set_view_transform(0, view, camera[i].projection);
  }
}

static void __camera_set_transform_lookat(ecs_iter_t* it) {
  camera_t* camera = ecs_field(it, camera_t, 0);
  camera_lookat_t* transform = ecs_field(it, camera_lookat_t, 1);
  for (int i = 0; i < it->count; i++) {
    mat4 view;
    glm_lookat(transform->eye, transform[i].target, transform[i].up, view);
    bgfx_set_view_transform(0, view, camera[i].projection);
  }
}

static void __camera_set_lookat_spin(ecs_iter_t* it) {
  camera_lookat_t* lookat = ecs_field(it, camera_lookat_t, 0);
  camera_lookat_spin_t* spin = ecs_field(it, camera_lookat_spin_t, 1);
  for (int i = 0; i < it->count; i++) {
    camera_lookat_t* _lookat = &lookat[i];
    camera_lookat_spin_t* _spin = &spin[i];
    _lookat->eye[0] = _spin->center[0] + (sinf(_spin->time) * _spin->distance);
    _lookat->eye[1] = _spin->center[1] + (cosf(_spin->time) * _spin->distance);
    _lookat->eye[2] = _spin->center[2] + _spin->distance;
    glm_vec3_copy(_spin->center, _lookat->target);
    _spin->time += it->delta_system_time;
  }
}

void runtime_register_camera(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_lookat_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_lookat_spin_t);

  ECS_SYSTEM(runtime->ecs, __camera_update_perspective, EcsPreUpdate, camera_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_lookat_spin, EcsPreUpdate,
             camera_lookat_t, camera_lookat_spin_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_transform, EcsOnUpdate, camera_t,
             transform3d_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_transform_lookat, EcsOnUpdate, camera_t,
             camera_lookat_t);
}
