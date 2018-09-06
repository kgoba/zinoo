#include "settings.h"

#include <libopencm3/stm32/adc.h>

#include <cstring>

#include "systick.h"
#include "console.h"
#include "storage.h"

extern "C" {
#include "cdcacm.h"
}


// Global shared variables
AppCalibration  gCalibration;
AppSettings     gSettings;
AppState        gState;

// Test::Test() : 
//     radio_callsign { "TEST" },
//     params {
//         { "radio_callsign", radio_callsign },
//         { "radio_frequency", radio_frequency },
//     }
// {
// }

int AppSettings::findByName(const char *name) {
    for (int i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
        if (strcmp(name, params[i].name) == 0) return i;
    }
    return -1;
}

AppSettings::AppSettings() :
    params {
        { "radio_callsign",  PARAM_STR, 16, &radio_callsign[0] },
        { "radio_frequency", PARAM_INT,  4, &radio_frequency },
        { "radio_tx_power",  PARAM_INT, -1, &radio_tx_power },
        { "radio_tx_period", PARAM_INT, 2, &radio_tx_period },
        { "radio_tx_start",  PARAM_INT, 2, &radio_tx_start },

        { "pyro_safe_time",  PARAM_INT, 2, &pyro_safe_time },
        //{ "pyro_safe_altitude",  PARAM_INT, 2, &pyro_safe_altitude },
        { "pyro_hold_time",  PARAM_INT, 2, &pyro_hold_time },
        { "pyro_min_voltage",  PARAM_INT, 2, &pyro_min_voltage },

        // { "log_mag_interval",  PARAM_INT, 2, &log_mag_interval },
        // { "log_acc_interval",  PARAM_INT, 2, &log_mag_interval },
        // { "log_gyro_interval",  PARAM_INT, 2, &log_gyro_interval },
        // { "log_baro_interval",  PARAM_INT, 2, &log_baro_interval },

        // { "ublox_platform_type",  PARAM_INT, 1, &ublox_platform_type }
    }
{   
}

void AppSettings::reset() {
    ublox_platform_type = (uint8_t)eDYN_AIRBORNE_1G;    // TODO: should be 4G?
    radio_callsign[0]   = 'T';
    radio_callsign[1]   = 'S';
    radio_callsign[2]   = 'T';
    radio_callsign[3]   = '\0';
    radio_tx_period     = 5;
    radio_tx_start      = 0;
    radio_frequency     = 434.25f * 1000000;
    radio_cw_frequency  = 444.25f * 1000000;
    radio_tx_power      = 13;   // TODO: change
    pyro_safe_time      = 10;   // TODO: change
    pyro_safe_altitude  = 50;
    pyro_hold_time      = 3000;
    pyro_min_voltage    = 2500;
    log_mag_interval    = 1000 / 20;
    log_acc_interval    = 1000 / 100;
    log_gyro_interval   = 1000 / 100;
    log_baro_interval   = 1000 / 10;

    gyro_temp_offset_q4  = -29.5f * 16;
}

void AppCalibration::reset() {
    mag_x_offset        = 0;
    mag_temp_offset_q4  = -12.0f * 16;
    baro_temp_offset_q4 =   0.5f * 16;
}


int AppSettings::save() {
    // address must point to EEPROM space (begins at 0x0808 0000)
    // void eeprom_program_word(uint32_t address, uint32_t data);
    // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    // uint32_t addr_eeprom = 0x08080000 + 0;
    // const uint32_t n_words = (3 + sizeof(AppSettings)) / 4;

    // eeprom_write(addr_eeprom, (uint32_t *)(this), n_words);
    
    must_be_zero = 0;
    while (gState.flash.busy()) {
        // idle wait
    }
    gState.flash.eraseSector(0x0000);
    while (gState.flash.busy()) {
        // idle wait
    }
    extflash_write(0x0000, (const uint8_t *)this, sizeof(AppSettings));

    // lfs_file_t  file;
    // int         err;

    // // Attempt to open settings file
    // // TODO: do we need the LFS_O_TRUNC?
    // err = lfs_file_open(&gState.lfs, &file, "settings", LFS_O_WRONLY | LFS_O_TRUNC | LFS_O_CREAT);
    // if (err) {
    //     return -1;
    // }

    // err = lfs_file_write(&gState.lfs, &file, (const void *)this, sizeof(AppSettings));
    // lfs_file_close(&gState.lfs, &file);

    // // // TODO: check err and do something about it (retry?)
    // if (err < 0) return -2;
    return 0;
}

