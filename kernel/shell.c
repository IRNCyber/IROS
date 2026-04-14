#include <iros/keyboard.h>
#include <iros/cpu.h>
#include <iros/log.h>
#include <iros/memory.h>
#include <iros/status.h>
#include <iros/string.h>
#include <iros/vga.h>
#include <iros/ports.h>
#include <iros/version.h>
#include <iros/apps.h>
#include <iros/serial.h>

static int use_kali_prompt = 1;
static void *last_alloc = (void *)0;

enum { HIST_MAX = 16, HIST_LEN = 128 };
static char history[HIST_MAX][HIST_LEN];
static u32 history_count = 0;
static u32 history_pos = 0; /* browsing position: 0..history_count */

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

static void print_prompt(void) {
  status_update();

  if (!use_kali_prompt) {
    vga_write_ansi("\x1b[36mIROS\x1b[0m > ");
    return;
  }

  /* CP437 box drawing characters (avoid UTF-8 in VGA text mode). */
  const char tl = (char)0xDA; /* ┌ */
  const char bl = (char)0xC0; /* └ */
  const char h  = (char)0xC4; /* ─ */

  char line1[64];
  line1[0] = tl;
  line1[1] = h;
  line1[2] = h;
  line1[3] = '(';
  line1[4] = 'r';
  line1[5] = 'o';
  line1[6] = 'o';
  line1[7] = 't';
  line1[8] = '@';
  line1[9] = 'i';
  line1[10] = 'r';
  line1[11] = 'o';
  line1[12] = 's';
  line1[13] = ')';
  line1[14] = '-';
  line1[15] = '[';
  line1[16] = '~';
  line1[17] = ']';
  line1[18] = 0;

  char line2[8];
  line2[0] = bl;
  line2[1] = h;
  line2[2] = '#';
  line2[3] = ' ';
  line2[4] = 0;

  /* Kali-like color scheme via ANSI SGR. */
  vga_write_ansi("\x1b[36m"); /* cyan */
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
  history_pos = history_count; /* reset to end */
}

static const char *hist_get(u32 absolute_index) {
  if (absolute_index >= history_count) return (const char *)0;
  u32 oldest = (history_count > HIST_MAX) ? (history_count - HIST_MAX) : 0;
  if (absolute_index < oldest) return (const char *)0;
  u32 slot = absolute_index % HIST_MAX;
  return history[slot];
}

static void cmd_help(void) {
  vga_write("Commands:\n");
  vga_write("  help            - list commands\n");
  vga_write("  clear | cls     - clear screen\n");
  vga_write("  mem             - memory usage\n");
  vga_write("  echo <text>     - echo arguments\n");
  vga_write("  app list        - list installed apps\n");
  vga_write("  app info <name> - show app info\n");
  vga_write("  app run <name>  - run an app\n");
  vga_write("  app hostrun <name> - host-run an app\n");
  vga_write("  app install <git-url> - host-assisted install\n");
  vga_write("  git install <git-url> - alias for app install\n");
  vga_write("  py ping         - test python bridge (serial)\n");
  vga_write("  py run <app>    - run python app via bridge\n");
  vga_write("  py exec <code>  - run python -c <code> via bridge\n");
  vga_write("  version         - print version\n");
  vga_write("  about           - short system info\n");
  vga_write("  banner          - show banner\n");
  vga_write("  color <fg> <bg> - set VGA color (0-15)\n");
  vga_write("  status on|off   - toggle status bar\n");
  vga_write("  prompt kali|simple - toggle prompt style\n");
  vga_write("  alloc [size]    - kmalloc (default 64)\n");
  vga_write("  free            - kfree last alloc\n");
  vga_write("  reboot          - reboot machine\n");
  vga_write("  halt            - halt CPU\n");
}

