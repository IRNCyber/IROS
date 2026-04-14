#pragma once

static inline void cpu_halt(void) {
  __asm__ volatile("hlt");
}

static inline void cpu_cli(void) {
  __asm__ volatile("cli");
}

static inline void cpu_sti(void) {
  __asm__ volatile("sti");
}

