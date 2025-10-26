#pragma once

#include <flecs.h>
struct runtime;

extern ECS_TAG_DECLARE(debug_t);

void runtime_register_debug(struct runtime* runtime);
