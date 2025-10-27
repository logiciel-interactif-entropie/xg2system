#include "transform.h"

#include <cglm/cglm.h>

#include "cglm/mat3.h"
#include "cglm/mat4.h"
#include "runtime.h"

ECS_COMPONENT_DECLARE(transform3d_t);

void runtime_register_transform(struct runtime *runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, transform3d_t);
}

void transform_identity(transform3d_t *transform) {
  glm_vec3_zero(transform->translation);
  glm_mat3_identity(transform->rotation);
  glm_vec3_one(transform->scale);
}

void transform_mat4(transform3d_t *transform, mat4 matrix) {
  glm_mat4_identity(matrix);
  glm_translate(matrix, transform->translation);

  mat4 rotation;
  glm_mat4_identity(rotation);
  glm_mat4_ins3(transform->rotation, rotation);
  glm_mat4_mul(matrix, rotation, matrix);

  glm_scale(matrix, transform->scale);
}
