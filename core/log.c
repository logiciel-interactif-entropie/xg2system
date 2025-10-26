#include "log.h"

#include <stdarg.h>
#include <stdio.h>

const char *log_level_strings[] = {"debug", "info", "warn", "error"};

#ifndef NDEBUG
void __internal_log(enum log_level l, const char *file, int len,
                    const char *restrict fmt, ...) {
#else
void __internal_log(enum log_level l, const char *restrict fmt, ...) {
#endif
  static char buf[0x1000];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
#ifndef NDEBUG
  printf("%s\t%s:%d %s\n", log_level_strings[l], file, len, buf);
#else
  printf("%s\t%s\n", log_level_strings[l], buf);
#endif
  va_end(ap);
}
