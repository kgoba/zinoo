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
#include "lsm6ds33.h"
#include "rfm96.h"

#include "settings.h"

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
LSM6DS33    gyro(bus_i2c);

DigitalOut<PB_12> statusLED;
DigitalOut<PB_15> buzzerOn;

SPI_T<SPI1, PB_5, PB_4, PB_3> bus_spi;
DigitalOut<PA_9>  loraRST;
DigitalOut<PA_10> loraSS;

GPSParserSimple gps;

RFM96 radio;

AppSettings gSettings;
AppState    gState;

extern "C" {
    #include "lfs.h"

    int spi_flash_lfs_read(const struct lfs_config *c, lfs_block_t block, 
        lfs_off_t off, void *buffer, lfs_size_t size);

    int spi_flash_lfs_prog(const struct lfs_config *c, lfs_block_t block, 
        lfs_off_t off, const void *buffer, lfs_size_t size);

    int spi_flash_lfs_erase(const struct lfs_config *c, lfs_block_t block);

    int spi_flash_lfs_sync(const struct lfs_config *c);
}

uint8_t lfs_read_buffer[16];
uint8_t lfs_prog_buffer[16];
uint8_t lfs_lookahead_buffer[16];
uint8_t lfs_file_buffer[16];

// configuration of the filesystem is provided by this struct
const lfs_config cfg = {
    .context = 0,

    // block device operations
    .read  = spi_flash_lfs_read,
    .prog  = spi_flash_lfs_prog,
    .erase = spi_flash_lfs_erase,
    .sync  = spi_flash_lfs_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = 128,
    .lookahead = 128,

    .read_buffer = lfs_read_buffer,
    .prog_buffer = lfs_prog_buffer,
    .lookahead_buffer = lfs_lookahead_buffer,
    .file_buffer = lfs_file_buffer
};

lfs_t lfs;

int spi_flash_lfs_read(const struct lfs_config *c, lfs_block_t block, 
    lfs_off_t off, void *buffer, lfs_size_t size) {
    return -1;
}

int spi_flash_lfs_prog(const struct lfs_config *c, lfs_block_t block, 
    lfs_off_t off, const void *buffer, lfs_size_t size) {
    return -1;
}

int spi_flash_lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    return -1;
}

int spi_flash_lfs_sync(const struct lfs_config *c) {
    return -1;
}

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
    // default clock after reset = MSI 2097 kHz
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
    // Initialize MCU clocks
    rcc_setup_hsi16_out_16mhz();

    rcc_periph_clock_enable(RCC_SYSCFG);
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_MIF);

    systick_setup();    // enable millisecond counting

    // Before initalizing any hardware, restore settings from EEPROM
    gSettings.restore();

    // Initialize internal peripherals
    usb_setup();
    usb_enable_interrupts();

    usart_gps.begin();

    bus_i2c.begin();
    //bus_i2c.enableIRQ();
    //bus_i2c.enableSTOPInterrupt();
    //bus_i2c.enableNACKInterrupt();

    bus_spi.begin();
    bus_spi.format(8, 0);
    bus_spi.frequency(4000000);

    statusLED.begin();
    buzzerOn.begin();
    loraRST.begin(1);
    loraSS.begin(1);

    // Initialize external peripherals
    delay(100);         // seems to be necessary for the following delays to work properly

    // Configure PPS to flash at 5Hz w/o lock, 1Hz w/ lock
    ublox_cfg_tp5(5, 1, 1 << 31, 1 << 29, true, false);

    radio.init();
    radio.setupLoRa(RFM96::ModemSettings()
        .setSF(RFM96::eSF_10)
        .setCR(RFM96::eCR_4_8)
    );
    // radio.setupLoRa(RFM96::ModemSettings()
    //     .setSF(RFM96::eSF_9)
    //     .setBW(RFM96::eBW_31k25)
    //     .setCR(RFM96::eCR_4_8)
    // );
    radio.setFrequencyHz(gSettings.radio_frequency);
    radio.setTXPower(gSettings.radio_tx_power);

    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // lfs_file_t file;
    // // read current count
    // uint32_t boot_count = 0;
    // lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    // lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // // update boot count
    // boot_count += 1;
    // lfs_file_rewind(&lfs, &file);
    // lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // // remember the storage is not updated until the file is closed successfully
    // lfs_file_close(&lfs, &file);

    statusLED = buzzerOn = 1;
    delay(250);
    statusLED = buzzerOn = 0;
}

