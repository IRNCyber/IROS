#pragma once

#include <iros/types.h>

typedef struct {
  const char *name;
  const char *desc;
  const char *type;   /* "text" or "python" */
  const char *entry;  /* for python: relative entrypoint (e.g. main.py) */
  const char *text;
} iros_app_t;

extern const iros_app_t iros_apps[];
extern const u32 iros_apps_count;

const iros_app_t *iros_app_find(const char *name);
void iros_app_list(void);
void iros_app_info(const char *name);
void iros_app_run(const char *name);
