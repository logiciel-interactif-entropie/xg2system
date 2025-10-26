#include "image.h"

#include <bgfx/c99/bgfx.h>

#include "core/log.h"
#include "core/mem.h"
#include "engine/resource.h"
#include "graphics/resource.h"

static void __image_graphics_ready(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  struct image_resource* image_resource = resource_get_image(resource);

  image_resource->texture = bgfx_create_texture(
      bgfx_copy(graphics_resource->loaded_graphics_data->data,
                graphics_resource->loaded_graphics_data->size),
      0, false, &image_resource->texture_info);
  image_resource->texture_loaded = true;
  LOG(ll_debug, "loaded texture %ix%i", image_resource->texture_info.width,
      image_resource->texture_info.height);
  HEAP_FREE(graphics_resource->loaded_graphics_data);
}

static void __image_free(struct resource* resource) {
  struct image_resource* image_resource = resource_get_image(resource);
  if (image_resource->texture_loaded)
    bgfx_destroy_texture(image_resource->texture);
  HEAP_FREE(image_resource);
}

void resource_init_image(struct resource* resource) {
  resource_init_graphics(resource);
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  graphics_resource->userdata = HEAP_ALLOC_TYPE(struct image_resource);
  graphics_resource->on_graphics_ready = __image_graphics_ready;
  graphics_resource->free_data = __image_free;

  resource_get_image(resource)->texture_loaded = false;
}

struct image_resource* resource_get_image(struct resource* resource) {
  struct graphics_resource* graphics_resource = resource_get_graphics(resource);
  return (struct image_resource*)graphics_resource->userdata;
}

bgfx_texture_handle_t resource_safe_get_texture(struct resource* resource) {
  struct image_resource* rsc = resource_get_image(resource);
  if (rsc->texture_loaded) {
    return rsc->texture;
  } else {
    if (resource == rsc_missing_texture) {
      bgfx_texture_handle_t t;
      t.idx = 0;
      return t;
    } else {
      return resource_safe_get_texture(rsc_missing_texture);
    }
  }
}
