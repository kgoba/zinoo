#pragma once

#include <stdint.h>

class MotorDriver {
public:
    MotorDriver(int pin_en, int pin_dir, int pin_pwm);

    void begin();

    void enable();
    void disable();

    /// Duty cycle in the range from -100 to 100, corresponding to percent 
    // of full PWM. 0 means active braking, if motor is enabled.
    void setDuty(int8_t percent);

private:
    const int pin_en, pin_dir, pin_pwm;
};


