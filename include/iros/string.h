#pragma once

#include <iros/types.h>

void *memset(void *dst, int value, usize count);
void *memcpy(void *dst, const void *src, usize count);
usize strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, usize n);
