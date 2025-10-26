#pragma once

#include <bgfx/c99/bgfx.h>
#include <engine/resource.h>
struct image_resource {
  bool texture_loaded;
  bgfx_texture_handle_t texture;
  bgfx_texture_info_t texture_info;
};

void resource_init_image(struct resource* resource);
struct image_resource* resource_get_image(struct resource* resource);

// try to call this as much as it is possibel
bgfx_texture_handle_t resource_safe_get_texture(struct resource* resource);
