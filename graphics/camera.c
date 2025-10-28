#include "camera.h"

#include <flecs.h>
#include <math.h>

#include "bgfx/c99/bgfx.h"
#include "cglm/cam.h"
#include "cglm/project.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"
#include "cglm/vec4.h"
#include "core/log.h"
#include "engine/runtime.h"
#include "engine/transform.h"
#include "flecs/addons/flecs_c.h"
#include "flecs/private/api_defines.h"
#include "graphics/renderer.h"
#include "graphics/window.h"

ECS_COMPONENT_DECLARE(camera_t);
ECS_COMPONENT_DECLARE(camera_lookat_t);
ECS_COMPONENT_DECLARE(camera_lookat_spin_t);
ECS_COMPONENT_DECLARE(camera_debug_controller_t);

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

static void __camera_debug_controller(ecs_iter_t* it) {
  struct runtime* runtime = (struct runtime*)ecs_get_ctx(it->world);
  struct window* window = runtime->renderer->window;
  camera_lookat_t* lookat = ecs_field(it, camera_lookat_t, 0);
  camera_debug_controller_t* controller =
      ecs_field(it, camera_debug_controller_t, 1);

  for (int i = 0; i < it->count; i++) {
    camera_lookat_t* _lookat = &lookat[i];
    camera_debug_controller_t* _controller = &controller[i];

    window->mouse_lock_requested = true;

    vec2 delta;
    glm_vec2_zero(delta);
    if (window->mouse_locked) {
      glm_vec2_sub(_controller->last_mouse, window->mouse_position, delta);
      glm_vec2_scale(delta, _controller->mouse_sensitivity * it->delta_time,
                     delta);
      glm_vec2_add(delta, _controller->angles, _controller->angles);
    }

    vec3 angles_xyz;
    angles_xyz[0] = 0.f;
    angles_xyz[1] = -_controller->angles[1];
    angles_xyz[2] = _controller->angles[0];
    mat4 vm;
    glm_euler_zyx(angles_xyz, vm);

    vec4 velocity;
    glm_vec4_zero(velocity);
    if (window->mouse_locked) {
      velocity[0] = (window->keys_down[GLFW_KEY_W] ? 1.0f : 0.0f) +
                    (window->keys_down[GLFW_KEY_S] ? -1.0f : 0.0f);
      velocity[1] = (window->keys_down[GLFW_KEY_A] ? 1.0f : 0.0f) +
                    (window->keys_down[GLFW_KEY_D] ? -1.0f : 0.0f);
    }
    glm_mat4_mulv(vm, velocity, velocity);

    vec3 velocity3;
    glm_vec4_copy3(velocity, velocity3);
    glm_vec3_scale(velocity3, _controller->speed * it->delta_time, velocity3);
    glm_vec3_add(velocity3, _lookat->eye, _lookat->eye);

    vec4 target;
    vec4 target_out;
    glm_vec4_zero(target);
    target[0] = 1.f;
    glm_mat4_mulv(vm, target, target_out);
    vec3 target_cam;
    target_cam[0] = _lookat->eye[0] + target_out[0];
    target_cam[1] = _lookat->eye[1] + target_out[1];
    target_cam[2] = _lookat->eye[2] + target_out[2];
    glm_vec3_copy(target_cam, _lookat->target);

    glm_vec2_copy(window->mouse_position, _controller->last_mouse);
  }
}

void runtime_register_camera(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_lookat_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_lookat_spin_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, camera_debug_controller_t);

  ECS_SYSTEM(runtime->ecs, __camera_update_perspective, EcsPreUpdate, camera_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_lookat_spin, EcsPreUpdate,
             camera_lookat_t, camera_lookat_spin_t);
  ECS_SYSTEM(runtime->ecs, __camera_debug_controller, EcsPreUpdate,
             camera_lookat_t, camera_debug_controller_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_transform, EcsOnUpdate, camera_t,
             transform3d_t);

  ECS_SYSTEM(runtime->ecs, __camera_set_transform_lookat, EcsOnUpdate, camera_t,
             camera_lookat_t);
}
