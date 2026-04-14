#include <iros/apps.h>
#include <iros/log.h>
#include <iros/string.h>
#include <iros/vga.h>

static int is_hex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static u8 hex_val(char c) {
  if (c >= '0' && c <= '9') return (u8)(c - '0');
  if (c >= 'a' && c <= 'f') return (u8)(c - 'a' + 10);
  return (u8)(c - 'A' + 10);
}

static void vga_write_ansi_escaped(const char *s) {
  /* Convert common backslash escapes, then run through ANSI parser. */
  char buf[256];
  u32 bi = 0;

  for (u32 i = 0; s[i]; i++) {
    char out = s[i];

    if (out == '\\') {
      char n = s[i + 1];
      if (n == 0) {
        out = '\\';
      } else if (n == 'n') {
        out = '\n';
        i++;
      } else if (n == 'r') {
        out = '\r';
        i++;
      } else if (n == 't') {
        out = '\t';
        i++;
      } else if (n == '\\') {
        out = '\\';
        i++;
      } else if (n == 'e') {
        out = (char)0x1B;
        i++;
      } else if (n == 'x' && is_hex(s[i + 2]) && is_hex(s[i + 3])) {
        out = (char)((hex_val(s[i + 2]) << 4) | hex_val(s[i + 3]));
        i += 3;
      } else {
        /* Unknown escape: keep the backslash. */
        out = '\\';
      }
    }

    /* Clamp to ASCII printable/control range for VGA text mode. */
    unsigned char u = (unsigned char)out;
    if (u >= 0x80) out = '?';

    buf[bi++] = out;
    if (bi >= sizeof(buf) - 1) {
      buf[bi] = 0;
      vga_write_ansi(buf);
      bi = 0;
    }
  }

  if (bi) {
    buf[bi] = 0;
    vga_write_ansi(buf);
  }
}

const iros_app_t *iros_app_find(const char *name) {
  if (!name || !name[0]) return (const iros_app_t *)0;
  for (u32 i = 0; i < iros_apps_count; i++) {
    if (strcmp(iros_apps[i].name, name) == 0) return &iros_apps[i];
  }
  return (const iros_app_t *)0;
}

void iros_app_list(void) {
  vga_write("Installed apps:\n");
  for (u32 i = 0; i < iros_apps_count; i++) {
    vga_write("  ");
    vga_write(iros_apps[i].name);
    vga_write(" - ");
    vga_write(iros_apps[i].desc ? iros_apps[i].desc : "");
    if (iros_apps[i].type && strcmp(iros_apps[i].type, "python") == 0) {
      vga_write(" (python)");
    }
    vga_write("\n");
  }
}

void iros_app_info(const char *name) {
  const iros_app_t *app = iros_app_find(name);
  if (!app) {
    log_error("app not found");
    return;
  }
  vga_write("name: ");
  vga_write(app->name);
  vga_write("\n");
  vga_write("desc: ");
  vga_write(app->desc ? app->desc : "");
  vga_write("\n");
  vga_write("type: ");
  vga_write(app->type ? app->type : "text");
  vga_write("\n");
  if (app->entry && app->entry[0]) {
    vga_write("entry: ");
    vga_write(app->entry);
    vga_write("\n");
  }
}

void iros_app_run(const char *name) {
  const iros_app_t *app = iros_app_find(name);
  if (!app) {
    log_error("app not found");
    return;
  }
  if (app->type && strcmp(app->type, "python") == 0) {
    vga_write("This app is Python and runs on the host (no Python runtime in IROS yet).\n");
    vga_write("Host run:\n");
    vga_write("  powershell -ExecutionPolicy Bypass -File tools\\\\iros-hostrun.ps1 ");
    vga_write(app->name);
    if (app->entry && app->entry[0]) {
      vga_write(" ");
      vga_write(app->entry);
    }
    vga_write("\n");
    return;
  }
  if (app->text && app->text[0]) {
    vga_write_ansi_escaped(app->text);
    if (app->text[strlen(app->text) - 1] != '\n') vga_write("\n");
  }
}
