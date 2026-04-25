#include <iros/keyboard.h>
#include <iros/cpu.h>
#include <iros/format.h>
#include <iros/log.h>
#include <iros/memory.h>
#include <iros/mouse.h>
#include <iros/ramdisk.h>
#include <iros/rtc.h>
#include <iros/serial.h>
#include <iros/status.h>
#include <iros/string.h>
#include <iros/timer.h>
#include <iros/vga.h>
#include <iros/ports.h>
#include <iros/version.h>
#include <iros/apps.h>

enum {
  UI_MODE_SIMPLE = 0,
  UI_MODE_KALI = 1,
  UI_MODE_PANEL = 2,
};

static int ui_mode = UI_MODE_SIMPLE;
static void *last_alloc = (void *)0;

enum { HIST_MAX = 16, HIST_LEN = 128 };
static char history[HIST_MAX][HIST_LEN];
static u32 history_count = 0;
static u32 history_pos = 0;

static const char *skip_ws(const char *s) {
  while (*s == ' ' || *s == '\t') s++;
  return s;
}

static int is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static u32 hex_val(char c) {
  if (c >= '0' && c <= '9') return (u32)(c - '0');
  if (c >= 'a' && c <= 'f') return (u32)(c - 'a' + 10);
  return (u32)(c - 'A' + 10);
}

static int parse_u32(const char *s, u32 *out, const char **end) {
  const char *p = skip_ws(s);
  u32 v = 0;
  int any = 0;

  if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    p += 2;
    while (is_hex_digit(*p)) {
      any = 1;
      v = (v << 4) | hex_val(*p);
      p++;
    }
  } else {
    while (*p >= '0' && *p <= '9') {
      any = 1;
      v = v * 10u + (u32)(*p - '0');
      p++;
    }
  }

  if (!any) return 0;
  if (out) *out = v;
  if (end) *end = p;
  return 1;
}

static void write_u32_dec(u32 v) {
  char tmp[11];
  u32 n = 0;
  if (v == 0) {
    vga_putc('0');
    return;
  }
  while (v && n < sizeof(tmp)) {
    tmp[n++] = (char)('0' + (v % 10u));
    v /= 10u;
  }
  while (n > 0) vga_putc(tmp[--n]);
}

static void print_prompt(void) {
  status_update();

  if (ui_mode == UI_MODE_SIMPLE) {
    vga_write_ansi("\x1b[36mIROS\x1b[0m > ");
    return;
  }

  if (ui_mode == UI_MODE_PANEL) {
    vga_write_ansi("\x1b[30;46m IROS PANEL \x1b[0m ");
    vga_write_ansi("\x1b[37mcmd>\x1b[0m ");
    return;
  }

  const char tl = (char)0xDA;
  const char bl = (char)0xC0;
  const char h  = (char)0xC4;

  char line1[64];
  line1[0] = tl; line1[1] = h; line1[2] = h;
  line1[3] = '('; line1[4] = 'r'; line1[5] = 'o';
  line1[6] = 'o'; line1[7] = 't'; line1[8] = '@';
  line1[9] = 'i'; line1[10] = 'r'; line1[11] = 'o';
  line1[12] = 's'; line1[13] = ')'; line1[14] = '-';
  line1[15] = '['; line1[16] = '~'; line1[17] = ']';
  line1[18] = 0;

  char line2[8];
  line2[0] = bl; line2[1] = h; line2[2] = '#';
  line2[3] = ' '; line2[4] = 0;

  vga_write_ansi("\x1b[36m");
  vga_write(line1);
  vga_write_ansi("\x1b[0m");
  vga_write("\n");

  vga_write_ansi("\x1b[36m");
  vga_write(line2);
  vga_write_ansi("\x1b[0m");
}

static void hist_push(const char *line) {
  const char *p = skip_ws(line);
  if (!p[0]) return;

  u32 slot = history_count % HIST_MAX;
  u32 i = 0;
  for (; p[i] && i + 1 < HIST_LEN; i++) history[slot][i] = p[i];
  history[slot][i] = 0;
  history_count++;
  history_pos = history_count;
}