int AppSettings::restore() {
    extflash_read(0x0000, (uint8_t *)this, sizeof(AppSettings));
    if (must_be_zero != 0) {
        reset();
    }

    // lfs_file_t  file;
    // int         err;

    // // Attempt to open settings file
    // err = lfs_file_open(&gState.lfs, &file, "settings", LFS_O_RDONLY);  // LFS_O_CREAT
    // if (err) {
    //     reset();
    //     // File not open, so just return
    //     return -1;
    // }

    // err = lfs_file_read(&gState.lfs, &file, (void *)this, sizeof(AppSettings));
    // lfs_file_close(&gState.lfs, &file);

    // if (err < 0) {
    //     // Reading failed 
    //     reset();
    //     return -2;
    // }

    // // if (err < sizeof(AppSettings))   // or we read too few bytes

    return 0;

    // // address must point to EEPROM space (begins at 0x0808 0000)
    // // void eeprom_program_word(uint32_t address, uint32_t data);
    // // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    // //eeprom_program_words(0x08080000 + 0, (uint32_t *)(this), (3 + sizeof(AppSettings)) / 4);
    // uint32_t addr_eeprom = 0x08080000 + 0;
    // const uint32_t n_bytes = sizeof(AppSettings);

    // memcpy((void *)(this), (const void *)addr_eeprom, n_bytes);
}

int AppCalibration::save() {
    must_be_zero = 0;
    while (gState.flash.busy()) {
        // idle wait
    }
    gState.flash.eraseSector(0x1000);
    while (gState.flash.busy()) {
        // idle wait
    }
    extflash_write(0x1000, (const uint8_t *)this, sizeof(AppCalibration));
    return 0;
}

int AppCalibration::restore() {
    extflash_read(0x1000, (uint8_t *)this, sizeof(AppCalibration));
    if (must_be_zero != 0) {
        reset();
    }
    return 0;
}

int AppState::init_lfs() {
    if (!flash.initialize()) return -1;
    delay(10);

    return xlog_init();
}

int AppState::init_hw() {
    // Initialize internal peripherals
    usb_setup();
    usb_enable_interrupts();

    adc.begin();
    adc.enableVREFINT();
    adc.setConversionMode(adc.eDISCONTINUOUS);
    adc.setSampleTime(7);

    usart_gps.begin();

    bus_i2c.begin();
    //bus_i2c.enableIRQ();
    //bus_i2c.enableSTOPInterrupt();
    //bus_i2c.enableNACKInterrupt();

    bus_spi.begin();
    bus_spi.format(8, 0);
    bus_spi.frequency(16000000);

    loraDIO2.begin();
    loraRST.begin(1);
    loraSS.begin(1);
    flashSS.begin(1);

    pyro1On.begin();    // TODO: leave uninitialized/floating for safety
    pyro2On.begin();

    arm_sense.begin();

    statusLED.begin();
    buzzerOn.begin();

    adc_v_batt.begin();
    adc_v_pyro.begin();
    adc_pyro_sense1.begin();
    adc_pyro_sense2.begin();

    return 0;    
}

