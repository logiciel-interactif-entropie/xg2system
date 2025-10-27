#include <core/log.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// gnu extension
#include <argp.h>

#include "config.h"
#include "core/debug.h"
#include "core/fs.h"
#include "core/mem.h"
#include "core/module_api.h"
#include "engine/module.h"
#include "engine/resource.h"
#include "engine/runtime.h"
#include "graphics/resource.h"

const char* argp_program_version = "XG2SYSTEM v" VERSION_STR;
static char doc[] = "logiciel interactif entropie game engine";
static char args_doc[] = "";
static struct argp_option options[] = {
#ifdef _WIN32
    {"game", 'g', "game.dll", 0, "Game module"},
#else
    {"game", 'g', "libgame.so", 0, "Game module"},
#endif
#ifndef NDEBUG
    {"debug_bgfx", 'D', 0, 0, "BGFX debug"},
#endif
    {"profiler_bgfx", 'p', 0, 0, "BGFX profiler"},
    {"wireframe_mode", 'W', 0, 0, "BGFX wireframe mode"},
    {0},
};

extern bool __bgfx_debug_mode;
extern bool __bgfx_profiler_mode;
extern bool __bgfx_wireframe_mode;

struct arguments {
  const char* game_module;
};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
  struct arguments* arguments = state->input;
  switch (key) {
    case 'g':
      arguments->game_module = arg;
      break;
    case 'D':
      __bgfx_debug_mode = true;
      break;
    case 'p':
      __bgfx_profiler_mode = true;
      break;
    case 'W':
      __bgfx_wireframe_mode = true;
      break;
    case ARGP_KEY_ARG:
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = {
    options, parse_opt, args_doc, doc, 0, 0, 0,
};

int main(int argc, char** argv) {
  heap_init();

  struct arguments arguments;
  arguments.game_module = NULL;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (!arguments.game_module) {
    LOG(ll_error, "Please specify game to run using --game argument");
    return EXIT_FAILURE;
  }

  fs_init();

  resources_init();
  resources_init_graphics();

  struct module game_module;
  int status = module_init(&game_module, arguments.game_module);
  if (status) {
    LOG(ll_error, "Error loading module: %s", strerror(status));
    return EXIT_FAILURE;
  }

  struct module_api* api = module_get_api(&game_module);
  ASSERT(api);

  LOG(ll_info, "Module %s is licensed under %s", api->get_module_name(),
      api->get_module_copyright_short());

  struct runtime* runtime = runtime_create();

  if (api->runtime_module_api)
    api->runtime_module_api->runtime_create_hook(runtime);

  runtime_main(runtime);

  LOG(ll_debug, "Exiting gracefully");

  runtime_deinit(runtime);

  resources_destroy_graphics();
  resources_destroy();

  runtime_destroy(runtime);

  fs_destroy();

  heap_end();

  return EXIT_SUCCESS;
}
