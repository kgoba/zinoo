#pragma once

#include <stdint.h>

extern volatile uint16_t gTonePeriod;
extern volatile uint16_t gToneCount;
extern volatile uint16_t gToneCompensation;
extern volatile uint16_t gTonePhase;
extern volatile int32_t  gToneDelta;

void tone_setup();
