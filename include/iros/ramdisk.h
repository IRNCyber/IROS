#pragma once

#include <iros/types.h>

enum { RAMDISK_MAX_FILES = 32, RAMDISK_NAME_LEN = 32, RAMDISK_MAX_SIZE = 4096 };

typedef struct {
  char name[RAMDISK_NAME_LEN];
  u32 size;
  u8 data[RAMDISK_MAX_SIZE];
  u8 used;
} ramdisk_file_t;

void ramdisk_init(void);
int ramdisk_create(const char *name, const void *data, u32 size);
int ramdisk_write(const char *name, const void *data, u32 size);
const ramdisk_file_t *ramdisk_open(const char *name);
int ramdisk_delete(const char *name);
void ramdisk_list(void);
u32 ramdisk_count(void);
