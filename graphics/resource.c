#include "resource.h"

#include <glib.h>
#include <string.h>

#include "core/log.h"
#include "core/mem.h"
#include "engine/resource.h"
#include "graphics/image.h"

static struct __resource_manager {
  GPtrArray* pending_resources;
  GPtrArray* resources_to_unpend;
  int num_resources_tick;
}* __resource_manager;

struct resource* rsc_missing_texture = NULL;

void resources_init_graphics() {
  __resource_manager = HEAP_ALLOC_TYPE(struct __resource_manager);
  __resource_manager->pending_resources = g_ptr_array_new();

  rsc_missing_texture = resource_create("engine/missingtexture.dds", rsc_image);
}

static void __p_resource_foreach(gpointer data, gpointer userdata) {
  struct resource* resource = (struct resource*)data;
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  if (graphics_resource->on_graphics_ready)
    graphics_resource->on_graphics_ready(resource);
  LOG(ll_debug, "Graphics loaded %s", resource->resource_name);
  graphics_resource->needs_graphics_upload = false;
  g_ptr_array_add(__resource_manager->resources_to_unpend, data);
  __resource_manager->num_resources_tick--;
  if (__resource_manager->num_resources_tick == 0) return;
}

static void __up_resource_foreach(gpointer data, gpointer userdata) {
  g_ptr_array_remove(__resource_manager->pending_resources, data);
}

void resources_tick_graphics() {
  __resource_manager->num_resources_tick = NR_RESOURCES_TO_PROCESS_TICK;
  __resource_manager->resources_to_unpend = g_ptr_array_new();
  g_ptr_array_foreach(__resource_manager->pending_resources,
                      __p_resource_foreach, NULL);
  g_ptr_array_foreach(__resource_manager->resources_to_unpend,
                      __up_resource_foreach, NULL);
  g_ptr_array_free(__resource_manager->resources_to_unpend, true);
}

void resources_destroy_graphics() {
  g_ptr_array_free(__resource_manager->pending_resources, true);
  HEAP_FREE(__resource_manager);
}

static void __on_resource_data_ready(struct resource* resource, void* data,
                                     size_t data_size) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  graphics_resource->needs_graphics_upload = true;
  graphics_resource->loaded_graphics_data =
      HEAP_ALLOC_TYPE(struct loaded_graphics_data);
  graphics_resource->loaded_graphics_data->data = HEAP_ALLOC(data_size);
  graphics_resource->loaded_graphics_data->size = data_size;
  memcpy(graphics_resource->loaded_graphics_data->data, data, data_size);
  g_ptr_array_add(__resource_manager->pending_resources, resource);
}

static void __graphics_resource_free(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  if (graphics_resource->free_data) graphics_resource->free_data(resource);

  // we expect that whatever happens in on_graphics_ready will free
  // loaded_graphics_data when it needs to

  HEAP_FREE(graphics_resource);
}

void resource_init_graphics(struct resource* resource) {
  if (resource->resource_user_data)
    LOG(ll_warn, "resource %s already has resource_user_data set, clearing",
        resource->resource_name);
  resource->resource_user_data =
      (void*)HEAP_ALLOC_TYPE(struct graphics_resource);
  resource->on_data_ready = __on_resource_data_ready;

  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  graphics_resource->needs_graphics_upload = false;
  graphics_resource->on_graphics_ready = NULL;
  graphics_resource->userdata = NULL;
  graphics_resource->free_data = __graphics_resource_free;
}

struct graphics_resource* resource_get_graphics(struct resource* resource) {
  return (struct graphics_resource*)resource->resource_user_data;
}
