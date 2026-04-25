#include <iros/ports.h>
#include <iros/vga.h>

enum { VGA_WIDTH = 80, VGA_HEIGHT = 25 };
enum { VIEW_ROWS = 24 };                 /* row 24 reserved for status bar */
enum { TEXT_WIDTH = 79, SCROLLBAR_COL = 79 };
enum { SCROLLBACK_LINES = 256 };

static volatile u16 *const VGA_MEMORY = (u16 *)0xB8000;

static u8 text_attr = 0x0F; /* white on black */

/* Scrollback ring buffer of lines. Stores only TEXT_WIDTH columns. */
static u16 lines[SCROLLBACK_LINES][TEXT_WIDTH];
static u32 line_total = 0; /* total lines ever produced */
static u32 cur_col = 0;

/* View offset from bottom: 0 = follow, >0 = scrolled back. */
static u32 view_offset = 0;

static int pointer_enabled = 0;
static int pointer_drawn = 0;
static u8 pointer_row = 0;
static u8 pointer_col = 0;
static u16 pointer_saved = 0;
static int scrollbar_dragging = 0;

static char logo_cell(u32 row, u32 col) {
  static const char *logo[] = {
      "  ___ ___  ___  ___ ",
      " |_ _| _ \\/ _ \\/ __|",
      "  | ||   / (_) \\__ \\",
      " |___|_|_\\\\___/|___/",
      "      I R O S      ",
  };
  enum { LOGO_ROWS = 5, LOGO_COLS = 20 };
  const u32 start_row = 9;
  const u32 start_col = 28;

  if (row < start_row || row >= start_row + LOGO_ROWS) return 0;
  if (col < start_col || col >= start_col + LOGO_COLS) return 0;
  return logo[row - start_row][col - start_col];
}

static inline u16 vga_entry(char c, u8 attr) {
  return (u16)(u8)c | ((u16)attr << 8);
}

