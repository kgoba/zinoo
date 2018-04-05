#pragma once
#include <stdint.h>

typedef uint32_t systime_t;

void systick_setup(void);

void delay(uint32_t delay);
void delay_us(uint32_t delay);

systime_t millis(void);