void query_i2c_devices() {
    // 0x0E - MAG3110   3-axis magnetometer
    // 0x19 - H3LIS200  3-axis accelerometer (0x18)
    // 0x60 - MPL3115A2 pressure sensor
    // 0x6B - LSM6DS33  iNEMO 3-axis gyro/accelerometer (0x6A)
    print("Scanning I2C bus...\n");
    uint8_t data[] = {0};
    for (uint8_t address = 0x08; address < 0x7F; address++) {
        if (0 == bus_i2c.check(address)) {
        //if (0 == bus_i2c.write2(address, data, 1)) {
            print("* detected %02Xh\n", address);
        }
    }
}

void query_sensors() {
    print("Magn: ");
    if (!gState.mag_initialized) {
        print("Uninitalized\n");
    }
    else {
        uint32_t age = millis() - gState.last_mag_time;
        print("Magn: [%d %d %d], %dC (%ld ms)\n", 
            gState.last_mx, gState.last_my, gState.last_mz, 
            gState.last_temp_mag, 
            age
        );
    }

    print("Baro: ");
    if (!gState.baro_initialized) {
        print("Uninitalized\n");
    }
    else {
        uint32_t age = millis() - gState.last_baro_time;
        print("%dPa, %d.%1dC (%ld ms)\n", 
            (gState.last_pressure + 8) / 16, 
            gState.last_temp_baro / 16, 
            ((gState.last_temp_baro % 16) * 10 + 8) / 16, 
            age
        );
    }

    print("Gyro: ");
    if (!gState.gyro_initialized) {
        print("Uninitalized\n");
    }
    else {
        //uint32_t age = millis() - gState.last_gyro_time;
        print("Initalized\n");
    }
}

void query_lora() {
    print("Checking LoRa...\n");

    uint8_t regs[] = { 0x01, 0x12, 0x0D, 0x11, 0x22 };
    const char *names[] = {"OpMode", "IRQFlags", "FifoAddr", "IrqMask", "PayloadLength" };

    for (uint8_t idx = 0; idx < 2; idx++) {
        uint8_t value = radio.readReg(regs[idx]);
        print("  [%d] (%s) = %02Xh\n", regs[idx], names[idx], value);
    }
}

void print_report() {
    query_lora();
    query_sensors();
    query_i2c_devices();

    char buf[16];
    print("%4d ", (millis() + 500)/ 1000);
    print("%d/%d ", gps.sentencesOK(), gps.sentencesErr());
    print("%d/%d ", gps.tracked().value(), gps.inView().value());

    if (gps.fixTime().valid()) {
        print("%02d:%02d:%02d ", 
            gps.fixTime().hour(), gps.fixTime().minute(), gps.fixTime().second());
    }
    if (gps.fixType().atLeast2D()) {
        if (gps.latitude().valid()) {
            print(ftostr( gps.latitude().degreesFloat(), 7, 4, buf, true)); 
            print(gps.latitude().cardinal()); print(' ');
        }
        if (gps.longitude().valid()) {
            print(ftostr(gps.longitude().degreesFloat(), 8, 4, buf, true));
            print(gps.longitude().cardinal()); print(' ');
        }
        if (gps.fixType().is3D()) {
            if (gps.altitude().valid()) {
                print(ftostr( gps.altitude().meters(), 5, 0, buf)); print("m ");
            }
        }
        else {
            print("2D fix");
        }
        print("\r\n");
    }
    else {
        print("NO fix\n");
    }    
}

