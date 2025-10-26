#include "runtime.h"

#include "bgfx/c99/bgfx.h"
#include "config.h"
#include "core/mem.h"
#include "engine/debug.h"
#include "engine/resource.h"
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

  // initialize everything

  runtime_register_transform(runtime);
  runtime_register_debug(runtime);
  runtime_register_camera(runtime);
  runtime_register_shader(runtime);
  runtime_register_model(runtime);

  return runtime;
}

void runtime_main(struct runtime* runtime) {
  struct renderer renderer;
  renderer_init(&renderer, window_create());
  while (ecs_progress(runtime->ecs, 0.f) && !renderer.window->quitRequested) {
    window_poll(renderer.window);

    bgfx_dbg_text_printf(0, 0, 0x8f, "XG2SYSTEM v" VERSION_STR);

    resources_tick();
    resources_tick_graphics();

    renderer_runtime_frame(&renderer);
  }
  renderer_stop(&renderer);
}

void runtime_destroy(struct runtime* runtime) { HEAP_FREE(runtime); }
