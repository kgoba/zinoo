#pragma once

#include <stdint.h>

void usb_setup();
void usb_poll();
void usb_enable_interrupts();
int usb_cdc_write(const uint8_t *buf, int len);