static const char *hist_get(u32 absolute_index) {
  if (absolute_index >= history_count) return (const char *)0;
  u32 oldest = (history_count > HIST_MAX) ? (history_count - HIST_MAX) : 0;
  if (absolute_index < oldest) return (const char *)0;
  u32 slot = absolute_index % HIST_MAX;
  return history[slot];
}

/* ---- Command handlers ---- */

static void serial_read_to_eot(void) {
  u32 idle = 0;
  for (;;) {
    int b = serial_read_byte();
    if (b < 0) {
      idle++;
      if (idle > 5000000u) {
        vga_write("\n[pybridge timeout]\n");
        break;
      }
      continue;
    }
    idle = 0;
    if (b == 0x04) break;
    char c = (char)(u8)b;
    if (c == '\r') continue;
    vga_putc(c);
  }
  if (status_is_enabled()) status_update();
}

static void cmd_help(const char *args);

static void cmd_clear(const char *args) {
  (void)args;
  vga_clear();
}

static void cmd_mem(const char *args) {
  (void)args;
  kmem_stats_t s = kmem_stats();
  vga_write("heap: ");
  vga_write_hex((u32)s.heap_start);
  vga_write("..");
  vga_write_hex((u32)s.heap_end);
  vga_write("\n");

  vga_write("used: ");
  write_u32_dec((u32)s.used_bytes);
  vga_write(" bytes (");
  vga_write_hex((u32)s.used_bytes);
  vga_write(")  free: ");
  write_u32_dec((u32)s.free_bytes);
  vga_write(" bytes (");
  vga_write_hex((u32)s.free_bytes);
  vga_write(")\n");

  vga_write("blocks used: ");
  write_u32_dec((u32)s.used_blocks);
  vga_write("  free: ");
  write_u32_dec((u32)s.free_blocks);
  vga_write("\n");
}

static void cmd_dmesg(const char *args) {
  (void)args;
  log_dmesg_dump();
}

static void cmd_log(const char *args) {
  const char *p = skip_ws(args);
  if (!p[0]) {
    vga_write("usage: log serial on|off\n");
    return;
  }
  if (strncmp(p, "serial", 6) == 0) {
    p = skip_ws(p + 6);
    if (strncmp(p, "on", 2) == 0) {
      log_set_serial_enabled(1);
      vga_write("log serial: on\n");
    } else if (strncmp(p, "off", 3) == 0) {
      log_set_serial_enabled(0);
      vga_write("log serial: off\n");
    } else {
      vga_write("usage: log serial on|off\n");
    }
    return;
  }
  vga_write("usage: log serial on|off\n");
}

static void cmd_echo(const char *args) {
  if (!args || !args[0]) {
    vga_write("\n");
    return;
  }
  vga_write(args);
  vga_write("\n");
}

static void cmd_py(const char *args) {
  const char *p = skip_ws(args);
  if (!p[0]) {
    vga_write("usage: py <ping|run|exec>\n");
    return;
  }

  char sub[16];
  u32 si = 0;
  while (p[0] && p[0] != ' ' && p[0] != '\t' && si + 1 < sizeof(sub)) {
    sub[si++] = p[0];
    p++;
  }
  sub[si] = 0;
  p = skip_ws(p);

  if (strcmp(sub, "ping") == 0) {
    serial_write("PING\n");
    serial_read_to_eot();
    return;
  }

  if (strcmp(sub, "run") == 0) {
    if (!p[0]) {
      vga_write("usage: py run <app>\n");
      return;
    }
    const iros_app_t *app = iros_app_find(p);
    if (app && app->type && strcmp(app->type, "python") != 0) {
      vga_write("note: app manifest is not marked python; attempting host run anyway\n");
    }
    serial_write("RUN ");
    serial_write(app ? app->name : p);
    serial_write(" ");
    serial_write((app && app->entry) ? app->entry : "");
    serial_write("\n");
    serial_read_to_eot();
    return;
  }

  if (strcmp(sub, "exec") == 0) {
    if (!p[0]) {
      vga_write("usage: py exec <code>\n");
      return;
    }
    serial_write("EXEC ");
    serial_write(p);
    serial_write("\n");
    serial_read_to_eot();
    return;
  }

  vga_write("usage: py <ping|run|exec>\n");
}

