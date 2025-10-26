#pragma once

#include <engine/runtime.h>
#include <graphics/window.h>

struct renderer {
  struct window* window;
};

// renderer will own window, so you don't need to free it, only stop the
// renderer (renderer_stop)
void renderer_init(struct renderer* renderer, struct window* window);
void renderer_runtime_frame(struct renderer* renderer);
void renderer_stop(struct renderer* renderer);
