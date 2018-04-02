#include <cstring>
#include <cstdio>
#include <cstdarg>

#include "systick.h"
#include "strconv.h"
#include "gps.h"
#include "ublox.h"
#include "serial.h"

#include "mag3110.h"
#include "mpl3115.h"

#include <ptlib/ptlib.h>
#include <ptlib/queue.h>

extern "C" {
#include "cdcacm.h"
}

//SerialGPS<USART<USART1, PB_6, PB_7>, 200, 64> usart_gps;
SerialGPS   usart_gps;

I2C<I2C1, PB_9, PB_8> bus_i2c;

MAG3110     mag(bus_i2c);
MPL3115     baro(bus_i2c);

DigitalOut<PB_12> statusLED;
DigitalOut<PB_15> buzzerOn;

SPI<SPI1, PB_5, PB_4, PB_3> bus_spi;
DigitalOut<PA_9>  loraRST;
DigitalOut<PA_10> loraSS;

GPSParserSimple gps;

void print(char c) {
    usb_cdc_write((const uint8_t *) &c, 1);
}

void print(const char *format, ...) {
    char    str[100];
    va_list argptr;

    va_start(argptr, format);
    vsnprintf(str, 100, format, argptr);
    va_end(argptr);

    usb_cdc_write((const uint8_t *) str, strlen(str));
}

void gps_send(uint8_t c) {
    // if (nmeaTXBuffer.push(c)) {
    //     usart_gps.enableTXInterrupt();
    // }
    usart_gps.putc(c);
}

void gps_send(const uint8_t *buf, int length) {
    while (length) {
        gps_send(*buf);
        buf++;
        length--;
    }
}

void gps_send_nmea(const char *msg) {
    uint8_t checksum = 0;
    for (const char *ptr = msg; *ptr; ptr++) {
        checksum ^= (uint8_t)(*ptr);
    }
    char nib1 = (checksum >> 4) & 0x0F;
    char nib2 = (checksum >> 0) & 0x0F;
    nib1 += (nib1 < 10) ? '0' : ('A' - 10);
    nib2 += (nib2 < 10) ? '0' : ('A' - 10);

    gps_send('$');
    gps_send((const uint8_t *)msg, strlen(msg));
    gps_send('*');
    gps_send(nib1);
    gps_send(nib2);
    gps_send('\r');
    gps_send('\n');
}

void rcc_setup_hsi16_out_16mhz() {
    rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
    rcc_set_ppre1(RCC_CFGR_PPRE1_NODIV);
    rcc_set_ppre2(RCC_CFGR_PPRE2_NODIV);

    rcc_osc_on(RCC_HSI16);
    rcc_wait_for_osc_ready(RCC_HSI16);
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
    //delay(1);           // seems to be necessary for the following delays to work properly

    usb_setup();
    usb_enable_interrupts();

    usart_gps.begin();

    bus_i2c.begin();
    //bus_i2c.enableIRQ();
    //bus_i2c.enableSTOPInterrupt();
    //bus_i2c.enableNACKInterrupt();

    bus_spi.begin();
    bus_spi.format(8, 3);
    bus_spi.frequency(4000000);

    statusLED.begin();
    buzzerOn.begin();

    loraRST.begin();
    loraSS.begin();
    loraSS = 1;
    delay(1);
    loraRST = 1;
    delay(100);

    statusLED = buzzerOn = 1;
    delay(250);
    statusLED = buzzerOn = 0;

    // Configure PPS to flash at 5Hz w/o lock, 1Hz w/ lock
    ublox_cfg_tp5(5, 1, 1 << 31, 1 << 29, true, false);
}

void query_i2c_devices() {
    // 0x0E - MAG3110   3-axis magnetometer
    // 0x19 - H3LIS200  3-axis accelerometer (0x18)
    // 0x60 - MPL3115A2 pressure sensor
    // 0x6B - LSM6DS33  iNEMO 3-axis gyro/accelerometer (0x6A)
    print("Scanning I2C bus...\n");
    uint8_t data[] = {0};
    for (uint8_t address = 0x60; address < 0x70; address++) {
        if (0 == bus_i2c.check(address)) {
        //if (0 == bus_i2c.write2(address, data, 1)) {
            print("* detected %02Xh\n", address);
        }
    }
}

void query_mag() {
    static bool initialized;

    if (!initialized) {
        if (mag.initialize()) {
            print("MAG3110 initialized\n");
            // Approx 25 ms sampling time
            mag.reset(MAG3110::eRATE_1280, MAG3110::eOSR_32);       
            initialized = true;
        }
        else {
            print("MAG3110 not initialized\n");
        }
    }
    else {
        uint32_t start = millis();
        mag.trigger();
        while (millis() - start < 100) {
            if (mag.dataReady()) {
                uint32_t time = millis() - start;
                char buf[40];
                int x, y, z;
                mag.readMag(x, y, z);
                int t = mag.readTemperature();
                sprintf(buf, "Read [%d %d %d], %dC (%ld ms)\n", x, y, z, t, time);
                print(buf);
                break;
            }
        }
    }
}

void query_baro() {
    static bool initialized;

    if (!initialized) {
        if (baro.initialize()) {
            print("MPL3115 initialized\n");
            // Approx 50 ms sampling time
            baro.reset(MPL3115::eOSR_16);
            initialized = true;
        } else {
            print("MPL3115 not initialized\n");
        }
    }
    else {
        uint32_t start = millis();
        baro.trigger();
        while (true) {
            if (millis() - start >= 100) {
                print("Timeout\n");
                break;
            }
            if (baro.dataReady()) {
                uint32_t time = millis() - start;
                char buf[40];
                uint32_t p;
                int16_t  t;
                baro.readPressure_28q4(p);
                baro.readTemperature_12q4(t);
                sprintf(buf, "Read %dPa %d.%1dC (%ld ms)\n", (p + 8) / 16, t / 16, ((t % 16) * 10 + 8) / 16, time);
                print(buf);
                break;
            }
        }
    }
}

