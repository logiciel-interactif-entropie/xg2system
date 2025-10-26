#pragma once
#include <stdbool.h>
#include <stddef.h>

struct fs_system {
  void* (*open)(struct fs_system* fs, const char* path, const char* mode);

  int (*read)(struct fs_system* fs, void* ptr, int size, void* hnd);
  int (*write)(struct fs_system* fs, const void* ptr, int size, void* hnd);
  int (*flush)(struct fs_system* fs, void* hnd);

  size_t (*tell)(struct fs_system* fs, void* hnd);
  int (*seek)(struct fs_system* fs, void* hnd, size_t offset, int mode);

  void (*close)(struct fs_system* fs, void* hnd);

  const char* uri;
  bool consider_gen_path;
};

struct file;
typedef struct file* XFILE;

void fs_init();
void fs_destroy();

struct fs_system* fs_add_literal_subdir(const char* system_path);

// will take ownership of the pointer
void fs_add_system(struct fs_system* system);

struct fs_system* get_fs_by_path(const char* path);
struct fs_system* get_fs_by_uri(const char* uri);

XFILE fs_open(const char* path, const char* mode);
bool fs_exists(const char* path);
size_t fs_size(const char* path);

int fs_read_all_contents(const char* path, char** output, int* length);

int fs_read(void* ptr, int size, XFILE hnd);
int fs_write(const void* ptr, int size, XFILE hnd);
size_t fs_tell(XFILE hnd);
int fs_seek(XFILE hnd, size_t offset, int mode);

int fs_flush(XFILE hnd);

void fs_close(XFILE hnd);
