#pragma once

#include <cglm/cglm.h>

struct window_private;

struct window {
  struct window_private* private;
  bool quitRequested;
};

struct window* window_create();
void* window_platform_hwnd(struct window* window);
void* window_platform_hwnd_disp(struct window* window);
void window_get_resolution(struct window* window, ivec2 res);
void window_destroy(struct window* window);
void window_poll(struct window* window);
