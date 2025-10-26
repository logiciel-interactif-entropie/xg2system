#pragma once

#include <core/log.h>
#include <stddef.h>
#include <stdlib.h>

#define ASSERT(EXP)                                  \
  {                                                  \
    if (!(EXP)) {                                    \
      LOG(ll_error, "Expression (" #EXP ") failed"); \
      exit(EXIT_FAILURE);                            \
    }                                                \
  }
