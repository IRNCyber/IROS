#include <iros/log.h>
#include <iros/paging.h>

enum { PAGE_SIZE = 4096 };
enum { PAGE_PRESENT = 0x01, PAGE_RW = 0x02 };

static u32 page_directory[1024] __attribute__((aligned(4096)));
static u32 page_table_0[1024] __attribute__((aligned(4096)));

void paging_init(void) {
  /* Identity-map the first 4 MiB (one page table covers 4 MiB). */
  for (u32 i = 0; i < 1024; i++) {
    page_table_0[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
  }

  /* First entry of page directory points to our page table. */
  page_directory[0] = ((u32)(usize)page_table_0) | PAGE_PRESENT | PAGE_RW;

  /* Remaining entries are not present. */
  for (u32 i = 1; i < 1024; i++) {
    page_directory[i] = 0;
  }

  /* Load page directory and enable paging. */
  __asm__ volatile(
    "mov %0, %%cr3\n"
    "mov %%cr0, %%eax\n"
    "or $0x80000000, %%eax\n"
    "mov %%eax, %%cr0\n"
    :
    : "r"((u32)(usize)page_directory)
    : "eax", "memory"
  );

  log_info("Paging enabled (4 MiB identity-mapped)");
}
