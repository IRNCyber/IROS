#include <iros/ports.h>
#include <iros/rtc.h>

enum { CMOS_ADDR = 0x70, CMOS_DATA = 0x71 };

static u8 cmos_read(u8 reg) {
  outb(CMOS_ADDR, reg);
  return inb(CMOS_DATA);
}

static u8 bcd_to_bin(u8 bcd) {
  return (u8)((bcd >> 4) * 10 + (bcd & 0x0F));
}

void rtc_read(rtc_time_t *t) {
  /* Wait for update-in-progress to clear. */
  while (cmos_read(0x0A) & 0x80) {}

  u8 sec = cmos_read(0x00);
  u8 min = cmos_read(0x02);
  u8 hr  = cmos_read(0x04);
  u8 day = cmos_read(0x07);
  u8 mon = cmos_read(0x08);
  u8 yr  = cmos_read(0x09);

  u8 regB = cmos_read(0x0B);
  int bcd = !(regB & 0x04);
  int is_24h = regB & 0x02;

  /* Save and strip PM flag before any conversion. */
  int pm = hr & 0x80;
  hr = (u8)(hr & 0x7Fu);

  if (bcd) {
    sec = bcd_to_bin(sec);
    min = bcd_to_bin(min);
    hr  = bcd_to_bin(hr);
    day = bcd_to_bin(day);
    mon = bcd_to_bin(mon);
    yr  = bcd_to_bin(yr);
  }

  /* Convert 12-hour to 24-hour if needed. */
  if (!is_24h) {
    if (hr == 12 && !pm) hr = 0;
    else if (hr != 12 && pm) hr = (u8)(hr + 12u);
  }

  t->second = sec;
  t->minute = min;
  t->hour = hr;
  t->day = day;
  t->month = mon;
  t->year = (u16)(2000u + yr);
}

void rtc_format_time(const rtc_time_t *t, char *buf) {
  buf[0] = (char)('0' + t->hour / 10);
  buf[1] = (char)('0' + t->hour % 10);
  buf[2] = ':';
  buf[3] = (char)('0' + t->minute / 10);
  buf[4] = (char)('0' + t->minute % 10);
  buf[5] = ':';
  buf[6] = (char)('0' + t->second / 10);
  buf[7] = (char)('0' + t->second % 10);
  buf[8] = 0;
}