static void cmd_version(const char *args) {
  (void)args;
  vga_write("IROS ");
  vga_write(IROS_VERSION);
  vga_write("\n");
}

static void cmd_about(const char *args) {
  (void)args;
  vga_write("IROS: freestanding 32-bit x86 kernel\n");
  vga_write("UI: VGA text + ANSI colors + shell\n");
  vga_write("Input: PS/2 keyboard + PS/2 mouse (IRQ1/IRQ12)\n");
}

static void cmd_banner(const char *args) {
  (void)args;
  vga_write_ansi("\x1b[36m");
  vga_write("  ___ ___  ___  ___\n");
  vga_write(" |_ _| _ \\/ _ \\/ __|\n");
  vga_write("  | ||   / (_) \\__ \\\n");
  vga_write(" |___|_|_\\\\___/|___/\n");
  vga_write_ansi("\x1b[0m");
}

static void cmd_logo(const char *args) {
  (void)args;
  vga_write("IROS logo asset: assets/iros-logo.svg\n");
  vga_write("Note: current kernel runs in VGA text mode, so SVG is host-side branding.\n");
}

static void cmd_color(const char *args) {
  u32 fg = 15, bg = 0;
  const char *p = args;
  if (!parse_u32(p, (u32 *)&fg, &p)) {
    vga_write("usage: color <fg> <bg>\n");
    return;
  }
  if (!parse_u32(p, (u32 *)&bg, &p)) {
    vga_write("usage: color <fg> <bg>\n");
    return;
  }
  if (fg > 15) fg = 15;
  if (bg > 15) bg = 0;
  vga_set_color((u8)fg, (u8)bg);
}

static void cmd_status(const char *args) {
  const char *p = skip_ws(args);
  if (strncmp(p, "on", 2) == 0) status_set_enabled(1);
  else if (strncmp(p, "off", 3) == 0) status_set_enabled(0);
  else vga_write("usage: status on|off\n");
}

static void cmd_prompt(const char *args) {
  const char *p = skip_ws(args);
  if (strncmp(p, "kali", 4) == 0) ui_mode = UI_MODE_KALI;
  else if (strncmp(p, "simple", 6) == 0) ui_mode = UI_MODE_SIMPLE;
  else vga_write("usage: prompt kali|simple\n");
}

static const char *ui_mode_name(void) {
  if (ui_mode == UI_MODE_PANEL) return "panel";
  if (ui_mode == UI_MODE_KALI) return "kali";
  return "simple";
}

static void cmd_ui(const char *args) {
  const char *p = skip_ws(args);
  if (!p[0] || strncmp(p, "show", 4) == 0) {
    vga_write("ui mode: ");
    vga_write(ui_mode_name());
    vga_write("\n");
    return;
  }

  if (strncmp(p, "kali", 4) == 0) {
    ui_mode = UI_MODE_KALI;
    vga_write("ui mode set: kali\n");
    return;
  }
  if (strncmp(p, "simple", 6) == 0) {
    ui_mode = UI_MODE_SIMPLE;
    vga_write("ui mode set: simple\n");
    return;
  }
  if (strncmp(p, "panel", 5) == 0) {
    ui_mode = UI_MODE_PANEL;
    status_set_enabled(1);
    vga_write("ui mode set: panel\n");
    return;
  }

  vga_write("usage: ui show|kali|simple|panel\n");
}

static void cmd_desktop(const char *args) {
  (void)args;
  vga_clear();
  vga_write_ansi("\x1b[30;46m IROS DASHBOARD \x1b[0m");
  vga_write("\n");
  vga_write("----------------------------------------------\n");
  vga_write(" quick actions:\n");
  vga_write("  help      - list commands\n");
  vga_write("  app list  - list installed apps\n");
  vga_write("  mem       - show memory stats\n");
  vga_write("  dmesg     - show kernel logs\n");
  vga_write("  ui panel  - enable panel prompt mode\n");
  vga_write("----------------------------------------------\n");
  vga_write(" tip: drag the right scrollbar with mouse.\n");
}

