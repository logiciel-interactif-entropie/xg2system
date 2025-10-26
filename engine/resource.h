#pragma once
#include <stdbool.h>
#include <stddef.h>

#define NR_RESOURCES_TO_PROCESS_TICK 100

/// all resources are managed in memory by the resource manager
/// when done with a resource, call resource_unref on it
struct resource {
  char resource_name[128];
  int references;

  void* resource_user_data;

  void (*on_data_ready)(struct resource* resource, void* data,
                        size_t data_size);
  void (*free_data)(struct resource* resource);
  bool waiting_for_data;
};

enum resource_type {
  rsc_text,
  rsc_image,
  rsc_model,
  rsc_sound,
  rsc_shader_program
};

void resources_init();
void resources_tick();
void resources_destroy();

struct resource* resource_create(const char* resource, enum resource_type type);
// don't call this
void resource_delete(struct resource* resource);
// don't call this either
void resource_destroy(struct resource* resource);
struct resource* resource_get(const char* resource);
void resource_unref(struct resource* resource);
