#include "window.h"

#include <GLFW/glfw3.h>
#include <bgfx/c99/bgfx.h>
#include <core/log.h>
#include <core/mem.h>
#include <signal.h>
#include <string.h>

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#define GLFW_EXPOSE_NATIVE_X11
#else
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

struct window_private {
  GLFWwindow* window;
};

static struct window* our_window;

static void __sighandler(int signal) { our_window->quit_requested = true; }

static void __window_close_handler(GLFWwindow* window) {
  struct window* win_host = (struct window*)glfwGetWindowUserPointer(window);
  win_host->quit_requested = true;
}

static void __window_key_event(GLFWwindow* window, int key, int scancode,
                               int action, int mods) {
  struct window* win_host = (struct window*)glfwGetWindowUserPointer(window);
  static bool debounce = false;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    win_host->quit_requested = true;
  else if (key == GLFW_KEY_TAB) {
    if (action == GLFW_PRESS) {
      if (!debounce) {
        debounce = true;
        win_host->user_allows_mouse_lock = !win_host->user_allows_mouse_lock;
      }
    } else if (action == GLFW_RELEASE) {
      debounce = false;
    }
  } else
    win_host->keys_down[key] =
        (action == GLFW_PRESS || action == GLFW_REPEAT) ? true : false;
}

static void __window_mouse_button_event(GLFWwindow* window, int button,
                                        int action, int mods) {
  struct window* win_host = (struct window*)glfwGetWindowUserPointer(window);
  win_host->mouse_buttons_down[button] = action == GLFW_PRESS ? true : false;
}

static void __window_cursor_pos_event(GLFWwindow* window, double xpos,
                                      double ypos) {
  struct window* win_host = (struct window*)glfwGetWindowUserPointer(window);
  win_host->mouse_position[0] = xpos;
  win_host->mouse_position[1] = ypos;
}

struct window* window_create() {
  struct window* window = HEAP_ALLOC_TYPE(struct window);
  our_window = window;
  window->private = HEAP_ALLOC_TYPE(struct window_private);

  memset(window->keys_down, 0, sizeof(window->keys_down));
  memset(window->mouse_buttons_down, 0, sizeof(window->mouse_buttons_down));

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window->private->window = glfwCreateWindow(800, 600, "", NULL, NULL);
  glfwSetWindowSizeLimits(window->private->window, 800, 600, GLFW_DONT_CARE,
                          GLFW_DONT_CARE);
  glfwSetWindowUserPointer(window->private->window, window);

  glfwSetWindowCloseCallback(window->private->window, __window_close_handler);
  glfwSetKeyCallback(window->private->window, __window_key_event);
  glfwSetMouseButtonCallback(window->private->window,
                             __window_mouse_button_event);
  glfwSetCursorPosCallback(window->private->window, __window_cursor_pos_event);

  window->quit_requested = false;
  window->mouse_lock_requested = false;
  window->user_allows_mouse_lock = true;
  window->mouse_locked = false;

  signal(SIGINT, __sighandler);

  return window;
}

void* window_platform_hwnd(struct window* window) {
  return (void*)glfwGetX11Window(window->private->window);
}

void* window_platform_hwnd_disp(struct window* window) {
  return glfwGetX11Display();
}

void window_get_resolution(struct window* window, ivec2 res) {
  glfwGetWindowSize(window->private->window, &res[0], &res[1]);
}

void window_destroy(struct window* window) {
  glfwDestroyWindow(window->private->window);
  HEAP_FREE(window->private);
  HEAP_FREE(window);
}

void window_poll(struct window* window) {
  glfwPollEvents();

  int focused = glfwGetWindowAttrib(window->private->window, GLFW_FOCUSED);
  window->mouse_locked = (window->mouse_lock_requested &&
                          window->user_allows_mouse_lock && focused);

  if (window->mouse_locked) {
    glfwSetInputMode(window->private->window, GLFW_CURSOR,
                     GLFW_CURSOR_DISABLED);
  } else {
    glfwSetInputMode(window->private->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}
