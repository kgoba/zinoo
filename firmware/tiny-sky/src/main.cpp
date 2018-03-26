#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include <cstring>
#include <cstdio>
#include <cstdarg>

#include "systick.h"
#include "strconv.h"
#include "gps.h"

#include <ptlib/ptlib.h>
#include <ptlib/queue.h>

extern "C" {
#include "cdcacm.h"
}

I2C<I2C1, PB_9, PB_8> bus_i2c;

DigitalOut<PB_12> statusLED;

SPI<SPI1, PB_5, PB_4, PB_3> bus_spi;
DigitalOut<PA_9>  loraRST;
DigitalOut<PA_10> loraSS;

USART<USART1, PB_6, PB_7> usart_gps;

Queue<char, 200> nmeaRXBuffer;
GPSParserSimple gps;

void rcc_setup_hsi16_out_16mhz() {
    rcc_osc_on(RCC_HSI16);
    rcc_wait_for_osc_ready(RCC_HSI16);
    rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
    rcc_set_ppre1(RCC_CFGR_PPRE1_NODIV);
    rcc_set_ppre2(RCC_CFGR_PPRE2_NODIV);
    rcc_set_sysclk_source(RCC_HSI16);

    rcc_ahb_frequency = 16 * 1000000UL;
    rcc_apb1_frequency = 16 * 1000000UL;
    rcc_apb2_frequency = 16 * 1000000UL;
}

void setup() {
    // default clock after reset = MSI 2097 kHz
    rcc_setup_hsi16_out_16mhz();

    rcc_periph_clock_enable(RCC_SYSCFG);
    rcc_periph_clock_enable(RCC_PWR);

    systick_setup();    // enable millisecond counting
    delay(1);           // seems to be necessary for the following delays to work properly

    usb_setup();
    usb_enable_interrupts();

    usart_gps.begin();
    usart_gps.enableIRQ();
    usart_gps.enableRXInterrupt();

    bus_i2c.begin();
    bus_spi.begin();
    bus_spi.format(8, 3);
    bus_spi.frequency(4000000);

    statusLED.begin();

    loraRST.begin();
    loraSS.begin();
    loraSS = 1;
    delay(1);
    loraRST = 1;
    delay(100);

    //gpio_set(GPIOB, GPIO12 | GPIO15);
    //delay(250);
    //gpio_clear(GPIOB, GPIO12 | GPIO15);
}

void print(char c) {
    usb_cdc_write((const uint8_t *) &c, 1);
    //delay(3);
}

void print(const char *format, ...) {
    char    str[100];
    va_list argptr;

    va_start(argptr, format);
    vsnprintf(str, 100, format, argptr);
    va_end(argptr);

    usb_cdc_write((const uint8_t *) str, strlen(str));
    //delay(3);
}

void query_i2c_devices() {
    // 0x60 - MPL3115A2 pressure sensor
    // 0x6B - LSM6DS33  iNEMO 3-axis gyro/accelerometer (0x6A)
    // 0x0E - MAG3110   3-axis magnetometer
    // 0x19 - H3LIS200  3-axis accelerometer (0x18)
    //uint8_t data[] = { 0x00 };
    print("Scanning I2C bus...\n");
    for (uint8_t address = 0x08; address < 0x7E; address++) {
        if (bus_i2c.check(address) == 0) {
            print("Detected %02Xh\n", address);
        }
        //delay(3);
    }
}

void query_mag() {
    const uint8_t slave = 0x0E;
    uint8_t data[] = { 0x07 };
    bus_i2c.write(slave, data, 1, true);
    bus_i2c.read(slave, data, 1);
    print("Mag. ID: %02Xh\n", data[0]);
}

void query_lora() {
    print("Checking LoRa...\n");
    uint8_t reg;
    uint8_t value;
    char buf[16];

    for (reg = 0; reg < 4; reg++) {
        loraSS = 0;
        bus_spi.write(reg);
        value = bus_spi.write(0);
        //delay(2);
        while (bus_spi.is_busy()) ; 
        loraSS = 1;

        print("  Reg[%02Xh] = %02Xh\n", reg, value);
    }
}

void loop() {
    static uint32_t nextOn;
    static uint32_t nextOff;
    static uint32_t nextReport;

    char ch;
    if (nmeaRXBuffer.pop(ch)) {
        //debug.putc(c);
        if (gps.decode(ch)) {
            //
        }
    }

    if (millis() >= nextReport) {
        nextReport += 5 * 1000ul;

        query_lora();
        query_mag();
        //query_i2c_devices();

        char buf[16];
        print(i32tostr( (millis() + 500)/ 1000, 4, buf, false)); print(' ');
        print("%d/%d ", gps.sentencesOK(), gps.sentencesErr());
        print(i32tostr( gps.tracked().value(), 2, buf, true)); print('/');
        print(i32tostr( gps.inView().value(), 2, buf, true)); print(' ');

        if (gps.fixType().atLeast2D()) {
            if (gps.fixTime().valid()) {
                print(u32tostr( gps.fixTime().hours(), 2, buf, true)); print(':'); 
                print(u32tostr( gps.fixTime().minutes(), 2, buf, true)); print(':');
                print(u32tostr( gps.fixTime().seconds(), 2, buf, true)); print(' ');
            }
            if (gps.latitude().valid()) {
                print(ftostr( gps.latitude().degreesFloat(), 7, 4, buf, true)); 
                print(gps.latitude().cardinal()); print(" ");
            }
            if (gps.longitude().valid()) {
                print(ftostr(gps.longitude().degreesFloat(), 8, 4, buf, true));
                print(gps.longitude().cardinal()); print(" ");
            }
            if (gps.fixType().is3D()) {
                if (gps.altitude().valid()) {
                    print(ftostr( gps.altitude().meters(), 5, 0, buf, true)); print("m ");
                }
            }
            else {
                print("2D fix");
            }
            print("\r\n");
        }
        else {
            if (gps.fixTime().valid()) {
                print(u32tostr( gps.fixTime().hours(), 2, buf, true)); print(':'); 
                print(u32tostr( gps.fixTime().minutes(), 2, buf, true)); print(':');
                print(u32tostr( gps.fixTime().seconds(), 2, buf, true));
                print(' ');
            }
            print("NO fix\n");
        }
    }

    if (millis() >= nextOn) {
        statusLED = 1;

        nextOff = nextOn + 100;

        if (gps.fixType().atLeast2D()) {
            if (gps.fixType().is3D()) {
                nextOn += 1000;
            }
            else {
                nextOn += 500;
            }
        }
        else {
            nextOn += 200;
        }
    }

    if (millis() >= nextOff) {
        statusLED = 0;
    }  
}

int main() {
    setup();

    while (1) {
        loop();
    }
}

extern "C" {
    void usart1_isr() {
        constexpr uint32_t periph = USART1;
        if (usart_get_flag(periph, USART_ISR_RXNE)) {
            char ch = usart_recv(periph);
            nmeaRXBuffer.push(ch);
        }
        if (usart_get_flag(periph, USART_ISR_TXE)) {
            usart_disable_tx_interrupt(periph);
        }
    }
}