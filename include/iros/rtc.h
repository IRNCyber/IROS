#pragma once

#include <iros/types.h>

typedef struct {
  u8 second;
  u8 minute;
  u8 hour;
  u8 day;
  u8 month;
  u16 year;
} rtc_time_t;

void rtc_read(rtc_time_t *t);

/* Format time as "HH:MM:SS" into buf (needs at least 9 bytes). */
void rtc_format_time(const rtc_time_t *t, char *buf);