int AppState::init_periph() {
    // Restore settings
    if (0 == init_lfs()) {
        gSettings.reset();
        //gSettings.restore();
        gCalibration.restore();
        // // Attempt to open log file
        // int err = lfs_file_open(&lfs, &log_file, "log", LFS_O_RDWR | LFS_O_APPEND | LFS_O_CREAT);  // LFS_O_CREAT
        // if (!err) log_file_ok = true;
        log_file_ok = true;
    } else {
        gSettings.reset();
        gCalibration.reset();
    }

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

    // Initialize magnetic field sensor
    if (mag.initialize()) {
        // Approx 50 ms sampling time?
        mag.reset(MAG3110::eRATE_1280, MAG3110::eOSR_32);      
        mag.rawData(false); 
        
        mag_initialized = true;
        mag.trigger();
        last_time_mag = millis();
    }

    // Initialize barometric sensor
    if (baro.initialize()) {
        // Approx 50 ms sampling time
        baro.reset(MPL3115::eOSR_16);
        baro_initialized = true;
        baro.trigger();
        last_time_baro = millis();
    }

    // Initialize gyroscopic sensor
    if (gyro.initialize()) {
        // Less than 10 ms sampling time
        gyro.setupGyro(LSM6DS33::eODR_G_104Hz, LSM6DS33::eFS_G_2000DPS); // 0.070 dps/LSB // TODO: change FS
        gyro.setupAccel(LSM6DS33::eODR_XL_104Hz, LSM6DS33::eFS_XL_16G); // 0.488 mg/LSB
        gyro_initialized = true;
        gyro.trigger();
        last_time_gyro = millis();
    }
    
    state = eSAFE;

    // Make a beep to indicate startup
    statusLED = buzzerOn = 1;
    delay(500);
    if (!mag_initialized) {
        while (true) { ; }
    }
    statusLED = buzzerOn = 0;
}

void AppState::buzz_times(int times) {
    for (int i = 0; i < times; i++) {
        gState.buzzerOn = 1;
        delay(100);
        gState.buzzerOn = 0;
        delay(100);
    }
}

systime_t AppState::task_gps(systime_t due_time) {
    uint8_t ch;
    while (usart_gps.getc(ch)) {
        if (millis() - due_time > 10) break;
        if (gps_raw_mode == 1) {
            print("%02X ", ch);
        }
        else {
            if (gps_raw_mode == 2) {
                print(ch);
            }   
            gps.decode(ch);
        }
    }

    static bool platform_updated;
    if (!platform_updated && millis() > 5000) {
        // Set GNSS platform type (e.g. airborne 1g)
        ublox_set_dyn_mode((ublox_dyn_t)gSettings.ublox_platform_type);

        platform_updated = true;
    }

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

    return 100;
}

systime_t AppState::task_console(systime_t due_time) {
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

    return 50;
}

systime_t AppState::task_report(systime_t due_time) {
    static uint8_t counter;

    bool is_armed = (state != eSAFE);

    counter++;
    if (counter == 5) {
        counter = 0;
    }
    
    if (counter == 0 && gps.fixTime().valid()) {
        uint8_t packet_data[64];
        int     packet_length = 64;
        
        memcpy(telemetry.callsign, gSettings.radio_callsign, 16);
        telemetry.msg_id++;
        if (telemetry.fixValid = gps.fixType().is3D()) {
            telemetry.lat = gps.latitude().degreesSignedFloat();
            telemetry.lng = gps.longitude().degreesSignedFloat();
            telemetry.alt = gps.altitude().meters();
        }
        telemetry.n_sats = gps.tracked().value();
        int temp_int = 0;
        int temp_cnt = 0;
        if (mag_initialized) {
            temp_int += last_temp_mag;
            temp_cnt++;
        }
        if (baro_initialized) {
            temp_int += last_temp_baro;
            temp_cnt++;
        }
        if (temp_cnt) temp_int /= temp_cnt;

        telemetry.temperature_int = (temp_int + 8) / 16;
        telemetry.pyro_voltage = gState.last_v_pyro;
        telemetry.battery_voltage = gState.last_v_batt;

        telemetry.hour = gps.fixTime().hour();
        telemetry.minute = gps.fixTime().minute();
        telemetry.second = gps.fixTime().second();

        if (telemetry.build_string((char *)packet_data, packet_length)) {
            if (state != eFLIGHT) {
                radio.writeFIFO(packet_data, packet_length);
                radio.startTX();
            }
        }
    }

    if (log_file_ok && is_armed && gps.fixType().is3D() && gps.altitude().valid()) {
        xlog_pos(millis(), gps.latitude().degreesFloat(), gps.longitude().degreesFloat(), gps.altitude().meters());
    }
    // if (log_file_ok) {
    //     lfs_file_sync(&lfs, &log_file);
    // }

    return 1000;
}

