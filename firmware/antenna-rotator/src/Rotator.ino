/*
    Antenna rotator controller firmware
    (c) Karlis Goba 2018
*/

#include <LiquidCrystal.h>
#include "MotorDriver.h"

// LCD (16x2 alphanumeric) pin connections
const int PIN_RS = 2, PIN_EN = 3, PIN_D4 = 4, PIN_D5 = 5, PIN_D6 = 6, PIN_D7 = 7;

// Motor driver (L293D) pin connections
// PWM on pins 9 (OC1A) and 10 (OC1B)
const int PIN_MOT_EN = 8;
const int PIN_MOT_1A = 9, PIN_MOT_2A = 11;
const int PIN_MOT_3A = 10, PIN_MOT_4A = 12;

const int PINA_PV1 = 1;
const int PINA_PV2 = 0;
const int PINA_SP1 = 3;
const int PINA_SP2 = 2;

LiquidCrystal lcd    (PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
MotorDriver   motor1 (PIN_MOT_EN, PIN_MOT_4A, PIN_MOT_3A);
MotorDriver   motor2 (PIN_MOT_EN, PIN_MOT_2A, PIN_MOT_1A);

struct DisplayInfo {
    int8_t    duty1;
    int8_t    duty2;

    uint16_t  sp1, sp2;
    uint16_t  pv1, pv2;
};

DisplayInfo gInfo;

void setup() {
    Serial.begin(9600);

    lcd.begin(16, 2);
    lcd.print("A-ROT v1.0");

    // Switch to external reference and do a few conversions to ramp up ADC
    analogReference(EXTERNAL);
    for (uint8_t i = 0; i < 8; i++) {
        analogRead(PINA_SP1);
        analogRead(PINA_SP2);
        delay(1);
    }

    motor1.begin();
    motor2.begin();

    // TEST
    set_duty1(-40);
    set_duty2(+80);
}

bool match_token(const char **cursor, const char *needle) {
    if (cursor == NULL) return false;
    if (*cursor == NULL) return false;

    // Compare to needle
    const char * ptr = *cursor;
    while (*needle) {
        if (*ptr != *needle) return false;
        needle++;
        ptr++;
    }

    // Skip whitespace
    while (*ptr == ' ') {
        ptr++;
    }

    // Save result
    *cursor = ptr;
    return true;
}

int get_token(const char **cursor, char *result, int capacity) {
    if (cursor == NULL) return false;
    if (*cursor == NULL) return false;

    int length = 0;
    const char * ptr = *cursor;
    while (*ptr) {
        if (*ptr == ' ') break;
        if (capacity > 1) {
            *result = *ptr;
            capacity--;
            result++;
        }
        length++;
        ptr++;
    }

    // Add zero termination
    if (capacity > 0) {
        *result = '\0';
        capacity--;
    }

    // Skip whitespace
    while (*ptr == ' ') {
        ptr++;
    }

    // Save result
    *cursor = ptr;
    return length;
}

bool parse(const char *line) {
    const char * cursor = line;
    char cmd[10], arg[10];

    get_token(&cursor, cmd, 10);
    get_token(&cursor, arg, 10);

    if (0 == strcmp(cmd, "A")) {
        int arg_int = atoi(arg);
        set_duty1(arg_int);
        return true;
    }
    if (0 == strcmp(cmd, "B")) {
        int arg_int = atoi(arg);
        set_duty2(arg_int);
        return true;
    }
    return false;
}

void parse(char c) {
    static char     line[40];
    static uint8_t  len;

    if (c == 0x0A || c == 0x0D) {
        if (len > 0) {
            line[len] = '\0';
            bool success = parse(line);
            Serial.println(success ? "OK" : "ERROR");
            len = 0;            
        }
    }
    else if (len < 40) {
        line[len++] = c;
    }
}

void loop() {
    while (Serial.available()) {
        char c = Serial.read();
        Serial.write(c);
        parse(c);
    }

    gInfo.sp1 = analogRead(PINA_SP1);
    gInfo.sp2 = analogRead(PINA_SP2);

    gInfo.pv1 = analogRead(PINA_PV1);
    gInfo.pv2 = analogRead(PINA_PV2);

    update_display();
    delay(100);
}

void set_duty1(int percent) {
    if (percent > 100) percent = 100;
    if (percent < -100) percent = -100;
    motor1.enable();
    motor1.setDuty(percent);
    gInfo.duty1 = percent;
    Serial.print("M_A: ");
    Serial.print("Setting duty to "); Serial.print(percent); Serial.println('%');
}

void set_duty2(int percent) {
    if (percent > 100) percent = 100;
    if (percent < -100) percent = -100;
    motor2.enable();
    motor2.setDuty(percent);
    gInfo.duty2 = percent;
    Serial.print("M_B: ");
    Serial.print("Setting duty to "); Serial.print(percent); Serial.println('%');
}

void update_display() {
    lcd.setCursor(11, 0);
    lcd.print(gInfo.sp1);    
    lcd.print("  ");

    lcd.setCursor(6, 1);
    lcd.print(gInfo.duty1);
    lcd.print('%');

    lcd.setCursor(11, 1);
    lcd.print(gInfo.duty2);    
    lcd.print('%');

    lcd.setCursor(0, 1);
    lcd.print(millis() / 1000);
}