static void cmd_history(const char *args) {
  (void)args;
  u32 oldest = (history_count > HIST_MAX) ? (history_count - HIST_MAX) : 0;
  if (history_count == oldest) {
    vga_write("history is empty\n");
    return;
  }

  for (u32 i = oldest; i < history_count; i++) {
    const char *line = hist_get(i);
    if (!line) continue;
    write_u32_dec(i);
    vga_write(": ");
    vga_write(line);
    vga_write("\n");
  }
}

static void cmd_keys(const char *args) {
  (void)args;
  vga_write("Keyboard shortcuts:\n");
  vga_write("  Up/Down       - command history\n");
  vga_write("  Left/Right    - move cursor in command line\n");
  vga_write("  Home/End      - jump to start/end of line\n");
  vga_write("  PgUp/PgDn     - scroll shell output (page step)\n");
  vga_write("  Tab           - auto-complete command name\n");
  vga_write("  Ctrl+C        - cancel current line\n");
  vga_write("  Ctrl+L        - clear screen\n");
  vga_write("  Ctrl+Alt+G    - release QEMU mouse/keyboard grab\n");
}

static void cmd_mouse(const char *args) {
  const char *p = skip_ws(args);
  if (!p[0] || strncmp(p, "status", 6) == 0) {
    mouse_state_t s = mouse_get_state();
    vga_write("mouse: ");
    vga_write(s.enabled ? "on" : "off");
    vga_write("  row=");
    write_u32_dec((u32)s.row);
    vga_write(" col=");
    write_u32_dec((u32)s.col);
    vga_write(" buttons=");
    vga_write_hex((u32)s.buttons);
    vga_write("\n");
    return;
  }

  if (strncmp(p, "on", 2) == 0) {
    mouse_set_enabled(1);
    vga_write("mouse pointer: on\n");
    return;
  }

  if (strncmp(p, "off", 3) == 0) {
    mouse_set_enabled(0);
    vga_write("mouse pointer: off\n");
    return;
  }

  if (strncmp(p, "pos", 3) == 0) {
    p = skip_ws(p + 3);
    u32 row = 0, col = 0;
    if (!parse_u32(p, &row, &p) || !parse_u32(p, &col, &p)) {
      vga_write("usage: mouse pos <row> <col>\n");
      return;
    }
    if (row > 23) row = 23;
    if (col > 78) col = 78;
    mouse_set_position((u8)row, (u8)col);
    vga_write("mouse moved\n");
    return;
  }

  vga_write("usage: mouse <status|on|off|pos>\n");
}

static void cmd_alloc(const char *args) {
  u32 size = 64;
  const char *p = skip_ws(args);
  if (*p) {
    if (!parse_u32(p, &size, &p)) {
      vga_write("usage: alloc [size]\n");
      return;
    }
  }
  last_alloc = kmalloc((usize)size, 16);
  vga_write("kmalloc(");
  write_u32_dec(size);
  vga_write(") = ");
  vga_write_hex((u32)(usize)last_alloc);
  vga_write("\n");
}

static void cmd_free(const char *args) {
  (void)args;
  if (!last_alloc) {
    vga_write("nothing to free\n");
    return;
  }
  kfree(last_alloc);
  last_alloc = (void *)0;
  vga_write("freed\n");
}

static void cmd_reboot(const char *args) {
  (void)args;
  cpu_cli();
  outb(0x64, 0xFE);
  for (;;) cpu_halt();
}

static void cmd_halt(const char *args) {
  (void)args;
  cpu_cli();
  for (;;) cpu_halt();
}

static void cmd_uptime(const char *args) {
  (void)args;
  u32 secs = timer_get_seconds();
  u32 h = secs / 3600u;
  u32 m = (secs % 3600u) / 60u;
  u32 s = secs % 60u;
  vga_write("uptime: ");
  write_u32_dec(h);
  vga_write("h ");
  write_u32_dec(m);
  vga_write("m ");
  write_u32_dec(s);
  vga_write("s (");
  write_u32_dec(timer_get_ticks());
  vga_write(" ticks)\n");
}

