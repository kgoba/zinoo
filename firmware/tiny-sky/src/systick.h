#pragma once
#include <stdint.h>

void systick_setup(void);

void delay(uint32_t delay);
void delay_us(uint32_t delay);

uint32_t millis(void);
