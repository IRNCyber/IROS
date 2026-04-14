#include <iros/memory.h>
#include <iros/vga.h>

extern u8 __kernel_end;

typedef struct kmem_block {
  usize size;              /* bytes available after this header (including padding+payload when allocated) */
  usize payload_size;      /* requested payload size (0 if free) */
  u32 padding;             /* bytes between end(header) and payload (allocated only) */
  u8 free;
  u8 _rsv0;
  u16 _rsv1;
  struct kmem_block *next;
} kmem_block_t;

static kmem_block_t *heap_head = (kmem_block_t *)0;
static usize heap_start = 0;
static usize heap_end = 0;

static inline usize align_up(usize value, usize alignment) {
  if (alignment < 2) return value;
  usize mask = alignment - 1;
  return (value + mask) & ~mask;
}

static inline usize min_usize(usize a, usize b) { return a < b ? a : b; }

static usize heap_total(void) { return heap_end > heap_start ? (heap_end - heap_start) : 0; }

static void kmem_panic(const char *msg) {
  vga_write(msg);
  vga_write("\n");
  for (;;) __asm__ volatile("hlt");
}

void kmem_init(void) {
  heap_start = align_up((usize)&__kernel_end, 16);
  heap_end = 0x80000; /* 512 KiB physical, enough for now */
  if (heap_start + sizeof(kmem_block_t) + 16 >= heap_end) {
    kmem_panic("kmem_init: heap too small");
  }

  heap_head = (kmem_block_t *)heap_start;
  heap_head->size = heap_end - heap_start - sizeof(kmem_block_t);
  heap_head->payload_size = 0;
  heap_head->padding = 0;
  heap_head->free = 1;
  heap_head->next = (kmem_block_t *)0;
}

static void split_block(kmem_block_t *block, usize needed) {
  /* needed is bytes consumed from block->size (payload area) */
  if (block->size <= needed + sizeof(kmem_block_t) + 16) return;

  usize new_block_addr = (usize)(block + 1) + needed;
  new_block_addr = align_up(new_block_addr, 16);
  if (new_block_addr + sizeof(kmem_block_t) + 16 >= heap_end) return;

  kmem_block_t *new_block = (kmem_block_t *)new_block_addr;
  usize remaining = (usize)((usize)(block + 1) + block->size) - (usize)(new_block + 1);

  if (remaining < 16) return;

  new_block->size = remaining;
  new_block->payload_size = 0;
  new_block->padding = 0;
  new_block->free = 1;
  new_block->next = block->next;

  block->size = (usize)((usize)new_block - (usize)(block + 1));
  block->next = new_block;
}

void *kmalloc(usize size, usize alignment) {
  if (size == 0) return (void *)0;
  if (alignment == 0) alignment = 16;
  if ((alignment & (alignment - 1)) != 0) alignment = 16;

  for (kmem_block_t *b = heap_head; b; b = b->next) {
    if (!b->free) continue;

    usize payload_base = (usize)(b + 1);
    usize payload_aligned = align_up(payload_base, alignment);
    usize padding = payload_aligned - payload_base;
    usize needed = padding + size;

    if (b->size < needed) continue;

    split_block(b, needed);
    b->free = 0;
    b->payload_size = size;
    b->padding = (u32)min_usize(padding, 0xFFFFFFFFu);

    return (void *)payload_aligned;
  }

  return (void *)0;
}

static void coalesce(void) {
  for (kmem_block_t *b = heap_head; b && b->next; ) {
    if (b->free && b->next->free) {
      kmem_block_t *n = b->next;
      usize end_of_b_payload = (usize)(b + 1) + b->size;
      usize start_of_n = (usize)n;
      if (end_of_b_payload == start_of_n) {
        b->size = b->size + sizeof(kmem_block_t) + n->size;
        b->next = n->next;
        continue;
      }
    }
    b = b->next;
  }
}

void kfree(void *ptr) {
  if (!ptr) return;
  usize p = (usize)ptr;

  for (kmem_block_t *b = heap_head; b; b = b->next) {
    if (b->free) continue;
    usize payload = align_up((usize)(b + 1), (b->padding ? 16 : 16));
    payload = (usize)(b + 1) + (usize)b->padding;
    if (payload == p) {
      b->free = 1;
      b->payload_size = 0;
      b->padding = 0;
      coalesce();
      return;
    }
  }
}

kmem_stats_t kmem_stats(void) {
  kmem_stats_t s;
  s.heap_start = heap_start;
  s.heap_end = heap_end;
  s.used_bytes = 0;
  s.free_bytes = 0;
  s.used_blocks = 0;
  s.free_blocks = 0;

  for (kmem_block_t *b = heap_head; b; b = b->next) {
    if (b->free) {
      s.free_blocks++;
      s.free_bytes += b->size;
    } else {
      s.used_blocks++;
      s.used_bytes += b->payload_size;
    }
  }

  /* Clamp in case of accounting oddities */
  usize total = heap_total();
  if (s.used_bytes > total) s.used_bytes = total;
  if (s.free_bytes > total) s.free_bytes = total;
  return s;
}
