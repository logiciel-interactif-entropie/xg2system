#include "physics.h"

#include "cglm/mat3.h"
#include "core/debug.h"
#include "core/log.h"
#include "engine/transform.h"
#include "flecs.h"
#include "flecs/addons/flecs_c.h"
#include "flecs/private/addons.h"
#include "ode/collision.h"
#include "ode/collision_space.h"
#include "ode/common.h"
#include "ode/contact.h"
#include "ode/objects.h"
#include "ode/odeconfig.h"
#include "pthread.h"
#include "runtime.h"

ECS_COMPONENT_DECLARE(physics_world_t);
ECS_COMPONENT_DECLARE(physics_object_t);
ECS_COMPONENT_DECLARE(physics_object_box_t);
ECS_COMPONENT_DECLARE(physics_object_sphere_t);

static physics_world_t* __get_world_from_it(ecs_iter_t* it) {
  ecs_query_t* physics_world_q =
      ecs_query(it->world, {.terms = {{ecs_id(physics_world_t)}}});
  ecs_iter_t physics_world_it = ecs_query_iter(it->world, physics_world_q);
  physics_world_t* world = NULL;
  while (ecs_query_next(&physics_world_it)) {
    if (physics_world_it.count) {
      world = ecs_field(&physics_world_it, physics_world_t, 0);
      break;
    }
  }
  ecs_iter_fini(&physics_world_it);
  return world;
}

static void __ode_to_cglm(mat3 M, const float* R) {
  M[0][0] = R[0];
  M[0][1] = R[1];
  M[0][2] = R[2];

  M[1][0] = R[3];
  M[1][1] = R[4];
  M[1][2] = R[5];

  M[2][0] = R[6];
  M[2][1] = R[7];
  M[2][2] = R[8];
}

static void __object_update(ecs_iter_t* it) {
  physics_object_t* _object = ecs_field(it, physics_object_t, 0);
  transform3d_t* _transform = ecs_field(it, transform3d_t, 1);

  for (int i = 0; i < it->count; i++) {
    physics_object_t* object = &_object[i];
    if (!object->geom) continue;
    transform3d_t* transform = &_transform[i];

    if (transform->dirty) {
      dGeomSetPosition(object->geom, transform->translation[0],
                       transform->translation[1], transform->translation[2]);
      transform->dirty = false;
    } else {
      const dReal* p = dGeomGetPosition(object->geom);
      transform->translation[0] = p[0];
      transform->translation[1] = p[1];
      transform->translation[2] = p[2];

      dQuaternion q;
      dGeomGetQuaternion(object->geom, q);
      glm_quat_mat3((vec4){q[1], q[2], q[3], q[0]}, transform->rotation);
    }
  }
}

static void __world_near_callback(void* data, dGeomID o1, dGeomID o2) {
  physics_world_t* world = (physics_world_t*)data;

  dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);
  dContact contact[1000];
  for (int i = 0; i < 1000; i++) {
    contact[i].surface.mode = dContactBounce | dContactSoftCFM;
    contact[i].surface.mu = dInfinity;
    contact[i].surface.mu2 = 0;
    contact[i].surface.bounce = 0.001;
    contact[i].surface.bounce_vel = 0.1;
    contact[i].surface.soft_cfm = 0.01;
  }

  int numc;
  if ((numc = dCollide(o1, o2, 1000, &contact[0].geom, sizeof(dContact)))) {
    for (int i = 0; i < numc; i++) {
      dJointID c =
          dJointCreateContact(world->world, world->contact_group, &contact[i]);
      dJointAttach(c, b1, b2);
    }
  }
}

static void __world_simulate(ecs_iter_t* it) {
  physics_world_t* _world = ecs_field(it, physics_world_t, 0);
  for (int i = 0; i < it->count; i++) {
    physics_world_t* world = &_world[i];
    world->last_delta = it->delta_system_time;

    dSpaceCollide(world->space, world, __world_near_callback);
    dWorldQuickStep(world->world, PHYSICS_RATE);
    dJointGroupEmpty(world->contact_group);
  }
}

static void __init_p_box(ecs_iter_t* it) {
  physics_world_t* world = __get_world_from_it(it);
  physics_object_t* _object = ecs_field(it, physics_object_t, 0);
  physics_object_box_t* _box = ecs_field(it, physics_object_box_t, 1);
  transform3d_t* transform = ecs_field(it, transform3d_t, 2);

  for (int i = 0; i < it->count; i++) {
    physics_object_t* object = &_object[i];
    physics_object_box_t* box = &_box[i];

    if (!object->body) {
      object->body = dBodyCreate(world->world);
      dBodySetData(object->body, (void*)it->ids[i]);

      dMass m;
      dMassSetBox(&m, object->mass, box->size[0], box->size[1], box->size[2]);
      dBodySetMass(object->body, &m);

      object->geom =
          dCreateBox(world->space, box->size[0], box->size[1], box->size[2]);

      dGeomSetBody(object->geom, object->body);
      dGeomSetData(object->geom, (void*)it->ids[i]);
    }
  }
}

static void __init_p_sphere() {}

ECS_CTOR(physics_world_t, world, {
  dInitODE2(0);

  world->world = dWorldCreate();
  world->space = dSimpleSpaceCreate(0);
  world->contact_group = dJointGroupCreate(0);

  dCreatePlane(world->space, 0, 0, 1, -100);

  dWorldSetAutoDisableFlag(world->world, 1);
  dWorldSetContactSurfaceLayer(world->world, 0.001);
  dWorldSetContactMaxCorrectingVel(world->world, 0.9);
  dWorldSetERP(world->world, 0.2);
  dWorldSetCFM(world->world, 1e-5);
  dWorldSetGravity(world->world, 0, 0, -9.81);
});

void runtime_register_physics(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, physics_world_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, physics_object_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, physics_object_box_t);
  ECS_COMPONENT_DEFINE(runtime->ecs, physics_object_sphere_t);

  ECS_SYSTEM(runtime->ecs, __world_simulate, EcsPostUpdate, physics_world_t);
  ecs_set_interval(runtime->ecs, ecs_id(__world_simulate), PHYSICS_RATE);

  ECS_SYSTEM(runtime->ecs, __object_update, EcsOnUpdate, physics_object_t,
             transform3d_t);
  ECS_SYSTEM(runtime->ecs, __init_p_box, EcsPreUpdate, physics_object_t,
             physics_object_box_t, transform3d_t);

  ecs_set_hooks(runtime->ecs, physics_world_t,
                {.ctor = ecs_ctor(physics_world_t)});
}
