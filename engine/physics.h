#pragma once

#include <cglm/cglm.h>
#include <flecs.h>
#include <ode/ode.h>
#include <pthread.h>

#define PHYSICS_RATE (1.0 / 60.0)

typedef struct {
  dWorldID world;
  dSpaceID space;
  dJointGroupID contact_group;

  float last_delta;
} physics_world_t;

typedef struct {
  float mass;

  dBodyID body;
  dGeomID geom;
} physics_object_t;

typedef struct {
  vec3 size;
} physics_object_box_t;

typedef struct {
  float radius;
} physics_object_sphere_t;

extern ECS_COMPONENT_DECLARE(physics_world_t);
extern ECS_COMPONENT_DECLARE(physics_object_t);
extern ECS_COMPONENT_DECLARE(physics_object_box_t);
extern ECS_COMPONENT_DECLARE(physics_object_sphere_t);

struct runtime;
void runtime_register_physics(struct runtime* runtime);