static void cmd_time(const char *args) {
  (void)args;
  rtc_time_t t;
  rtc_read(&t);
  char ts[9];
  rtc_format_time(&t, ts);
  vga_write(ts);
  vga_write("  ");
  write_u32_dec((u32)t.year);
  vga_write("-");
  if (t.month < 10) vga_putc('0');
  write_u32_dec((u32)t.month);
  vga_write("-");
  if (t.day < 10) vga_putc('0');
  write_u32_dec((u32)t.day);
  vga_write("\n");
}

static void cmd_app(const char *args) {
  const char *p = skip_ws(args);
  if (!p[0]) {
    vga_write("usage: app <list|info|run|install>\n");
    return;
  }

  char sub[16];
  u32 si = 0;
  while (p[0] && p[0] != ' ' && p[0] != '\t' && si + 1 < sizeof(sub)) {
    sub[si++] = p[0];
    p++;
  }
  sub[si] = 0;
  p = skip_ws(p);

  if (strcmp(sub, "list") == 0) {
    iros_app_list();
  } else if (strcmp(sub, "info") == 0) {
    if (!p[0]) {
      vga_write("usage: app info <name>\n");
      return;
    }
    iros_app_info(p);
  } else if (strcmp(sub, "run") == 0) {
    if (!p[0]) {
      vga_write("usage: app run <name>\n");
      return;
    }
    iros_app_run(p);
  } else if (strcmp(sub, "hostrun") == 0) {
    if (!p[0]) {
      vga_write("usage: app hostrun <name>\n");
      return;
    }
    vga_write("Host run:\n");
    vga_write("  powershell -ExecutionPolicy Bypass -File tools\\\\iros-hostrun.ps1 ");
    vga_write(p);
    vga_write("\n");
  } else if (strcmp(sub, "install") == 0) {
    vga_write("App installation is host-assisted.\n");
    vga_write("On the host, run:\n");
    vga_write("  powershell -ExecutionPolicy Bypass -File tools\\\\iros-install.ps1 ");
    vga_write(p[0] ? p : "<git-url>");
    vga_write("\n");
  } else {
    vga_write("usage: app <list|info|run|install>\n");
  }
}

static void cmd_git(const char *args) {
  const char *p = skip_ws(args);
  if (strncmp(p, "install", 7) == 0) {
    p = skip_ws(p + 7);
    vga_write("Host-assisted install:\n");
    vga_write("  powershell -ExecutionPolicy Bypass -File tools\\\\iros-install.ps1 ");
    vga_write(p[0] ? p : "<git-url>");
    vga_write("\n");
    return;
  }
  vga_write("This is not full git. Use: git install <git-url>\n");
}

