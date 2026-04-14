#pragma once

#include <iros/types.h>

void pic_remap(u8 offset1, u8 offset2);
void pic_send_eoi(u8 irq);
void pic_set_mask(u8 irq, int masked);

