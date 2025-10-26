#include "fs.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "core/mem.h"

struct file {
  void* hnd;
  struct fs_system* fs;
};

struct __fs_system_mgr {
  GArray* fsystems;
}* __fs_system_mgr;

static char* _strndup(const char* s, size_t n) {
  char* p;
  size_t n1;

  for (n1 = 0; n1 < n && s[n1] != '\0'; n1++) continue;
  p = malloc(n + 1);
  if (p != NULL) {
    memcpy(p, s, n1);
    p[n1] = '\0';
  }
  return p;
}

void fs_init() {
  __fs_system_mgr = HEAP_ALLOC_TYPE(struct __fs_system_mgr);
  __fs_system_mgr->fsystems =
      g_array_new(false, true, sizeof(struct fs_system*));

  struct fs_system* system = fs_add_literal_subdir(".");
  system->uri = "pwd";
#ifndef NDEBUG
  struct fs_system* system2 = fs_add_literal_subdir("../assets");
  system2->uri = "assets";
#endif
}

struct fs_literal {
  struct fs_system base;
  const char* path;
};

static void* __fs_literal_open(struct fs_system* fs, const char* path,
                               const char* mode) {
  struct fs_literal* fl = (struct fs_literal*)fs;
  char* new_path = _strndup(fl->path, PATH_MAX);
  strncat(new_path, path, PATH_MAX);
  return (void*)fopen(new_path, mode);
}

static void __fs_literal_close(struct fs_system* fs, void* hnd) {
  fclose((FILE*)hnd);  // its just a FILE under here
}

static int __fs_literal_read(struct fs_system* fs, void* ptr, int size,
                             void* hnd) {
  return fread(ptr, size, 1, (FILE*)hnd);
}

static int __fs_literal_write(struct fs_system* fs, const void* ptr, int size,
                              void* hnd) {
  return fwrite(ptr, size, 1, (FILE*)hnd);
}

static int __fs_literal_flush(struct fs_system* fs, void* hnd) {
  return fflush((FILE*)hnd);
}

static size_t __fs_literal_tell(struct fs_system* fs, void* hnd) {
  return ftell((FILE*)hnd);
}

static int __fs_literal_seek(struct fs_system* fs, void* hnd, size_t offset,
                             int mode) {
  return fseek((FILE*)hnd, offset, mode);
}

struct fs_system* fs_add_literal_subdir(const char* system_path) {
  char* s2path = _strndup(system_path, PATH_MAX);
  strncat(s2path, "/", PATH_MAX);
  struct fs_literal* fs = HEAP_ALLOC_TYPE(struct fs_literal);
  fs->path = s2path;
  fs->base.uri = NULL;
  fs->base.open = __fs_literal_open;
  fs->base.close = __fs_literal_close;
  fs->base.read = __fs_literal_read;
  fs->base.write = __fs_literal_write;
  fs->base.flush = __fs_literal_flush;
  fs->base.tell = __fs_literal_tell;
  fs->base.seek = __fs_literal_seek;
  fs->base.consider_gen_path = true;
  fs_add_system(&fs->base);

  LOG(ll_debug, "mounted literal path %s", system_path);
}

void fs_destroy() { g_array_free(__fs_system_mgr->fsystems, true); }

void fs_add_system(struct fs_system* system) {
  g_array_append_val(__fs_system_mgr->fsystems, system);
}

struct fs_system* get_fs_by_uri(const char* uri) {
  for (int i = 0; i < __fs_system_mgr->fsystems->len; i++) {
    struct fs_system* fs =
        g_array_index(__fs_system_mgr->fsystems, struct fs_system*, i);
    if (!fs->uri) continue;
    if (strcmp(fs->uri, uri) == 0) {
      return fs;
    }
  }
  return NULL;
}

struct fs_system* get_fs_by_path(const char* path) {
  struct fs_system* result = NULL;
  for (int i = 0; i < __fs_system_mgr->fsystems->len; i++) {
    struct fs_system* fs =
        g_array_index(__fs_system_mgr->fsystems, struct fs_system*, i);
    if (!fs->consider_gen_path) continue;
    void* hwnd = fs->open(fs, path, "r");
    if (hwnd) {
      result = fs;
      fs->close(fs, hwnd);
      break;
    }
  }
  return result;
}

struct file* fs_open(const char* path, const char* mode) {
  char* uri_pred = strstr(path, "://");
  struct fs_system* fs;
  XFILE f;
  void* hnd;
  if (uri_pred) {
    size_t uri_len = strlen(path) - strlen(uri_pred);
    char uri_cpy[64];
    strncpy(uri_cpy, path, uri_len);
    fs = get_fs_by_uri(uri_cpy);
    if (!fs) goto failure;
  } else {
    fs = get_fs_by_path(path);
    if (!fs) goto failure;
  }
  hnd = fs->open(fs, path, mode);
  if (!hnd) goto failure;

  f = HEAP_ALLOC_TYPE(struct file);
  f->fs = fs;
  f->hnd = hnd;
  return f;
failure:
  return NULL;
}

bool fs_exists(const char* path) {
  XFILE file = fs_open(path, "r");
  bool test = false;
  if (file) {
    fs_close(file);
    test = true;
  }
  return test;
}

int fs_read_all_contents(const char* path, char** output, int* length) {
  if (!fs_exists(path)) {
    LOG(ll_error, "Could not find file %s", path);
    return 0;
  }
  int size = fs_size(path);
  if (length) *length = size;

  XFILE file = fs_open(path, "r");
  *output = HEAP_ALLOC(size + 1);
  memset(*output, 0, size + 1);

  int c = fs_read(*output, size, file);
  fs_close(file);

  return c;
}

size_t fs_size(const char* path) {
  XFILE file = fs_open(path, "r");
  if (file) {
    fs_seek(file, 0, SEEK_END);
    size_t size = fs_tell(file);
    fs_close(file);
    return size;
  } else
    return 0;
}

int fs_read(void* ptr, int size, struct file* hnd) {
  return hnd->fs->read(hnd->fs, ptr, size, hnd->hnd);
}

int fs_write(const void* ptr, int size, struct file* hnd) {
  if (!hnd->fs->write) {
    LOG(ll_error, "FS doesn't support write operation!");
    return 0;
  }
  return hnd->fs->write(hnd->fs, ptr, size, hnd->hnd);
}

size_t fs_tell(struct file* hnd) { return hnd->fs->tell(hnd->fs, hnd->hnd); }

int fs_seek(struct file* hnd, size_t offset, int mode) {
  return hnd->fs->seek(hnd->fs, hnd->hnd, offset, mode);
}

int fs_flush(struct file* hnd) {
  if (!hnd->fs->flush) {
    LOG(ll_error, "FS doesn't support flush operation!");
    return 0;
  }
  return hnd->fs->flush(hnd->fs, hnd->hnd);
}

void fs_close(struct file* hnd) { hnd->fs->close(hnd->fs, hnd->hnd); }
