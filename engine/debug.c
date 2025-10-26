#include "debug.h"

#include "core/log.h"
#include "flecs/addons/flecs_c.h"
#include "runtime.h"
#include "transform.h"

static int debugIndex = 0;

ECS_TAG_DECLARE(debug_t);

static void __debug_transform3d(ecs_iter_t* it) {
  transform3d_t* t = ecs_field(it, transform3d_t, 1);
  // LOG(ll_debug, "%0.2fx%0.2fx%0.2f", t->translation);
}

void runtime_register_debug(struct runtime* runtime) {
  ECS_TAG_DEFINE(runtime->ecs, debug_t);
  ECS_SYSTEM(runtime->ecs, __debug_transform3d, EcsOnUpdate, debug_t,
             transform3d_t);
}
