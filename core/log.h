#pragma once

enum log_level {
  ll_debug,
  ll_info,
  ll_warn,
  ll_error,
};

#ifndef NDEBUG
void __internal_log(enum log_level level, const char* file, int line,
                    const char* fmt, ...);
#define LOG(L, F...) __internal_log(L, __FILE__, __LINE__, F);
#else
void __log(enum log_level level, const char *fmt, ...);
#define LOG(L, F...) __internal_log(L, F);
#endif
