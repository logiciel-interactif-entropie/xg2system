#pragma once

#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

struct window_private;

struct window {
  struct window_private* private;
  bool quit_requested;
  bool mouse_lock_requested;

  vec2 mouse_position;
  bool keys_down[GLFW_KEY_LAST + 1];
  bool mouse_buttons_down[GLFW_MOUSE_BUTTON_LAST + 1];
};

struct window* window_create();
void* window_platform_hwnd(struct window* window);
void* window_platform_hwnd_disp(struct window* window);
void window_get_resolution(struct window* window, ivec2 res);
void window_destroy(struct window* window);
void window_poll(struct window* window);
