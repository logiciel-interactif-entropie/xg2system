#pragma once

#include <cglm/cglm.h>
#include <flecs.h>
#include <stdbool.h>

struct runtime;

typedef struct {
  float fov;

  bool dirty;

  mat4 projection;
} camera_t;

typedef struct {
  vec3 target;
  vec3 eye;
  vec3 up;
} camera_lookat_t;

extern ECS_COMPONENT_DECLARE(camera_t);
extern ECS_COMPONENT_DECLARE(camera_lookat_t);

void runtime_register_camera(struct runtime* runtime);
