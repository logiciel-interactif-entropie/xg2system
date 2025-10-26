#include "shader_program.h"

#include <glib.h>

#include "bgfx/c99/bgfx.h"
#include "core/fs.h"
#include "core/log.h"
#include "core/mem.h"
#include "engine/resource.h"
#include "engine/runtime.h"
#include "flecs.h"
#include "graphics/resource.h"

ECS_COMPONENT_DECLARE(shader_t);

static void __shader_free(ecs_iter_t* it) {
  shader_t* shader = ecs_field(it, shader_t, it);
  resource_unref(shader->shader);
}

void runtime_register_shader(struct runtime* runtime) {
  ECS_COMPONENT_DEFINE(runtime->ecs, shader_t);
  ECS_OBSERVER(runtime->ecs, __shader_free, EcsOnRemove, shader_t);
}

static void __shader_program_graphics_ready(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  struct shader_program_resource* shader_program_resource =
      resource_get_shader_program(resource);
  GError* error = NULL;
  GKeyFile* shdr_key_file = g_key_file_new();
  g_key_file_load_from_data(
      shdr_key_file, graphics_resource->loaded_graphics_data->data,
      graphics_resource->loaded_graphics_data->size, G_KEY_FILE_NONE, &error);
  if (error) {
    LOG(ll_error, "Could not read shader definition `%s`", error->message);
    goto free_exit;
  }

  enum { unknown, model_shader, compute_shader } outputting_type = unknown;

  gchar* vertex_shader_string = NULL;
  gsize vertex_shader_len = 0;
  gchar* fragment_shader_string = NULL;
  gsize fragment_shader_len = 0;
  gchar* compute_shader_string = NULL;
  gsize compute_shader_len = 0;

  gsize length_groups;
  gchar** groups = g_key_file_get_groups(shdr_key_file, &length_groups);
  for (int i = 0; i < groups; i++) {
    if (!groups[i]) break;
    const char* shdrpath =
        g_key_file_get_string(shdr_key_file, groups[i], "path", &error);
    if (strcmp(groups[i], "vertex shader") == 0) {
      outputting_type = model_shader;
      fs_read_all_contents(shdrpath, &vertex_shader_string, &vertex_shader_len);
    } else if (strcmp(groups[i], "fragment shader") == 0) {
      outputting_type = model_shader;
      fs_read_all_contents(shdrpath, &fragment_shader_string,
                           &fragment_shader_len);
    }
    if (error) {
      LOG(ll_error, "Error loading shader `%s`", error->message);
      goto free_exit;
    }
  }

  switch (outputting_type) {
    case model_shader: {
      if (!vertex_shader_string) {
        LOG(ll_error, "Could not load vertex shader");
        goto free_exit;
      }
      if (!fragment_shader_string) {
        LOG(ll_error, "Could not load vertex shader");
        goto free_exit;
      }
      bgfx_shader_handle_t vertex = bgfx_create_shader(
          bgfx_copy(vertex_shader_string, vertex_shader_len));
      bgfx_shader_handle_t fragment = bgfx_create_shader(
          bgfx_copy(fragment_shader_string, fragment_shader_len));
      shader_program_resource->program =
          bgfx_create_program(vertex, fragment, true);
    } break;
    case unknown:
    default:
      LOG(ll_error, "Error loaded mysterious shader type, failing");
      goto free_exit;
  }

  shader_program_resource->program_loaded = true;

free_exit:
  g_key_file_free(shdr_key_file);
  HEAP_FREE(graphics_resource->loaded_graphics_data);
}

static void __shader_program_free(struct resource* resource) {
  struct shader_program_resource* shader_program_resource =
      resource_get_shader_program(resource);
  if (shader_program_resource->program_loaded)
    bgfx_destroy_program(shader_program_resource->program);
  HEAP_FREE(shader_program_resource);
}

void resource_init_shader_program(struct resource* resource) {
  resource_init_graphics(resource);
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  graphics_resource->userdata = HEAP_ALLOC_TYPE(struct shader_program_resource);
  graphics_resource->on_graphics_ready = __shader_program_graphics_ready;
  graphics_resource->free_data = __shader_program_free;

  resource_get_shader_program(resource)->program_loaded = false;
}

struct shader_program_resource* resource_get_shader_program(
    struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  return (struct shader_program_resource*)graphics_resource->userdata;
}