void console_parse(const char *line) {
    bool cmd_ok = false;
    if (0 == strcmp(line, "gps_off")) {
        ublox_powermode(ePM_BACKUP);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_on")) {
        ublox_powermode(ePM_POWERON);
        delay(100);
        gState.gps_raw_mode = false;
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "gps_rst")) {
        ublox_reset();
        gState.gps_raw_mode = false;
        cmd_ok = true;
    }
    // else if (0 == strcmp(line, "nmea")) {
    //     gps_send_nmea("PUBX,41,1,0003,0003,9600,0");
    //     cmd_ok = true;
    // }
    else if (0 == strcmp(line, "gps_raw")) {
        gps_send_nmea("PUBX,41,1,0001,0001,9600,0");
        gState.gps_raw_mode = true;
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
    else if (0 == strcmp(line, "lora_test")) {
        uint8_t test_data[64];
        radio.setTXPower(13);
        radio.writeFIFO(test_data, sizeof(test_data));
        radio.startTX();
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "lora_test2")) {
        uint8_t test_data[64];
        radio.setTXPower(23);
        radio.writeFIFO(test_data, sizeof(test_data));
        radio.startTX();
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "mag_cal")) {
        //mag.rawData(true);
        gState.mag_cal_enabled = true;
        gState.mag_cal_start = millis();
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "save")) {
        print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
        gSettings.save();
        //uint32_t data[10];
        //int result = eeprom_write(0x08080100, data, 10);
        //print("Result: %d\n", result);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "restore")) {
        gSettings.restore();
        print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
        cmd_ok = true;
    }
    if (cmd_ok) print(line);
    else print("?");
}

void buzz_times(int times) {
    for (int i = 0; i < times; i++) {
        buzzerOn = 1;
        delay(100);
        buzzerOn = 0;
        delay(100);
    }
}

systime_t task_gps(systime_t due_time) {
    uint8_t ch;
    while (usart_gps.getc(ch)) {
        if (millis() - due_time > 10) break;
        if (gState.gps_raw_mode == 1) {
            print("%02X ", ch);
        }
        else if (gps.decode(ch)) {
            //
        }
    }

    // static bool platform_updated;
    // if (!platform_updated && millis() > 5000) {
    //     // Set GNSS platform type (e.g. airborne 1g)
    //     ublox_set_dyn_mode((ublox_dyn_t)gSettings.ublox_platform_type);

    //     platform_updated = true;
    // }

    // static bool fix_3d;
    // static bool fix_2d;
    // static uint32_t time_lost;

    // if (gps.fixType().atLeast2D()) {
    //     if (gps.fixType().is3D()) {
    //         if (!fix_3d) {
    //             print("Time to 3D fix: %ld seconds\n", (millis() - time_lost) / 1000);
    //             fix_3d = true;
    //             fix_2d = false;
    //             buzz_times(3);
    //         }
    //     } 
    //     else {
    //         if (!fix_2d) {
    //             print("Time to 2D fix: %ld seconds\n", (millis() - time_lost) / 1000);
    //             fix_2d = true;
    //             fix_3d = false;
    //             buzz_times(2);
    //         }
    //     }
    // }
    // else {
    //     if (fix_2d || fix_3d) {
    //         time_lost = millis();
    //         fix_2d = false;
    //         fix_3d = false;
    //         buzz_times(1);
    //     }
    // }

    return due_time + 100;
}

systime_t task_console(systime_t due_time) {
    static char rx_line[80];
    static int  rx_len;
    uint8_t ch;

    while (usb_cdc_read((uint8_t *)&ch, 1)) {
        if (millis() - due_time > 5) break;
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
            } else {
                print_report();
            }
            print("> ");
            rx_len = 0;
        }
    }

    return due_time + 50;
}

systime_t task_report(systime_t due_time) {
    if (gps.fixTime().valid()) {
        uint8_t packet_data[64];
        int     packet_length = 64;
        
        memcpy(gState.telemetry.callsign, gSettings.radio_callsign, 16);
        gState.telemetry.msg_id++;
        if (gState.telemetry.fixValid = gps.fixType().is3D()) {
            gState.telemetry.lat = gps.latitude().degreesSignedFloat();
            gState.telemetry.lng = gps.longitude().degreesSignedFloat();
            gState.telemetry.alt = gps.altitude().meters();
        }
        gState.telemetry.n_sats = gps.tracked().value();
        gState.telemetry.hour = gps.fixTime().hour();
        gState.telemetry.minute = gps.fixTime().minute();
        gState.telemetry.second = gps.fixTime().second();

        if (gState.telemetry.build_string((char *)packet_data, packet_length)) {
            print("Telemetry: [%s]\n", (const char *)packet_data);
            radio.writeFIFO(packet_data, packet_length);
            radio.startTX();
        }            
    }
    return due_time + 5000;
}

