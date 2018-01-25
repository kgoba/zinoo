/*
*/

#include <LiquidCrystal.h>
#include "MotorDriver.h"

// LCD pin connections
const int PIN_RS = 2, PIN_EN = 3, PIN_D4 = 4, PIN_D5 = 5, PIN_D6 = 6, PIN_D7 = 7;

// Motor driver pin connections
// PWM on pins 9 (OC1A) and 10 (OC1B)
const int PIN_MOT_EN = 8, PIN_MOT_1A = 9, PIN_MOT_2A = 11, PIN_MOT_3A = 10, PIN_MOT_4A = 12;

LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
MotorDriver   motor1(PIN_MOT_EN, PIN_MOT_2A, PIN_MOT_1A);
MotorDriver   motor2(PIN_MOT_EN, PIN_MOT_4A, PIN_MOT_3A);

struct DisplayInfo {
    int8_t duty1;
    int8_t duty2;
};

DisplayInfo gInfo;

void setup() {
    Serial.begin(9600);

    lcd.begin(16, 2);
    lcd.print("Rotator v1.0");

    motor1.begin();
    motor2.begin();

    // TEST
    set_duty1(-040);
    set_duty2(+80);
}

int get_word(const char *line) {
    if (line == NULL) return 0;

    char * result = strchr(line, ' ');

    if (result == NULL)
        return strlen(line);
    else
        return result - line;
}

bool parse(const char *line) {
    if (strlen(line) < 2) return false;

    if (line[1] != ' ') return false;
    char cmd = line[0];

    if (cmd == 'A' || cmd == 'B') {
        int duty;
        sscanf(line + 2, "%d", &duty);
        if (duty > 100) duty = 100;
        if (duty < -100) duty = -100;
        if (cmd == 'A') {
            set_duty1(duty);
        }
        else {
            set_duty2(duty);
        }
        return true;
    }
}

void parse(char c) {
    static char line[40];
    static uint8_t len;

    if (c == 0x0A || c == 0x0D) {
        if (len > 0) {
            line[len] = '\0';
            if (parse(line)) 
                Serial.println("OK");            
            else
                Serial.println("ERROR");
            len = 0;            
        }
    }
    else if (len < 40) {
        line[len++] = c;
    }
}

void loop() {
    delay(100);
    update_display();
    while (Serial.available()) {
        char c = Serial.read();
        Serial.write(c);
        parse(c);
    }
}

void set_duty1(int8_t percent) {
    motor1.enable();
    motor1.setDuty(percent);
    gInfo.duty1 = percent;
    Serial.print("M_A: ");
    Serial.print("Setting duty to "); Serial.print(percent); Serial.println('%');
}

void set_duty2(int8_t percent) {
    motor2.enable();
    motor2.setDuty(percent);
    gInfo.duty2 = percent;
    Serial.print("M_B: ");
    Serial.print("Setting duty to "); Serial.print(percent); Serial.println('%');
}

void update_display() {
    lcd.setCursor(6, 1);
    lcd.print(gInfo.duty1);
    lcd.print('%');

    lcd.setCursor(11, 1);
    lcd.print(gInfo.duty2);    
    lcd.print('%');

    lcd.setCursor(0, 1);
    lcd.print(millis() / 1000);
}
