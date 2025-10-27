#pragma once

#include <cglm/cglm.h>
#include <flecs.h>
#include <stdbool.h>

struct runtime;

typedef struct {
  float fov;

  mat4 projection;
} camera_t;

typedef struct {
  vec3 target;
  vec3 eye;
  vec3 up;
} camera_lookat_t;

typedef struct {
  vec3 center;
  float distance;
  float time;
} camera_lookat_spin_t;

typedef struct {
  float speed;
  float mouse_sensitivity;
  vec2 angles;
  vec2 last_mouse;
} camera_debug_controller_t;

extern ECS_COMPONENT_DECLARE(camera_t);
extern ECS_COMPONENT_DECLARE(camera_lookat_t);
extern ECS_COMPONENT_DECLARE(camera_lookat_spin_t);
extern ECS_COMPONENT_DECLARE(camera_debug_controller_t);

void runtime_register_camera(struct runtime* runtime);