void query_lora() {
    print("Checking LoRa...\n");
    uint8_t reg;
    uint8_t value;
    char buf[16];

    for (reg = 0; reg < 0x40; reg++) {
        loraSS = 0;
        bus_spi.write(reg);
        value = bus_spi.write(0);
        while (bus_spi.is_busy()) ; 
        loraSS = 1;
        print("  Reg[%02Xh] = %02Xh\n", reg, value);
    }
}

void print_report() {
    //query_lora();
    query_mag();
    query_baro();
    query_i2c_devices();

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

uint8_t gps_mode;

void console_parse(const char *line) {
    bool cmd_ok = false;
    if (0 == strcmp(line, "gps_off")) {
        ublox_powermode(ePM_BACKUP);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_on")) {
        ublox_powermode(ePM_POWERON);
        delay(100);
        gps_mode = 0;
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_rst")) {
        ublox_reset();
        gps_mode = 0;
        cmd_ok = true;
    }
    // else if (0 == strcmp(line, "nmea")) {
    //     gps_send_nmea("PUBX,41,1,0003,0003,9600,0");
    //     cmd_ok = true;
    // }
    else if (0 == strcmp(line, "gps_raw")) {
        gps_send_nmea("PUBX,41,1,0001,0001,9600,0");
        gps_mode = 1;
        delay(100);
        ublox_enable_trkd5();
        delay(10);
        ublox_enable_sfrbx();
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_pps")) {
        ublox_cfg_tp5(5, 1000, 1 << 31, 1 << 31, true, false);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_air")) {
        ublox_set_dyn_mode(eDYN_AIRBORNE_1G);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_stat")) {
        ublox_set_dyn_mode(eDYN_STATIONARY);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_ped")) {
        ublox_set_dyn_mode(eDYN_PEDESTRIAN);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_port")) {
        ublox_set_dyn_mode(eDYN_PORTABLE);
        cmd_ok = true;
    }
    if (cmd_ok) print(line);
    else print("?");
}

void task_gps_decode() {
    uint8_t ch;
    if (usart_gps.getc(ch)) {
        if (gps_mode == 1) {
            print("%02X ", ch);
        }
        else if (gps.decode(ch)) {
            //
        }
    }

    static bool first_fix;
    if (!first_fix && gps.fixType().is3D()) {
        first_fix = true;
        print("Time to first 3D fix: %ld seconds\n", millis() / 1000);
        for (int i = 0; i < 3; i++) {
            buzzerOn = 1;
            delay(100);
            buzzerOn = 0;
            delay(100);
        }
    }
}

void task_console() {
    static char rx_line[80];
    static int  rx_len;
    uint8_t ch;

    if (usb_cdc_read((uint8_t *)&ch, 1)) {
        print(ch);
        if (rx_len < 79 && ch != 0x0A && ch != 0x0D) {
            rx_line[rx_len] = ch;
            rx_len++;
        }
        if (ch == 0x0A) {
            rx_line[rx_len] = '\0';
            if (rx_len > 0) {
                console_parse(rx_line);
                print("\r\n");
            }
            else {
                print_report();
            }
            print("> ");
            rx_len = 0;
        }
    }
}

void task_report() {
    // static uint32_t nextReport;

    // if (millis() >= nextReport) {
    //     nextReport += 5 * 1000ul;
    //     print_report();
    // }
}

void task_led_blink() {
    static uint32_t nextOn;
    static uint32_t nextOff;

    if (millis() >= nextOn) {
        statusLED = 1;
        nextOff = nextOn + 100;
        if (gps.fixType().atLeast2D()) {
            nextOn += (gps.fixType().is3D()) ? 1000 : 500;
        } else {
            nextOn += 200;
        }
    }
    if (millis() >= nextOff) {
        statusLED = 0;
    }    
}

void loop() {
    task_gps_decode();
    task_console();
    task_report();
    task_led_blink();
}

int main() {
    setup();

    while (1) {
        loop();
    }
}

extern "C" {
    // void i2c1_isr(void) {
    //     constexpr uint32_t periph = I2C1;
    //     uint32_t flags = I2C_ISR(periph);
    //     if (flags & I2C_ISR_RXNE) {
    //         // read IC2_RXDR to clear the flag
    //         uint8_t data = I2C_RXDR(periph);
    //     }
    //     if (flags & I2C_ISR_TXIS) {
    //         // write I2C_TXDR to clear the flag
    //         uint8_t data;
    //         I2C_TXDR(periph) = data;
    //     }
    //     if (flags & I2C_ISR_TC) {
    //         // write START or STOP
    //     }
    //     if (flags & I2C_ISR_STOPF) {
    //         I2C_ICR(periph) = I2C_ICR_STOPCF;
    //     }
    //     if (flags & I2C_ISR_NACKF) {
    //         I2C_ICR(periph) = I2C_ICR_NACKCF;
    //     }
    // }
    // void tim2_isr(void);
    // void tim21_isr(void);
    // void tim22_isr(void);    
    // void tim6_dac_isr(void);    // BASIC timer
    // void lptim1_isr(void);      // Low Power timer
}