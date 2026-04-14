#pragma once

#include <iros/types.h>

typedef struct {
  usize heap_start;
  usize heap_end;
  usize used_bytes;
  usize free_bytes;
  usize used_blocks;
  usize free_blocks;
} kmem_stats_t;

void kmem_init(void);
void *kmalloc(usize size, usize alignment);
void kfree(void *ptr);
kmem_stats_t kmem_stats(void);