systime_t AppState::task_led(systime_t due_time) {
    static bool led_on;

    if (!led_on) {
        led_on = true;
        statusLED = 1;
        if (gps.fixType().atLeast2D()) {
            return 200;
        } else {
            return 100;
        }
    }
    else {
        led_on = false;
        statusLED = 0;
        if (gps.fixType().atLeast2D()) {
            return (gps.fixType().is3D() ? 800 : 300);
        } else {
            return 100;
        }
    }
}

systime_t AppState::task_buzz(systime_t due_time) {
/*
STARTUP:
- MAG error   - BUZZ continuously
- BARO error  - 
- GYRO error  - 
- FLASH error - 

SAFE mode:
- MAG test              - LED continuous? BUZZ? telemetry?
- No GPS lock           - LED fast + telemetry
- Low pyro voltage      - telemetry
- No Pyro1 continuity   - telemetry?
- No Pyro2 continuity   - telemetry?

ARM mode:
- Safe timeout          - BUZZ
- No Pyro1 continuity   - BUZZ
- No Pyro2 continuity   - BUZZ
*/
    static bool buzzer_on;

    bool is_armed = (state != eSAFE);
    bool is_pyro_unlocked = (state == eFLIGHT || state == eRECOVERY);

    if (!is_armed) {
        if (!is_pyro1_on) {
            buzzer_on = false;
            buzzerOn = 0;
            return 500;
        }

        buzzer_on = !buzzer_on;
        buzzerOn = (buzzer_on) ? 1 : 0;
        return 100;
    }

    if (!buzzer_on) {
        buzzer_on = true;
        buzzerOn = 1;
        return (is_pyro_unlocked ? 200 : 1000);
    }
    else {
        buzzer_on = false;
        buzzerOn = 0;
        return (is_pyro_unlocked ? 200 : 1000);
    }
}

