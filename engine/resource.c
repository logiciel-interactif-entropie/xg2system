#include "resource.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "core/fs.h"
#include "core/log.h"
#include "core/mem.h"
#include "graphics/image.h"
#include "graphics/model.h"
#include "graphics/shader_program.h"

static struct __resource_manager {
  GHashTable* resource_table;
  GPtrArray* pending_resources;
  GPtrArray* resources_to_unpend;
  int num_resources_tick;
}* __resource_manager;

void resources_init() {
  __resource_manager = HEAP_ALLOC_TYPE(struct __resource_manager);
  __resource_manager->pending_resources = g_ptr_array_new();
  __resource_manager->resource_table =
      g_hash_table_new(g_str_hash, g_str_equal);
}

static void __p_resource_foreach(gpointer data, gpointer userdata) {
  if (__resource_manager->num_resources_tick == 0) return;
  struct resource* resource = (struct resource*)data;

  if (fs_exists(resource->resource_name)) {
    size_t resource_size = fs_size(resource->resource_name);
    XFILE resource_file = fs_open(resource->resource_name, "rb");

    void* resource_data = HEAP_ALLOC(resource_size);
    fs_read(resource_data, resource_size, resource_file);

    if (resource->on_data_ready)
      resource->on_data_ready(resource, resource_data, resource_size);

    LOG(ll_debug, "Loaded %s", resource->resource_name);

    fs_close(resource_file);
    HEAP_FREE(resource_data);
  } else {
    LOG(ll_error, "Could not load resource '%s'", resource->resource_name);
  }

  g_ptr_array_add(__resource_manager->resources_to_unpend, data);
  __resource_manager->num_resources_tick--;
}

static void __up_resource_foreach(gpointer data, gpointer userdata) {
  g_ptr_array_remove(__resource_manager->pending_resources, data);
}

void resources_tick() {
  __resource_manager->num_resources_tick = NR_RESOURCES_TO_PROCESS_TICK;
  __resource_manager->resources_to_unpend = g_ptr_array_new();
  g_ptr_array_foreach(__resource_manager->pending_resources,
                      __p_resource_foreach, NULL);
  g_ptr_array_foreach(__resource_manager->resources_to_unpend,
                      __up_resource_foreach, NULL);
  g_ptr_array_free(__resource_manager->resources_to_unpend, true);
}

static void __ght_destroy_resources(gpointer key, gpointer value,
                                    gpointer user_data) {
  resource_delete((struct resource*)value);
}

void resources_destroy() {
  g_hash_table_foreach(__resource_manager->resource_table,
                       __ght_destroy_resources, NULL);
  g_hash_table_destroy(__resource_manager->resource_table);
  g_ptr_array_free(__resource_manager->pending_resources, true);
  HEAP_FREE(__resource_manager);
}

struct resource* resource_create(const char* resource,
                                 enum resource_type type) {
  struct resource* old_resource = resource_get(resource);
  if (old_resource) {
    return old_resource;
  }

  struct resource* new_resource = HEAP_ALLOC_TYPE(struct resource);
  strncpy(new_resource->resource_name, resource,
          sizeof(new_resource->resource_name));
  new_resource->references = 1;
  new_resource->resource_user_data = NULL;
  new_resource->waiting_for_data = true;
  new_resource->on_data_ready = NULL;
  new_resource->free_data = NULL;
  g_hash_table_insert(__resource_manager->resource_table, (gpointer)resource,
                      (gpointer)new_resource);
  switch (type) {
    case rsc_image:
      resource_init_image(new_resource);
      break;
    case rsc_shader_program:
      resource_init_shader_program(new_resource);
      break;
    case rsc_model:
      resource_init_model(new_resource);
      break;
    default:
      LOG(ll_error, "unknown type");
      break;
  }

  g_ptr_array_add(__resource_manager->pending_resources, new_resource);
  return new_resource;
}

void resource_delete(struct resource* resource) {
  if (resource->references) {
    LOG(ll_warn, "resource '%s' doesn't have all references cleared",
        resource->resource_name);
  }
  if (resource->free_data) resource->free_data(resource);
  HEAP_FREE(resource);
}

void resource_destroy(struct resource* resource) {
  g_hash_table_remove(__resource_manager->resource_table,
                      (gpointer)resource->resource_name);
  resource_delete(resource);
#ifndef NDEBUG
  if (g_hash_table_size(__resource_manager->resource_table) == 0) {
    LOG(ll_debug, "AWESOME! all resources deleted");
  }
#endif
}

struct resource* resource_get(const char* resource) {
  struct resource* rsc = (struct resource*)g_hash_table_lookup(
      __resource_manager->resource_table, (gpointer)resource);
  if (!rsc) return NULL;
  rsc->references++;
  return rsc;
}

void resource_unref(struct resource* resource) {
  resource->references--;
  if (!resource->references) {  // no more references
    resource_destroy(resource);
  }
}

struct resource* resource_ref(struct resource* resource) {
  resource->references++;
  return resource;
}