static void cmd_fs(const char *args) {
  const char *p = skip_ws(args);
  if (!p[0]) {
    vga_write("usage: fs <ls|cat|write|rm|touch>\n");
    return;
  }

  char sub[16];
  u32 si = 0;
  while (p[0] && p[0] != ' ' && p[0] != '\t' && si + 1 < sizeof(sub)) {
    sub[si++] = p[0];
    p++;
  }
  sub[si] = 0;
  p = skip_ws(p);

  if (strcmp(sub, "ls") == 0) {
    ramdisk_list();
  } else if (strcmp(sub, "touch") == 0) {
    if (!p[0]) { vga_write("usage: fs touch <name>\n"); return; }
    int r = ramdisk_create(p, (const void *)0, 0);
    if (r == -2) vga_write("file already exists\n");
    else if (r == -3) vga_write("ramdisk full\n");
    else vga_write("created\n");
  } else if (strcmp(sub, "write") == 0) {
    if (!p[0]) { vga_write("usage: fs write <name> <data>\n"); return; }
    char fname[32];
    u32 fi = 0;
    while (p[0] && p[0] != ' ' && p[0] != '\t' && fi + 1 < sizeof(fname)) {
      fname[fi++] = p[0]; p++;
    }
    fname[fi] = 0;
    p = skip_ws(p);
    /* Create if it doesn't exist. */
    if (!ramdisk_open(fname)) ramdisk_create(fname, (const void *)0, 0);
    u32 dlen = 0;
    while (p[dlen]) dlen++;
    int r = ramdisk_write(fname, (const void *)p, dlen);
    if (r == -1) vga_write("data too large (max 4096)\n");
    else if (r == -2) vga_write("file not found\n");
    else { vga_write("wrote "); write_u32_dec(dlen); vga_write(" bytes\n"); }
  } else if (strcmp(sub, "cat") == 0) {
    if (!p[0]) { vga_write("usage: fs cat <name>\n"); return; }
    const ramdisk_file_t *f = ramdisk_open(p);
    if (!f) { vga_write("file not found\n"); return; }
    for (u32 i = 0; i < f->size; i++) vga_putc((char)f->data[i]);
    vga_write("\n");
  } else if (strcmp(sub, "rm") == 0) {
    if (!p[0]) { vga_write("usage: fs rm <name>\n"); return; }
    int r = ramdisk_delete(p);
    if (r < 0) vga_write("file not found\n");
    else vga_write("deleted\n");
  } else {
    vga_write("usage: fs <ls|cat|write|rm|touch>\n");
  }
}

/* ---- Command dispatch table ---- */

typedef struct {
  const char *name;
  void (*handler)(const char *args);
  const char *help;
} shell_cmd_t;

static const shell_cmd_t commands[] = {
  { "help",    cmd_help,    "list commands" },
  { "clear",   cmd_clear,   "clear screen" },
  { "cls",     cmd_clear,   "clear screen" },
  { "mem",     cmd_mem,     "memory usage (decimal + hex)" },
  { "dmesg",   cmd_dmesg,   "dump kernel log buffer" },
  { "log",     cmd_log,     "log serial on|off" },
  { "echo",    cmd_echo,    "echo <text>" },
  { "app",     cmd_app,     "app <list|info|run|install>" },
  { "git",     cmd_git,     "git install <git-url>" },
  { "py",      cmd_py,      "py <ping|run|exec>" },
  { "version", cmd_version, "print version" },
  { "about",   cmd_about,   "short system info" },
  { "banner",  cmd_banner,  "show banner" },
  { "logo",    cmd_logo,    "show logo info" },
  { "color",   cmd_color,   "color <fg> <bg> (0-15)" },
  { "status",  cmd_status,  "status on|off" },
  { "prompt",  cmd_prompt,  "prompt kali|simple" },
  { "ui",      cmd_ui,      "ui show|kali|simple|panel" },
  { "desktop", cmd_desktop, "draw IROS dashboard" },
  { "history", cmd_history, "show recent commands" },
  { "keys",    cmd_keys,    "show keyboard shortcuts" },
  { "mouse",   cmd_mouse,   "mouse <status|on|off|pos>" },
  { "alloc",   cmd_alloc,   "kmalloc [size] (default 64)" },
  { "free",    cmd_free,    "kfree last alloc" },
  { "uptime",  cmd_uptime,  "show uptime" },
  { "time",    cmd_time,    "show current date/time" },
  { "fs",      cmd_fs,      "fs <ls|cat|write|rm|touch>" },
  { "reboot",  cmd_reboot,  "reboot machine" },
  { "halt",    cmd_halt,    "halt CPU" },
  { (const char *)0, (void (*)(const char *))0, (const char *)0 },
};

enum { CMD_COUNT = sizeof(commands) / sizeof(commands[0]) - 1 };

static void cmd_help(const char *args) {
  (void)args;
  vga_write("Commands:\n");
  for (u32 i = 0; i < CMD_COUNT; i++) {
    /* Skip 'cls' alias to avoid clutter. */
    if (strcmp(commands[i].name, "cls") == 0) continue;
    vga_write("  ");
    vga_write(commands[i].name);
    u32 nlen = 0;
    const char *n = commands[i].name;
    while (n[nlen]) nlen++;
    for (u32 pad = nlen; pad < 14; pad++) vga_putc(' ');
    vga_write("- ");
    vga_write(commands[i].help);
    vga_write("\n");
  }
}

