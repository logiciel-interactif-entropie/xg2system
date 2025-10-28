#include "runtime.h"

#include "bgfx/c99/bgfx.h"
#include "config.h"
#include "core/mem.h"
#include "engine/debug.h"
#include "engine/physics.h"
#include "engine/resource.h"
#include "flecs.h"
#include "graphics/infinite_plane.h"
#include "graphics/model.h"
#include "graphics/shader_program.h"
#include "graphics/window.h"

// all the things we will register to ecs
#include "engine/transform.h"
#include "graphics/camera.h"
#include "graphics/renderer.h"
#include "graphics/resource.h"

struct runtime* runtime_create() {
  struct runtime* runtime = HEAP_ALLOC_TYPE(struct runtime);
  runtime->ecs = ecs_init();
  ecs_set_ctx(runtime->ecs, runtime, NULL);
  ecs_set_threads(runtime->ecs, 8);

  // initialize everything

  runtime_register_transform(runtime);
  runtime_register_physics(runtime);
  runtime_register_camera(runtime);
  runtime_register_shader(runtime);
  runtime_register_model(runtime);

  runtime_register_infinite_plane(runtime);

  runtime_register_debug(runtime);

  runtime->renderer = HEAP_ALLOC_TYPE(struct renderer);
  renderer_init(runtime->renderer, window_create());

  return runtime;
}

void runtime_main(struct runtime* runtime) {
  while (ecs_progress(runtime->ecs, 0.f) &&
         !runtime->renderer->window->quit_requested) {
    window_poll(runtime->renderer->window);

    bgfx_dbg_text_printf(0, 0, 0x8f, "XG2SYSTEM v" VERSION_STR);

    resources_tick();
    resources_tick_graphics();

    renderer_runtime_frame(runtime->renderer);
  }
}

void runtime_deinit(struct runtime* runtime) { ecs_fini(runtime->ecs); }

void runtime_destroy(struct runtime* runtime) {
  renderer_stop(runtime->renderer);
  HEAP_FREE(runtime->renderer);
  HEAP_FREE(runtime);
}
