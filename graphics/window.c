#include "window.h"

#include <GLFW/glfw3.h>
#include <bgfx/c99/bgfx.h>
#include <core/log.h>
#include <core/mem.h>
#include <signal.h>

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

static void __sighandler(int signal) { our_window->quitRequested = true; }

struct window* window_create() {
  struct window* window = HEAP_ALLOC_TYPE(struct window);
  our_window = window;
  window->private = HEAP_ALLOC_TYPE(struct window_private);

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window->private->window = glfwCreateWindow(800, 600, "", NULL, NULL);
  glfwSetWindowSizeLimits(window->private->window, 800, 600, GLFW_DONT_CARE,
                          GLFW_DONT_CARE);

  window->quitRequested = false;

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
  HEAP_FREE(window->private);
  HEAP_FREE(window);
}

void window_poll(struct window* window) {}
