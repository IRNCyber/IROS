BUILD_DIR := build

# Toolchain:
# - On Windows/MinGW, the default `ld` is usually a PE/COFF linker and cannot link ELF (`-m elf_i386` fails).
#   This Makefile auto-selects `ld.lld` or `i686-elf-ld` unless you explicitly override on the command line.
# - On Linux/macOS, use `i686-elf-*` or LLVM `clang` + `ld.lld` + `llvm-objcopy`.
TARGET ?= i686-elf
PY ?= python3

ifeq ($(OS),Windows_NT)
  NULLDEV := NUL
  # CLion/MinGW often runs make recipes under sh.exe; `where` may not exist there.
  # Use cmd.exe explicitly for robust discovery.
  FIND := cmd /c where
else
  NULLDEV := /dev/null
  FIND := command -v
endif

HAVE_LD_LLD := $(strip $(shell $(FIND) ld.lld 2>$(NULLDEV)))
HAVE_LLD := $(strip $(shell $(FIND) lld 2>$(NULLDEV)))
HAVE_I686_LD := $(strip $(shell $(FIND) i686-elf-ld 2>$(NULLDEV)))
HAVE_LLVM_OBJCOPY := $(strip $(shell $(FIND) llvm-objcopy 2>$(NULLDEV)))
HAVE_I686_OBJCOPY := $(strip $(shell $(FIND) i686-elf-objcopy 2>$(NULLDEV)))
HAVE_CLANG := $(strip $(shell $(FIND) clang 2>$(NULLDEV)))
HAVE_I686_GCC := $(strip $(shell $(FIND) i686-elf-gcc 2>$(NULLDEV)))

# Ignore environment-provided CC/LD/OBJCOPY on Windows unless overridden via command line.
ifeq ($(OS),Windows_NT)
  ifneq ($(origin LD),command line)
    ifneq ($(HAVE_LD_LLD),)
      override LD := ld.lld
      override LD_FLAVOR :=
    else ifneq ($(HAVE_LLD),)
      override LD := lld
      override LD_FLAVOR := -flavor gnu
    else ifneq ($(HAVE_I686_LD),)
      override LD := i686-elf-ld
      override LD_FLAVOR :=
    else
      override LD := ld
      override LD_FLAVOR :=
    endif
  endif

  ifneq ($(origin OBJCOPY),command line)
    ifneq ($(HAVE_LLVM_OBJCOPY),)
      override OBJCOPY := llvm-objcopy
    else ifneq ($(HAVE_I686_OBJCOPY),)
      override OBJCOPY := i686-elf-objcopy
    else
      override OBJCOPY := objcopy
    endif
  endif

  ifneq ($(origin CC),command line)
    ifneq ($(HAVE_CLANG),)
      override CC := clang
    else ifneq ($(HAVE_I686_GCC),)
      override CC := i686-elf-gcc
    else
      override CC := gcc
    endif
  endif
else
  CC ?= clang
  LD ?= ld.lld
  LD_FLAVOR ?=
  OBJCOPY ?= llvm-objcopy
endif

CFLAGS := -m32 -ffreestanding -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables -mno-sse -mno-sse2 -mno-mmx -mno-80387 -msoft-float -fno-pic -I include -O2 -Wall -Wextra

ifneq (,$(findstring clang,$(notdir $(CC))))
  CC_TARGET := --target=$(TARGET)
else
  CC_TARGET :=
endif

LD_IS_MINGW_PE := $(filter ld,$(notdir $(LD)))
ifeq ($(OS),Windows_NT)
  ifneq ($(LD_IS_MINGW_PE),)
    $(warning Using linker '$(LD)'. If you see "unrecognised emulation mode: elf_i386", install LLVM and run: mingw32-make LD=ld.lld OBJCOPY=llvm-objcopy CC=clang PY=python)
  endif
endif

KERNEL_OBJS := \
	$(BUILD_DIR)/entry.o \
	$(BUILD_DIR)/isr_stub.o \
	$(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/vga.o \
	$(BUILD_DIR)/log.o \
	$(BUILD_DIR)/memory.o \
	$(BUILD_DIR)/idt.o \
	$(BUILD_DIR)/pic.o \
	$(BUILD_DIR)/isr.o \
	$(BUILD_DIR)/keyboard.o \
	$(BUILD_DIR)/shell.o \
	$(BUILD_DIR)/status.o \
	$(BUILD_DIR)/serial.o \
	$(BUILD_DIR)/apps.o \
	$(BUILD_DIR)/apps_gen.o \
	$(BUILD_DIR)/string.o

.PHONY: all clean run

all: $(BUILD_DIR)/iros.img

ifeq ($(OS),Windows_NT)
  ifeq ($(LD_IS_MINGW_PE),ld)
    $(error Windows MinGW 'ld' cannot link ELF. Install LLVM (clang/ld.lld/llvm-objcopy) or an i686-elf cross toolchain, then run: mingw32-make LD=ld.lld OBJCOPY=llvm-objcopy CC=clang PY=python)
  endif
endif

$(BUILD_DIR):
	$(PY) -c "import os; os.makedirs('$(BUILD_DIR)', exist_ok=True)"

$(BUILD_DIR)/entry.o: kernel/entry.S | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/isr_stub.o: kernel/isr_stub.S | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga.o: kernel/vga.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/log.o: kernel/log.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: kernel/idt.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pic.o: kernel/pic.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/isr.o: kernel/isr.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: kernel/keyboard.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell.o: kernel/shell.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/status.o: kernel/status.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/serial.o: kernel/serial.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/apps.o: kernel/apps.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/apps_gen.c: tools/gen_apps.py $(wildcard apps/*.app)
	$(PY) tools/gen_apps.py

$(BUILD_DIR)/apps_gen.o: kernel/apps_gen.c $(BUILD_DIR)/apps_gen.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/string.o: lib/string.c | $(BUILD_DIR)
	$(CC) $(CC_TARGET) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) linker.ld
	$(LD) $(LD_FLAVOR) -m elf_i386 -T linker.ld -o $@ $(KERNEL_OBJS)

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/boot.bin: boot/boot.S boot/boot.ld $(BUILD_DIR)/kernel.bin | $(BUILD_DIR)
	$(CC) $(CC_TARGET) -c -DKERNEL_SECTORS=$$($(PY) -c "import math,os; print(int(math.ceil(os.path.getsize('$(BUILD_DIR)/kernel.bin')/512.0)))") boot/boot.S -o $(BUILD_DIR)/boot.o
	$(LD) $(LD_FLAVOR) -m elf_i386 -T boot/boot.ld -o $(BUILD_DIR)/boot.elf $(BUILD_DIR)/boot.o
	$(OBJCOPY) -O binary $(BUILD_DIR)/boot.elf $@

$(BUILD_DIR)/iros.img: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	$(PY) tools/mkimg.py --boot $(BUILD_DIR)/boot.bin --kernel $(BUILD_DIR)/kernel.bin --out $@

run: $(BUILD_DIR)/iros.img
	qemu-system-i386 -drive format=raw,file=$(BUILD_DIR)/iros.img

clean:
	rm -rf $(BUILD_DIR)