systime_t AppState::task_sensors(systime_t due_time) {
    static int      cal_mx_min, cal_mx_max;
    static int      cal_count;

    bool is_armed = (state != eSAFE);

    adc.setChannel(ADC_CHANNEL_VREF);
    adc.startConversion();
    while (!adc.isEOC());
    uint16_t v_dd = adc.read();
    last_vdd = 1220 * (1 << 12) / v_dd; // VREFINT = 1224 mV
    //last_vdd = (3000 * ST_VREFINT_CAL) / v_dd;

    adc.setChannel(adc_v_batt.channel);
    adc.startConversion();
    while (!adc.isEOC());
    uint16_t v_batt = adc.read();
    last_v_batt = (v_batt * 2 * last_vdd) / (1 << 12);

    adc.setChannel(adc_v_pyro.channel);
    adc.startConversion();
    while (!adc.isEOC());
    uint16_t v_pyro = adc.read();
    last_v_pyro = (v_pyro * 2 * last_vdd) / (1 << 12);

    adc.setChannel(adc_pyro_sense1.channel);
    adc.startConversion();
    while (!adc.isEOC());
    uint16_t v_sense1 = adc.read();
    last_pyro_sense1 = (v_sense1 * 2 * last_vdd) / (1 << 12);

    // TODO: fake pyro_2 readings for now
    last_pyro_sense2 = last_v_pyro;

    if (baro_initialized && baro.dataReady()) {
        baro.readPressure_u28q4(last_pressure);
        int16_t temp;
        baro.readTemperature_12q4(temp);
        last_temp_baro = temp - gCalibration.baro_temp_offset_q4;

        if (log_file_ok && is_armed) {
            xlog_baro(last_time_baro, last_pressure, last_temp_baro);
        }

        // trigger a new conversion
        baro.trigger();
        last_time_baro = millis();
    }

    if (mag_initialized && mag.dataReady()) {
        int16_t m1, m2, m3;
        mag.readMag(m1, m2, m3);
        last_mx = m2 - gCalibration.mag_x_offset;
        last_my = -m1;
        last_mz = m3;            

        int temp;
        mag.readTemperature(temp);
        last_temp_mag = (temp << 4) - gCalibration.mag_temp_offset_q4;

        if (mag_cal_enabled) {
            cal_count++;
            if (cal_count > 3) {
                if (last_mx > cal_mx_max) cal_mx_max = last_mx;
                if (last_mx < cal_mx_min) cal_mx_min = last_mx;
            } else {
                cal_mx_max = cal_mx_min = last_mx;
            }

            if (millis() - mag_cal_start >= 10000) {
                gCalibration.mag_x_offset = cal_mx_min + (cal_mx_max - cal_mx_min) / 2;
                print("Mag offset = %d [%d,%d]\n", gCalibration.mag_x_offset, cal_mx_min, cal_mx_max);
                mag_cal_enabled = false;
                cal_count = 0;

                buzz_times(4);
            }
        }

        if (log_file_ok && is_armed) {
            xlog_mag(last_time_mag, last_mx, last_temp_mag);
        }

        // trigger a new conversion
        mag.trigger();
        last_time_mag = millis();
    }

    if (gyro_initialized && gyro.dataReady()) {
        int16_t wx, wy, wz, ax, ay, az;
        gyro.readMeasurement(wx, wy, wz, ax, ay, az);
        last_wx = -wx;
        last_wy = wy;
        last_wz = -wz;
        last_ax = -ax;
        last_ay = ay;
        last_az = -az;
        int16_t temp;
        gyro.readTemperature_12q4(temp);
        last_temp_gyro = temp - gSettings.gyro_temp_offset_q4;

        // trigger a new conversion
        gyro.trigger();
        last_time_gyro = millis();
    }

    return 100;
}

systime_t AppState::task_control(systime_t due_time) {
    static uint32_t timeout;

    bool trig = (last_mx < 0);
    is_pyro1_on = trig;

    switch (state) {
    case eSAFE:
        pyro1On = 0;
        if (arm_sense.read() != 0) {
            // SAFE pin pulled    
            // Start safe timer
            timeout = millis() + 1000UL * gSettings.pyro_safe_time;
            xlog_arm(millis(), gps.fixTime().hour(), gps.fixTime().minute(), gps.fixTime().second());
            state = eARMED;
        }
        break;
    case eARMED:
        if (arm_sense.read() == 0) {
            // SAFE pin in place
            state = eSAFE;
            break;
        }
        if (millis() >= timeout) {
            // Timer expired; unlock pyro

            radio.sleep();
            delay(5);
            radio.setupFSK();
            radio.setFrequencyHz(gSettings.radio_cw_frequency);

            radio.startTX();

            state = eFLIGHT;
        }
        break;
    case eFLIGHT:
        if (arm_sense.read() == 0) {
            // SAFE pin in place
            state = eSAFE;
            break;
        }
        if (trig) {
            pyro1On = 1;
            timeout = millis() + 3000;

            radio.sleep();
            delay(5);
            radio.setupLoRa(RFM96::ModemSettings()
                .setSF(RFM96::eSF_10)
                .setCR(RFM96::eCR_4_8)
            );
            radio.setFrequencyHz(gSettings.radio_frequency);

            xlog_eject(millis());
            state = eRECOVERY;
        }
        break;
    case eRECOVERY:
        if (arm_sense.read() == 0) {
            // SAFE pin in place
            state = eSAFE;
            break;
        }
        if (millis() >= timeout) {
            pyro1On = 0;
        }
        break;
    }

    return 200;
}

int AppState::free_space() {
    return xlog_free_space();
}