static void handle_line(char *line) {
  const char *p = skip_ws(line);
  if (!p[0]) return;

  char cmd[16];
  u32 ci = 0;
  while (p[0] && p[0] != ' ' && p[0] != '\t' && ci + 1 < sizeof(cmd)) {
    cmd[ci++] = p[0];
    p++;
  }
  cmd[ci] = 0;
  p = skip_ws(p);

  for (u32 i = 0; i < CMD_COUNT; i++) {
    if (strcmp(cmd, commands[i].name) == 0) {
      commands[i].handler(p);
      status_update();
      return;
    }
  }

  log_error("Unknown command");
  status_update();
}

/* ---- Tab completion ---- */

static void tab_complete(char *line, u32 *len, u32 *cursor) {
  /* Extract the first token (what the user has typed so far). */
  u32 prefix_len = 0;
  while (prefix_len < *len && line[prefix_len] != ' ' && line[prefix_len] != '\t') {
    prefix_len++;
  }
  if (prefix_len == 0) return;
  /* Only complete the command name (first token), not arguments. */
  if (*cursor > prefix_len) return;

  const shell_cmd_t *match = (const shell_cmd_t *)0;
  u32 match_count = 0;

  for (u32 i = 0; i < CMD_COUNT; i++) {
    int ok = 1;
    for (u32 j = 0; j < prefix_len; j++) {
      if (commands[i].name[j] != line[j]) { ok = 0; break; }
    }
    if (ok && commands[i].name[prefix_len - 1] != 0) {
      /* name is at least as long as prefix */
      match = &commands[i];
      match_count++;
    }
  }

  if (match_count == 1 && match) {
    /* Erase current display. */
    while (*cursor > 0) { vga_putc('\b'); (*cursor)--; }
    for (u32 i = 0; i < *len; i++) vga_putc(' ');
    for (u32 i = 0; i < *len; i++) vga_putc('\b');

    /* Fill in the matched command. */
    *len = 0;
    for (u32 i = 0; match->name[i] && *len + 1 < HIST_LEN; i++) {
      line[*len] = match->name[i];
      (*len)++;
    }
    /* Add trailing space. */
    if (*len + 1 < HIST_LEN) {
      line[*len] = ' ';
      (*len)++;
    }
    line[*len] = 0;
    *cursor = *len;

    for (u32 i = 0; i < *len; i++) vga_putc(line[i]);
  } else if (match_count > 1) {
    /* Show all matching commands. */
    vga_write("\n");
    for (u32 i = 0; i < CMD_COUNT; i++) {
      int ok = 1;
      for (u32 j = 0; j < prefix_len; j++) {
        if (commands[i].name[j] != line[j]) { ok = 0; break; }
      }
      if (ok && commands[i].name[prefix_len - 1] != 0) {
        vga_write("  ");
        vga_write(commands[i].name);
        vga_write("\n");
      }
    }
    print_prompt();
    for (u32 i = 0; i < *len; i++) vga_putc(line[i]);
    *cursor = *len;
  }
}

/* ---- Line editing helpers ---- */

static void line_redraw_from(const char *line, u32 len, u32 pos) {
  for (u32 i = pos; i < len; i++) vga_putc(line[i]);
  vga_putc(' ');
  for (u32 i = pos; i < len + 1; i++) vga_putc('\b');
}

static void line_replace(char *line, u32 *len, u32 *cursor, const char *text) {
  /* Erase displayed content. */
  while (*cursor > 0) { vga_putc('\b'); (*cursor)--; }
  for (u32 i = 0; i < *len; i++) vga_putc(' ');
  for (u32 i = 0; i < *len; i++) vga_putc('\b');
  /* Copy new text. */
  *len = 0;
  for (u32 i = 0; text[i] && *len + 1 < HIST_LEN; i++) {
    line[*len] = text[i];
    (*len)++;
    vga_putc(text[i]);
  }
  line[*len] = 0;
  *cursor = *len;
}