static void serial_read_to_eot(void) {
  /* Best-effort timeout to avoid hanging the shell if the host bridge isn't connected. */
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
    /* Don't hard-require type=python: host bridge can run any installed repo name. */
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

static void cmd_mem(void) {
  kmem_stats_t s = kmem_stats();
  vga_write("heap: ");
  vga_write_hex((u32)s.heap_start);
  vga_write("..");
  vga_write_hex((u32)s.heap_end);
  vga_write("\n");

  vga_write("used bytes: ");
  vga_write_hex((u32)s.used_bytes);
  vga_write("  free bytes: ");
  vga_write_hex((u32)s.free_bytes);
  vga_write("\n");

  vga_write("used blocks: ");
  vga_write_hex((u32)s.used_blocks);
  vga_write("  free blocks: ");
  vga_write_hex((u32)s.free_blocks);
  vga_write("\n");
}

static void cmd_echo(const char *args) {
  if (!args || !args[0]) {
    vga_write("\n");
    return;
  }
  vga_write(args);
  vga_write("\n");
}

static void cmd_version(void) {
  vga_write("IROS ");
  vga_write(IROS_VERSION);
  vga_write("\n");
}

static void cmd_about(void) {
  vga_write("IROS: freestanding 32-bit x86 kernel\n");
  vga_write("UI: VGA text + ANSI colors + shell\n");
  vga_write("Input: PS/2 keyboard (IRQ1)\n");
}

static void cmd_banner(void) {
  vga_write_ansi("\x1b[36m");
  vga_write("  ___ ___  ___  ___\n");
  vga_write(" |_ _| _ \\/ _ \\/ __|\n");
  vga_write("  | ||   / (_) \\__ \\\n");
  vga_write(" |___|_|_\\\\___/|___/\n");
  vga_write_ansi("\x1b[0m");
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
  if (strncmp(p, "kali", 4) == 0) use_kali_prompt = 1;
  else if (strncmp(p, "simple", 6) == 0) use_kali_prompt = 0;
  else vga_write("usage: prompt kali|simple\n");
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
  vga_write_hex(size);
  vga_write(") = ");
  vga_write_hex((u32)(usize)last_alloc);
  vga_write("\n");
}

static void cmd_free(void) {
  if (!last_alloc) {
    vga_write("nothing to free\n");
    return;
  }
  kfree(last_alloc);
  last_alloc = (void *)0;
  vga_write("freed\n");
}

static void cmd_reboot(void) {
  cpu_cli();
  /* Reset via keyboard controller. */
  outb(0x64, 0xFE);
  for (;;) cpu_halt();
}

static void cmd_halt(void) {
  cpu_cli();
  for (;;) cpu_halt();
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

static void handle_line(char *line) {
  const char *p = skip_ws(line);
  if (!p[0]) return;

  /* Split first token. */
  char cmd[16];
  u32 ci = 0;
  while (p[0] && p[0] != ' ' && p[0] != '\t' && ci + 1 < sizeof(cmd)) {
    cmd[ci++] = p[0];
    p++;
  }
  cmd[ci] = 0;
  p = skip_ws(p);

  if (strcmp(cmd, "help") == 0) {
    cmd_help();
  } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
    vga_clear();
  } else if (strcmp(cmd, "mem") == 0) {
    cmd_mem();
  } else if (strcmp(cmd, "echo") == 0) {
    cmd_echo(p);
  } else if (strcmp(cmd, "app") == 0) {
    cmd_app(p);
  } else if (strcmp(cmd, "git") == 0) {
    cmd_git(p);
  } else if (strcmp(cmd, "py") == 0) {
    cmd_py(p);
  } else if (strcmp(cmd, "version") == 0) {
    cmd_version();
  } else if (strcmp(cmd, "about") == 0) {
    cmd_about();
  } else if (strcmp(cmd, "banner") == 0) {
    cmd_banner();
  } else if (strcmp(cmd, "color") == 0) {
    cmd_color(p);
  } else if (strcmp(cmd, "status") == 0) {
    cmd_status(p);
  } else if (strcmp(cmd, "prompt") == 0) {
    cmd_prompt(p);
  } else if (strcmp(cmd, "alloc") == 0) {
    cmd_alloc(p);
  } else if (strcmp(cmd, "free") == 0) {
    cmd_free();
  } else if (strcmp(cmd, "reboot") == 0) {
    cmd_reboot();
  } else if (strcmp(cmd, "halt") == 0) {
    cmd_halt();
  } else {
    log_error("Unknown command");
  }

  status_update();
}

void shell_run(void) {
  log_info("Interactive shell ready");

  char line[128];
  for (;;) {
    print_prompt();

    u32 len = 0;
    for (;;) {
      u16 key = keyboard_getkey();
      char c = (key < 0x100) ? (char)(u8)key : 0;

      if (key == KEY_UP) {
        if (history_count == 0) continue;
        if (history_pos > 0) history_pos--;
        const char *h = hist_get(history_pos);
        if (!h) continue;
        while (len > 0) { len--; vga_putc('\b'); }
        for (u32 i = 0; h[i] && len + 1 < sizeof(line); i++) {
          line[len++] = h[i];
          vga_putc(h[i]);
        }
        continue;
      }

      if (key == KEY_DOWN) {
        if (history_count == 0) continue;
        if (history_pos < history_count) history_pos++;
        while (len > 0) { len--; vga_putc('\b'); }
        if (history_pos == history_count) {
          continue;
        }
        const char *h = hist_get(history_pos);
        if (!h) continue;
        for (u32 i = 0; h[i] && len + 1 < sizeof(line); i++) {
          line[len++] = h[i];
          vga_putc(h[i]);
        }
        continue;
      }

      if (c == '\n') {
        vga_write("\n");
        line[len] = 0;
        break;
      }

      if (c == '\b') {
        if (len > 0) {
          len--;
          vga_putc('\b');
        }
        continue;
      }

      if (c == '\t') {
        /* Treat tab as space in the command buffer. */
        c = ' ';
      }

      if (c >= 32 && c <= 126) {
        if (len + 1 < sizeof(line)) {
          line[len++] = c;
          vga_putc(c);
        }
      }
    }

    hist_push(line);
    handle_line(line);
  }
}
