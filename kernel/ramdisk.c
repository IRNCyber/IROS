#include <iros/ramdisk.h>
#include <iros/string.h>
#include <iros/vga.h>

static ramdisk_file_t files[RAMDISK_MAX_FILES];

void ramdisk_init(void) {
  for (u32 i = 0; i < RAMDISK_MAX_FILES; i++) {
    files[i].used = 0;
    files[i].size = 0;
    files[i].name[0] = 0;
  }
}

static ramdisk_file_t *find_file(const char *name) {
  for (u32 i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (files[i].used && strcmp(files[i].name, name) == 0) {
      return &files[i];
    }
  }
  return (ramdisk_file_t *)0;
}

static ramdisk_file_t *find_free(void) {
  for (u32 i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (!files[i].used) return &files[i];
  }
  return (ramdisk_file_t *)0;
}

static void copy_name(char *dst, const char *src) {
  u32 i = 0;
  for (; src[i] && i + 1 < RAMDISK_NAME_LEN; i++) dst[i] = src[i];
  dst[i] = 0;
}

static void copy_data(u8 *dst, const void *src, u32 size) {
  const u8 *s = (const u8 *)src;
  for (u32 i = 0; i < size; i++) dst[i] = s[i];
}

int ramdisk_create(const char *name, const void *data, u32 size) {
  if (size > RAMDISK_MAX_SIZE) return -1;
  if (find_file(name)) return -2;
  ramdisk_file_t *f = find_free();
  if (!f) return -3;
  f->used = 1;
  copy_name(f->name, name);
  f->size = size;
  if (data && size > 0) copy_data(f->data, data, size);
  return 0;
}

int ramdisk_write(const char *name, const void *data, u32 size) {
  if (size > RAMDISK_MAX_SIZE) return -1;
  ramdisk_file_t *f = find_file(name);
  if (!f) return -2;
  f->size = size;
  if (data && size > 0) copy_data(f->data, data, size);
  return 0;
}

const ramdisk_file_t *ramdisk_open(const char *name) {
  return find_file(name);
}

int ramdisk_delete(const char *name) {
  ramdisk_file_t *f = find_file(name);
  if (!f) return -1;
  f->used = 0;
  f->name[0] = 0;
  f->size = 0;
  return 0;
}

void ramdisk_list(void) {
  u32 count = 0;
  for (u32 i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (!files[i].used) continue;
    count++;
  }
  if (count == 0) {
    vga_write("ramdisk: empty\n");
    return;
  }
  for (u32 i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (!files[i].used) continue;
    vga_write("  ");
    vga_write(files[i].name);
    vga_write("  ");
    vga_write_hex(files[i].size);
    vga_write(" bytes\n");
  }
}

u32 ramdisk_count(void) {
  u32 c = 0;
  for (u32 i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (files[i].used) c++;
  }
  return c;
}