static void vga_hw_set_cursor(u16 pos) {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (u8)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

static void clear_line_buf(u16 *dst) {
  for (u32 c = 0; c < TEXT_WIDTH; c++) dst[c] = vga_entry(' ', text_attr);
}

static u32 ring_index(u32 absolute_line) {
  return absolute_line % SCROLLBACK_LINES;
}

static u32 max_offset(void) {
  u32 total = (line_total < SCROLLBACK_LINES) ? line_total : SCROLLBACK_LINES;
  if (total <= VIEW_ROWS) return 0;
  return total - VIEW_ROWS;
}

static void pointer_erase(void) {
  if (!pointer_drawn) return;
  if (pointer_row < VIEW_ROWS && pointer_col < TEXT_WIDTH) {
    VGA_MEMORY[(u32)pointer_row * VGA_WIDTH + (u32)pointer_col] = pointer_saved;
  }
  pointer_drawn = 0;
}

static void pointer_draw(void) {
  if (!pointer_enabled) return;
  if (pointer_row >= VIEW_ROWS || pointer_col >= TEXT_WIDTH) return;

  u32 off = (u32)pointer_row * VGA_WIDTH + (u32)pointer_col;
  pointer_saved = VGA_MEMORY[off];

  u8 ch = (u8)(pointer_saved & 0xFFu);
  u8 attr = (u8)((pointer_saved >> 8) & 0xFFu);
  u8 fg = (u8)(attr & 0x0Fu);
  u8 bg = (u8)((attr >> 4) & 0x0Fu);
  u8 inv = (u8)((fg << 4) | bg);
  VGA_MEMORY[off] = (u16)ch | ((u16)inv << 8);
  pointer_drawn = 1;
}

static void pointer_refresh(void) {
  pointer_erase();
  pointer_draw();
}

static void render_scrollbar(void) {
  u32 total = (line_total < SCROLLBACK_LINES) ? line_total : SCROLLBACK_LINES;
  if (total < VIEW_ROWS) total = VIEW_ROWS;

  u32 maxOff = max_offset();
  u32 thumb = (VIEW_ROWS * VIEW_ROWS) / total;
  if (thumb < 1) thumb = 1;
  if (thumb > VIEW_ROWS) thumb = VIEW_ROWS;

  u32 start = 0;
  if (maxOff != 0) {
    /* offset 0 => bottom thumb; offset max => top thumb */
    u32 travel = VIEW_ROWS - thumb;
    start = (travel * (maxOff - view_offset)) / maxOff;
  }

  u16 track = vga_entry((char)0xB0, (u8)((0 << 4) | 8)); /* light gray */
  u16 block = vga_entry((char)0xDB, (u8)((0 << 4) | 7)); /* white */

  for (u32 r = 0; r < VIEW_ROWS; r++) {
    VGA_MEMORY[r * VGA_WIDTH + SCROLLBAR_COL] = track;
  }
  for (u32 r = 0; r < thumb; r++) {
    VGA_MEMORY[(start + r) * VGA_WIDTH + SCROLLBAR_COL] = block;
  }
}

static void render_view(void) {
  /* Determine the first absolute line to show. */
  u32 total = (line_total < SCROLLBACK_LINES) ? line_total : SCROLLBACK_LINES;
  u32 base_abs = (line_total > total) ? (line_total - total) : 0;

  u32 maxOff = max_offset();
  if (view_offset > maxOff) view_offset = maxOff;

  u32 bottom_abs = (line_total == 0) ? 0 : (line_total - 1);
  u32 view_bottom_abs = (view_offset <= bottom_abs) ? (bottom_abs - view_offset) : base_abs;
  u32 view_start_abs = (view_bottom_abs + 1 >= VIEW_ROWS) ? (view_bottom_abs + 1 - VIEW_ROWS) : base_abs;
  if (view_start_abs < base_abs) view_start_abs = base_abs;

  for (u32 r = 0; r < VIEW_ROWS; r++) {
    u32 abs = view_start_abs + r;
    u32 idx = ring_index(abs);
    for (u32 c = 0; c < TEXT_WIDTH; c++) {
      u16 cell = lines[idx][c];
      u8 ch = (u8)(cell & 0xFFu);
      if (ch == ' ') {
        char bg = logo_cell(r, c);
        if (bg) {
          /* Dim cyan logo as background watermark. */
          cell = vga_entry(bg, (u8)((0 << 4) | 3));
        }
      }
      VGA_MEMORY[r * VGA_WIDTH + c] = cell;
    }
  }

  render_scrollbar();
  pointer_draw();
}

static void follow_bottom(void) {
  view_offset = 0;
}

static void ensure_current_line_exists(void) {
  if (line_total == 0) {
    clear_line_buf(lines[0]);
    line_total = 1;
    cur_col = 0;
  }
}

void vga_init(void) {
  vga_set_color(15, 0);
  vga_clear();
}

void vga_set_color(u8 fg, u8 bg) {
  text_attr = (u8)((bg << 4) | (fg & 0x0F));
}

void vga_get_color(u8 *fg, u8 *bg) {
  if (fg) *fg = (u8)(text_attr & 0x0F);
  if (bg) *bg = (u8)((text_attr >> 4) & 0x0F);
}

void vga_set_cursor(vga_cursor_t c) {
  /* Cursor is for the live bottom view only; clamp to visible area excluding scrollbar. */
  if (c.row >= VIEW_ROWS) c.row = VIEW_ROWS - 1;
  if (c.col >= TEXT_WIDTH) c.col = TEXT_WIDTH - 1;
  vga_hw_set_cursor((u16)c.row * VGA_WIDTH + (u16)c.col);
}

vga_cursor_t vga_get_cursor(void) {
  /* Best effort: return current write cursor (bottom line). */
  vga_cursor_t c;
  u32 row = 0;
  if (line_total > 0) {
    u32 total = (line_total < SCROLLBACK_LINES) ? line_total : SCROLLBACK_LINES;
    row = total - 1;
    if (row >= VIEW_ROWS) row = VIEW_ROWS - 1;
  }
  c.row = (u8)row;
  c.col = (u8)((cur_col < TEXT_WIDTH) ? cur_col : (TEXT_WIDTH - 1));
  return c;
}

void vga_clear(void) {
  pointer_drawn = 0;

  /* Clear scrollback */
  for (u32 i = 0; i < SCROLLBACK_LINES; i++) {
    clear_line_buf(lines[i]);
  }
  line_total = 0;
  cur_col = 0;
  view_offset = 0;

  /* Clear visible text + status row */
  for (u32 r = 0; r < VGA_HEIGHT; r++) {
    for (u32 c = 0; c < VGA_WIDTH; c++) {
      VGA_MEMORY[r * VGA_WIDTH + c] = vga_entry(' ', text_attr);
    }
  }
  vga_hw_set_cursor(0);
  pointer_draw();
}

static void new_line(void) {
  ensure_current_line_exists();

  /* Pad rest of current line with spaces. */
  u32 idx = ring_index(line_total - 1);
  for (u32 c = cur_col; c < TEXT_WIDTH; c++) lines[idx][c] = vga_entry(' ', text_attr);

  /* Advance */
  u32 next_abs = line_total;
  u32 next_idx = ring_index(next_abs);
  clear_line_buf(lines[next_idx]);
  line_total++;
  cur_col = 0;
}

void vga_putc(char ch) {
  follow_bottom();
  ensure_current_line_exists();

  if (ch == '\n') {
    new_line();
    render_view();
    vga_set_cursor(vga_get_cursor());
    return;
  }
  if (ch == '\r') {
    cur_col = 0;
    render_view();
    vga_set_cursor(vga_get_cursor());
    return;
  }
  if (ch == '\b') {
    if (cur_col > 0) {
      cur_col--;
      u32 idx = ring_index(line_total - 1);
      lines[idx][cur_col] = vga_entry(' ', text_attr);
      render_view();
      vga_set_cursor(vga_get_cursor());
    }
    return;
  }

  u32 idx = ring_index(line_total - 1);
  lines[idx][cur_col] = vga_entry(ch, text_attr);
  cur_col++;
  if (cur_col >= TEXT_WIDTH) {
    new_line();
  }
  render_view();
  vga_set_cursor(vga_get_cursor());
}

void vga_write(const char *s) {
  for (u32 i = 0; s && s[i]; i++) vga_putc(s[i]);
}

static int is_digit(char c) { return c >= '0' && c <= '9'; }

static void ansi_apply_sgr(int code) {
  u8 fg, bg;
  vga_get_color(&fg, &bg);

  if (code == 0) {
    fg = 15;
    bg = 0;
  } else if (code == 1) {
    fg = (u8)(fg | 0x08);
  } else if (code >= 30 && code <= 37) {
    fg = (u8)(code - 30);
  } else if (code >= 90 && code <= 97) {
    fg = (u8)((code - 90) | 0x08);
  } else if (code >= 40 && code <= 47) {
    bg = (u8)(code - 40);
  } else if (code >= 100 && code <= 107) {
    bg = (u8)((code - 100) | 0x08);
  }

  vga_set_color(fg, bg);
}

void vga_write_ansi(const char *s) {
  for (u32 i = 0; s && s[i]; ) {
    unsigned char ch = (unsigned char)s[i];
    if (ch != 0x1B) {
      vga_putc((char)ch);
      i++;
      continue;
    }

    if (s[i + 1] != '[') {
      i++;
      continue;
    }

    i += 2;
    int any = 0;
    int code = 0;

    for (;;) {
      char c = s[i];
      if (!c) return;
      if (is_digit(c)) {
        any = 1;
        code = code * 10 + (c - '0');
        i++;
        continue;
      }
      if (c == ';') {
        ansi_apply_sgr(any ? code : 0);
        any = 0;
        code = 0;
        i++;
        continue;
      }
      if (c == 'm') {
        ansi_apply_sgr(any ? code : 0);
        i++;
        break;
      }
      i++;
      break;
    }
  }
}

void vga_write_hex(u32 value) {
  static const char *hex = "0123456789ABCDEF";
  vga_write("0x");
  for (int shift = 28; shift >= 0; shift -= 4) {
    vga_putc(hex[(value >> shift) & 0xF]);
  }
}

void vga_write_at(u8 row, u8 col, const char *s) {
  if (row >= VGA_HEIGHT) return;
  if (col >= VGA_WIDTH) return;
  for (u32 i = 0; s && s[i]; i++) {
    u32 x = (u32)col + i;
    if (x >= VGA_WIDTH) break;
    VGA_MEMORY[(u32)row * VGA_WIDTH + x] = vga_entry(s[i], text_attr);
  }
  pointer_refresh();
}

void vga_fill_row(u8 row, char ch) {
  if (row >= VGA_HEIGHT) return;
  for (u32 col = 0; col < VGA_WIDTH; col++) {
    VGA_MEMORY[(u32)row * VGA_WIDTH + col] = vga_entry(ch, text_attr);
  }
  pointer_refresh();
}

void vga_scroll(int lines_delta) {
  if (lines_delta == 0) return;
  u32 maxOff = max_offset();

  if (lines_delta > 0) {
    u32 d = (u32)lines_delta;
    if (view_offset + d > maxOff) view_offset = maxOff;
    else view_offset += d;
  } else {
    u32 d = (u32)(-lines_delta);
    if (d > view_offset) view_offset = 0;
    else view_offset -= d;
  }

  render_view();
}

void vga_scroll_top(void) {
  view_offset = max_offset();
  render_view();
}

void vga_scroll_bottom(void) {
  view_offset = 0;
  render_view();
}

int vga_is_scrolled(void) {
  return view_offset != 0;
}

void vga_pointer_enable(int enabled) {
  if (enabled) {
    pointer_enabled = 1;
    render_view();
    vga_set_cursor(vga_get_cursor());
    return;
  }

  pointer_enabled = 0;
  pointer_erase();
}

int vga_pointer_is_enabled(void) {
  return pointer_enabled;
}

void vga_pointer_set(u8 row, u8 col) {
  if (row >= VIEW_ROWS) row = VIEW_ROWS - 1;
  if (col >= TEXT_WIDTH) col = TEXT_WIDTH - 1;

  pointer_row = row;
  pointer_col = col;
  pointer_refresh();
}

void vga_pointer_input(u8 row, u8 col, u8 buttons) {
  if (row >= VIEW_ROWS) row = VIEW_ROWS - 1;
  if (col >= TEXT_WIDTH) col = TEXT_WIDTH - 1;

  u8 left_down = (u8)(buttons & 0x01u);
  if (!left_down) {
    scrollbar_dragging = 0;
    return;
  }

  if (col == SCROLLBAR_COL) scrollbar_dragging = 1;
  if (!scrollbar_dragging) return;

  u32 maxOff = max_offset();
  if (maxOff == 0) {
    if (view_offset != 0) {
      view_offset = 0;
      render_view();
      vga_set_cursor(vga_get_cursor());
    }
    return;
  }

  u32 maxRow = VIEW_ROWS - 1u;
  u32 invRow = maxRow - row; /* top => max scrollback */
  u32 target = (invRow * maxOff + (maxRow / 2u)) / maxRow;
  if (target != view_offset) {
    view_offset = target;
    render_view();
    vga_set_cursor(vga_get_cursor());
  }
}
