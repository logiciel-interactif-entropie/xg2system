#include "debug.h"

#include "bgfx/c99/bgfx.h"
#include "core/log.h"
#include "flecs.h"
#include "flecs/addons/flecs_c.h"
#include "graphics/camera.h"
#include "ode/collision_space.h"
#include "physics.h"
#include "runtime.h"
#include "transform.h"

#define COL 0x8f

static int debugIndex;

ECS_TAG_DECLARE(debug_t);

static void __debug_transform3d(ecs_iter_t* it) {
  transform3d_t* _t = ecs_field(it, transform3d_t, 1);
  for (int i = 0; i < it->count; i++) {
    transform3d_t* t = &_t[i];
    bgfx_dbg_text_printf(0, debugIndex++, COL, "%i: (%0.2f, %0.2f, %0.2f)",
                         t->translation[0], t->translation[1],
                         t->translation[2]);
  }
  // LOG(ll_debug, "%0.2fx%0.2fx%0.2f", t->translation);
}

static void __debug_camera(ecs_iter_t* it) {
  camera_t* cam = ecs_field(it, camera_t, 1);
  for (int i = 0; i < it->count; i++) {
    bgfx_dbg_text_printf(0, debugIndex++, COL, "%i: FOV = %f", i, cam[i].fov);
  }
}

static void __debug_camera_lookat(ecs_iter_t* it) {
  camera_lookat_t* cam = ecs_field(it, camera_lookat_t, 1);
  for (int i = 0; i < it->count; i++) {
    bgfx_dbg_text_printf(0, debugIndex++, COL, "%i: P = (%0.2f, %0.2f, %0.2f)",
                         i, cam[i].eye[0], cam[i].eye[1], cam[i].eye[2]);
  }
}

static void __debug_camera_lookat_spin(ecs_iter_t* it) {
  camera_lookat_spin_t* cam = ecs_field(it, camera_lookat_spin_t, 1);
  for (int i = 0; i < it->count; i++) {
    bgfx_dbg_text_printf(0, debugIndex++, COL,
                         "%i: T = %0.1f, D = %0.2f, C = (%0.2f, %0.2f, %0.2f)",
                         i, cam[i].time, cam[i].distance, cam[i].center[0],
                         cam[i].center[1], cam[i].center[2]);
  }
}

static void __debug_physics_world(ecs_iter_t* it) {
  physics_world_t* _physics = ecs_field(it, physics_world_t, 1);
  for (int i = 0; i < it->count; i++) {
    physics_world_t* physics = &_physics[i];
    bgfx_dbg_text_printf(0, debugIndex++, COL, "%i: %i geoms, last delta: %f",
                         i, dSpaceGetNumGeoms(physics->space),
                         physics->last_delta);
    float r = PHYSICS_RATE / physics->last_delta;
    bgfx_dbg_text_printf(0, debugIndex++, COL,
                         "%i: physics is running at %3.2f%% speed", r * 100.f);
  }
}

static void __debug_reset(ecs_iter_t* it) {
  bgfx_dbg_text_clear(0x00, true);
  debugIndex = 2;
  bgfx_dbg_text_printf(0, debugIndex++, COL, "ECS Dt = %f (%0.2fFPS), SDt = %f",
                       it->delta_time, 1.0 / it->delta_time,
                       it->delta_system_time);
}

void runtime_register_debug(struct runtime* runtime) {
  ECS_TAG_DEFINE(runtime->ecs, debug_t);
  ECS_SYSTEM(runtime->ecs, __debug_reset, EcsPreUpdate);
  ECS_SYSTEM(runtime->ecs, __debug_transform3d, EcsPostUpdate, debug_t,
             transform3d_t);
  ECS_SYSTEM(runtime->ecs, __debug_camera, EcsPostUpdate, debug_t, camera_t);
  ECS_SYSTEM(runtime->ecs, __debug_camera_lookat, EcsPostUpdate, debug_t,
             camera_lookat_t);
  ECS_SYSTEM(runtime->ecs, __debug_camera_lookat_spin, EcsPostUpdate, debug_t,
             camera_lookat_spin_t);
  ECS_SYSTEM(runtime->ecs, __debug_physics_world, EcsPostUpdate, debug_t,
             physics_world_t);
}
