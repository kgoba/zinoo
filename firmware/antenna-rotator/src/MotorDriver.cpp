#include "MotorDriver.h"

#include <Arduino.h>

MotorDriver::MotorDriver(int pin_en, int pin_dir, int pin_pwm)
    : pin_en(pin_en), pin_dir(pin_dir), pin_pwm(pin_pwm)
{}

void MotorDriver::begin() {
    digitalWrite(pin_en, LOW);
    pinMode(pin_en, OUTPUT);
    pinMode(pin_dir, OUTPUT);
    pinMode(pin_pwm, OUTPUT);

    if (pin_pwm == 9 || pin_pwm == 10) {
        // Mode 8 (frequency and phase correct PWM)
        // Prescaler 1
        TCCR1A &= 0xF0;
        TCCR1B = (1 << CS10) | (1 << WGM13);
        ICR1   = 400;   // 20/2 = 10 kHz

        // Check for OC1A or OC1B
        if (pin_pwm == 9) { 
            OCR1A  = 0;   // 0% duty
            TCCR1A |= (1 << COM1A1);
        }
        else {
            OCR1B  = 0;   // 0% duty
            TCCR1A |= (1 << COM1B1);
        }
    }
}

void MotorDriver::setDuty(int8_t percent) {
    uint16_t ocr;

    if (percent < 0) {
        ocr = -(int16_t)percent * 4;
    }
    else {
        ocr = 400 - (int16_t)percent * 4;
    }

    digitalWrite(pin_dir, (percent >= 0) ? HIGH : LOW);

    if (pin_pwm == 9) {
        OCR1A = ocr;
    }
    else if (pin_pwm == 10) {
        OCR1B = ocr;
    }
}

void MotorDriver::enable() {
    digitalWrite(pin_en, HIGH);
}
void MotorDriver::disable() {
    digitalWrite(pin_en, LOW);
}
