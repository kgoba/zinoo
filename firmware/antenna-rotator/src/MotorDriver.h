#pragma once

#include <stdint.h>

class MotorDriver {
public:
    MotorDriver(int pin_en, int pin_dir, int pin_pwm);

    void begin();

    void enable();
    void disable();
    void setDuty(int8_t percent);

private:
    const int pin_en, pin_dir, pin_pwm;
};


