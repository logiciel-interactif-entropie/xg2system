#pragma once
#include <engine/resource.h>
#include <stdbool.h>

struct graphics_resource {
  bool needs_graphics_upload;

  void (*on_graphics_ready)(struct resource* resource);
  void (*free_data)(struct resource* resource);

  void* userdata;

  struct loaded_graphics_data {
    void* data;
    size_t size;
  }* loaded_graphics_data;
};

extern struct resource* rsc_missing_texture;

void resources_init_graphics();
void resources_tick_graphics();
void resources_destroy_graphics();

void resource_init_graphics(struct resource* resource);
struct graphics_resource* resource_get_graphics(struct resource* resource);
