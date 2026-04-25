#pragma once

void log_init(void);
void log_info(const char *msg);
void log_error(const char *msg);

/* Log buffer + serial mirroring */
void log_set_serial_enabled(int enabled);
int log_serial_enabled(void);
void log_dmesg_dump(void);