/* ---- Serial console input ---- */

static int serial_try_getchar(void) {
  if (!serial_can_read()) return -1;
  int b = serial_read_byte();
  if (b < 0) return -1;
  return b;
}

/* ---- Main shell loop ---- */

void shell_run(void) {
  log_info("Interactive shell ready");
  vga_write_ansi("\x1b[36mIROS UI initialized. Type 'ui show' or 'help'.\x1b[0m\n");

  char line[128];
  for (;;) {
    print_prompt();

    u32 len = 0;
    u32 cursor = 0;
    for (;;) {
      /* Poll both keyboard and serial for input. */
      u16 key = keyboard_try_getkey();
      if (key == KEY_NONE) {
        int sb = serial_try_getchar();
        if (sb >= 0) {
          key = (u16)(u8)sb;
        } else {
          cpu_halt();
          continue;
        }
      }
      char c = (key < 0x100) ? (char)(u8)key : 0;

      /* Ctrl+C (ASCII 0x03) */
      if (c == 0x03) {
        vga_write("^C\n");
        len = 0;
        cursor = 0;
        break;
      }

      /* Ctrl+L (ASCII 0x0C) */
      if (c == 0x0C) {
        vga_clear();
        print_prompt();
        for (u32 i = 0; i < len; i++) vga_putc(line[i]);
        cursor = len;
        continue;
      }

      if (key == KEY_UP) {
        if (history_count == 0) continue;
        if (history_pos > 0) history_pos--;
        const char *h = hist_get(history_pos);
        if (!h) continue;
        line_replace(line, &len, &cursor, h);
        continue;
      }

      if (key == KEY_DOWN) {
        if (history_count == 0) continue;
        if (history_pos < history_count) history_pos++;
        if (history_pos == history_count) {
          line_replace(line, &len, &cursor, "");
          continue;
        }
        const char *h = hist_get(history_pos);
        if (!h) continue;
        line_replace(line, &len, &cursor, h);
        continue;
      }

      if (key == KEY_LEFT) {
        if (cursor > 0) {
          cursor--;
          vga_putc('\b');
        }
        continue;
      }

      if (key == KEY_RIGHT) {
        if (cursor < len) {
          vga_putc(line[cursor]);
          cursor++;
        }
        continue;
      }

      if (key == KEY_HOME) {
        while (cursor > 0) { cursor--; vga_putc('\b'); }
        continue;
      }

      if (key == KEY_END) {
        while (cursor < len) { vga_putc(line[cursor]); cursor++; }
        continue;
      }

      if (key == KEY_DEL) {
        if (cursor < len) {
          for (u32 i = cursor; i + 1 < len; i++) line[i] = line[i + 1];
          len--;
          line[len] = 0;
          line_redraw_from(line, len, cursor);
        }
        continue;
      }

      if (key == KEY_PGUP) {
        vga_scroll(+12);
        continue;
      }
      if (key == KEY_PGDN) {
        vga_scroll(-12);
        continue;
      }

      if (c == '\n') {
        vga_write("\n");
        line[len] = 0;
        break;
      }

      if (c == '\b') {
        if (cursor > 0) {
          cursor--;
          for (u32 i = cursor; i + 1 < len; i++) line[i] = line[i + 1];
          len--;
          line[len] = 0;
          vga_putc('\b');
          line_redraw_from(line, len, cursor);
        }
        continue;
      }

      if (c == '\t') {
        tab_complete(line, &len, &cursor);
        continue;
      }

      if (c >= 32 && c <= 126) {
        if (len + 1 < sizeof(line)) {
          /* Insert at cursor position. */
          for (u32 i = len; i > cursor; i--) line[i] = line[i - 1];
          line[cursor] = c;
          len++;
          line[len] = 0;
          /* Print from cursor onward, then move cursor back. */
          for (u32 i = cursor; i < len; i++) vga_putc(line[i]);
          cursor++;
          for (u32 i = cursor; i < len; i++) vga_putc('\b');
        }
      }
    }

    if (len > 0) {
      hist_push(line);
      handle_line(line);
    }
  }
}
