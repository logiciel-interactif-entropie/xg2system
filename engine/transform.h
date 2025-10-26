#pragma once
#include <cglm/cglm.h>
#include <cglm/quat.h>
#include <flecs.h>

struct runtime;

typedef struct {
  vec3 translation;
  vec4 quat_rotation;
  vec3 scale;
} transform3d_t;

extern ECS_COMPONENT_DECLARE(transform3d_t);

void runtime_register_transform(struct runtime* runtime);

void transform_identity(transform3d_t* transform);
void transform_mat4(transform3d_t* transform, mat4 matrix);
