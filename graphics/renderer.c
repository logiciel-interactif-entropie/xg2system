#include "renderer.h"

#include <bgfx/c99/bgfx.h>
#include <core/log.h>
#include <graphics/window.h>

#include "bgfx/defines.h"
#include "core/mem.h"

bool __bgfx_debug_mode = false;
bool __bgfx_profiler_mode = false;
bool __bgfx_wireframe_mode = false;

void renderer_init(struct renderer *renderer, struct window *window) {
  renderer->window = window;

  bgfx_platform_data_t pd;
  pd.ndt = window_platform_hwnd_disp(window);
  pd.nwh = window_platform_hwnd(window);
  pd.context = NULL;

  LOG(ll_debug, "pd.ndt = %p", pd.ndt);
  LOG(ll_debug, "pd.nwh = %p", pd.nwh);

  bgfx_init_t bgfx_init_data;
  bgfx_init_ctor(&bgfx_init_data);

  ivec2 resolution;
  window_get_resolution(window, resolution);

  bgfx_init_data.type = BGFX_RENDERER_TYPE_OPENGL;
  bgfx_init_data.resolution.width = resolution[0];
  bgfx_init_data.resolution.height = resolution[1];
  bgfx_init_data.resolution.reset = BGFX_RESET_VSYNC;
#ifndef NDEBUG
  bgfx_init_data.debug = __bgfx_debug_mode;
#endif
  bgfx_init_data.profile = __bgfx_profiler_mode;

  bgfx_init_data.platformData = pd;

  bgfx_init(&bgfx_init_data);
  bgfx_set_view_clear(0,
                      BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL,
                      0x000000ff, 1.0f, 0);
  int bgfx_debug_values = BGFX_DEBUG_TEXT;
  if (__bgfx_debug_mode) bgfx_debug_values |= BGFX_DEBUG_STATS;
  if (__bgfx_profiler_mode) bgfx_debug_values |= BGFX_DEBUG_PROFILER;
  if (__bgfx_wireframe_mode) bgfx_debug_values |= BGFX_DEBUG_WIREFRAME;

#ifndef NDEBUG
  LOG(ll_debug, "Debug mode: %s, Profiler mode: %s",
      __bgfx_debug_mode ? "true" : "false",
      __bgfx_profiler_mode ? "true" : "false");
#endif

  bgfx_set_debug(bgfx_debug_values);

  LOG(ll_debug, "using %s renderer",
      bgfx_get_renderer_name(bgfx_get_renderer_type()));
}

void renderer_runtime_frame(struct renderer *renderer) {
  ivec2 resolution;
  static ivec2 last_resolution;
  window_get_resolution(renderer->window, resolution);

  if (!glm_ivec2_eqv(resolution, last_resolution)) {
    bgfx_reset(resolution[0], resolution[1],
               BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4,
               BGFX_TEXTURE_FORMAT_COUNT);
    glm_ivec2_copy(resolution, last_resolution);
    bgfx_set_view_rect(0, 0, 0, resolution[0], resolution[1]);
  }

  bgfx_touch(0);

  bgfx_frame(false);
}

void renderer_stop(struct renderer *renderer) {
  bgfx_shutdown();
  window_destroy(renderer->window);
}
