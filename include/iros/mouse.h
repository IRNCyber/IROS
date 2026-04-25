#pragma once

#include <iros/types.h>

typedef struct {
  u8 row;
  u8 col;
  u8 buttons; /* bit0=left, bit1=right, bit2=middle */
  u8 enabled;
} mouse_state_t;

void mouse_init(void);
void mouse_set_enabled(int enabled);
int mouse_is_enabled(void);
void mouse_set_position(u8 row, u8 col);
mouse_state_t mouse_get_state(void);
void mouse_poll_once(void);
void mouse_feed_byte(u8 data);
void mouse_flush_ui(void);
