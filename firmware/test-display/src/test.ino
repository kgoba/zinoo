#include <Wire.h>
#include <LiquidCrystal.h>

#include <Queue.h>

const int BTN_SEL_PINA = 0;
const int LCD_RS = 8, LCD_EN = 9, LCD_D4 = 4, LCD_D5 = 5, LCD_D6 = 6, LCD_D7 = 7, LCD_BKLT = 10;
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void i2c_recv_event(int howMany);
volatile uint8_t registers[16];

void setup() {
    Wire.begin(38);                // join i2c bus with address #38
    Wire.onReceive(i2c_recv_event);  // register event

    pinMode(LCD_BKLT, OUTPUT);
    analogWrite(LCD_BKLT, 50);

    lcd.begin(16, 2); //initialize the lcd
    lcd.print("RESET");
}

class PktStatus {
public:
    bool    received;
    int8_t  rssi;
};

Queue<PktStatus, 10, uint8_t> recent;
uint8_t recent_ok;
int16_t recent_rssi_sum;

//void add_recent(const PacketStatus &status);

void loop() {
    delay(1000);

    //PacketStatus status;
    //add_recent(status);
}

void add_recent(const class PktStatus & status) {
    recent.push(status);
    if (status.received) recent_ok++;
    recent_rssi_sum += status.rssi;
}

void i2c_recv_event(int howMany) {
    if (howMany < 1) return;

    uint8_t addr = Wire.read();
    while (Wire.available()) {
        uint8_t data = Wire.read();
        if (addr < 16) registers[addr] = data;
        addr++;
    }
}