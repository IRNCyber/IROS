#pragma once

/* Initializes and draws the bottom status bar. */
void status_init(void);

/* Updates the status bar (call periodically or after actions). */
void status_update(void);

void status_set_enabled(int enabled);
int status_is_enabled(void);