systime_t task_led_blink(systime_t due_time) {
    static bool led_on;

    if (!led_on) {
        led_on = true;
        statusLED = 1;
        return due_time + 200;
    }
    else {
        led_on = false;
        statusLED = 0;
        if (gps.fixType().atLeast2D()) {
            return due_time + (gps.fixType().is3D() ? 800 : 300);
        }
        return due_time + 200;
    }
}

systime_t task_buzz(systime_t due_time) {
    static bool buzzer_on;

    if (!buzzer_on) {
        buzzer_on = true;
        buzzerOn = 1;
        return due_time + 200;
    }
    else {
        buzzer_on = false;
        buzzerOn = 0;
        return due_time + 800;
    }
}

systime_t task_sensors(systime_t due_time) {
    static int      cal_my_min, cal_my_max;
    static int      cal_count;

    if (!gState.baro_initialized) {
        if (baro.initialize()) {
            // Approx 50 ms sampling time
            baro.reset(MPL3115::eOSR_16);
            // temp offset -1.5 C
            gState.baro_initialized = true;
            baro.trigger();
        }
    }
    else if (baro.dataReady()) {
        gState.last_baro_time = millis();
        baro.readPressure_u28q4(gState.last_pressure);
        baro.readTemperature_12q4(gState.last_temp_baro);

        // trigger a new conversion
        baro.trigger();
    }

    if (!gState.mag_initialized) {
        if (mag.initialize()) {
            // Approx 25 ms sampling time
            mag.reset(MAG3110::eRATE_1280, MAG3110::eOSR_32);      
            
            mag.setOffset(MAG3110::eY_AXIS, (gSettings.mag_y_min + gSettings.mag_y_max) / 2);
            mag.rawData(false); 
            // temp offset +12 C
            
            gState.mag_initialized = true;
            mag.trigger();
        }
    }
    else if (mag.dataReady()) {
        gState.last_mag_time = millis();
        mag.readMag(gState.last_mx, gState.last_my, gState.last_mz);
        mag.readTemperature(gState.last_temp_mag);

        if (gState.mag_cal_enabled) {
            if (cal_count == 0 || gState.last_my > cal_my_max) cal_my_max = gState.last_my;
            if (cal_count == 0 || gState.last_my < cal_my_min) cal_my_min = gState.last_my;
            cal_count++;
            if (millis() - gState.mag_cal_start >= 5000) {
                gState.mag_cal_enabled = false;
                gSettings.mag_y_min = cal_my_min;
                gSettings.mag_y_max = cal_my_max;
                print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
                //gSettings.save();

                mag.setOffset(MAG3110::eY_AXIS, (cal_my_min + cal_my_max) / 2);
                mag.rawData(false);
                buzz_times(4);
            }
        }

        // trigger a new conversion
        mag.trigger();
    }

    if (!gState.gyro_initialized) {
        if (gyro.initialize()) {
            // Approx 5 ms sampling time?
            //gyro.reset(MPL3115::eOSR_16);
            gState.gyro_initialized = true;
            //baro.trigger();
        }
    }

    return due_time + 100;
}

timer_task_t timer_tasks[6];

int main() {
    setup();

    add_task(&timer_tasks[0], task_led_blink);
    add_task(&timer_tasks[1], task_console);
    add_task(&timer_tasks[2], task_gps);
    add_task(&timer_tasks[3], task_sensors);
    add_task(&timer_tasks[4], task_report);
    //add_task(&timer_tasks[5], task_buzz);

    while (1) {
        schedule_tasks();
        __asm("WFI");
    }
}

// TODO: 
// - better buzzer/LED indication
// - mag calibration, EEPROM save
// - UKHAS packet forming, regular TX
// - check arm/safe status, pyro activation
// - simple SPI flash logging
// - battery/pyro voltage sensing
// - timer-based mag/baro polling
// - console parameter get/set

typedef struct {
    uint16_t    address;    // 7/10 bit address
    uint8_t     type;       // read/write, stop/continue
    uint8_t     status;     // busy/complete/error
    uint8_t     *rx_buf;
    uint8_t     *tx_buf;
    uint16_t    rx_len;
    uint16_t    tx_len;
} i2c_request_t;

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
