#include "console.h"

#include "settings.h"
#include "systick.h"
#include "ublox.h"
#include "strconv.h"

extern "C" {
#include "cdcacm.h"
}

#include <cstring>
#include <cstdio>
#include <cstdarg>

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
    gState.usart_gps.putc(c);
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

void query_i2c_devices() {
    // 0x0E - MAG3110   3-axis magnetometer
    // 0x19 - H3LIS200  3-axis accelerometer (0x18)
    // 0x60 - MPL3115A2 pressure sensor
    // 0x6B - LSM6DS33  iNEMO 3-axis gyro/accelerometer (0x6A)
    print("Scanning I2C bus...\n");
    uint8_t data[] = {0};
    for (uint8_t address = 0x08; address < 0x7F; address++) {
        if (0 == gState.bus_i2c.check(address)) {
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
        uint32_t age = millis() - gState.last_time_mag;
        print("[%d %d %d], %dC (%ld ms)\n", 
            gState.last_mx, gState.last_my, gState.last_mz, 
            gState.last_temp_mag / 16, 
            //((gState.last_temp_mag % 16) * 10 + 8) / 16, 
            age
        );
    }

    print("Baro: ");
    if (!gState.baro_initialized) {
        print("Uninitalized\n");
    }
    else {
        uint32_t age = millis() - gState.last_time_baro;
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
        uint8_t value = gState.radio.readReg(regs[idx]);
        print("  [%d] (%s) = %02Xh\n", regs[idx], names[idx], value);
    }
}

void query_flash() {
    // if (flash_initialized) {
    // }
    // else {
    //     print("Flash: uninitialized\n");
    // }
}

void print_report() {
    query_lora();
    query_sensors();
    query_i2c_devices();
    //query_flash();

    char buf[16];

    const GPSParserSimple &gps = gState.gps;

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
        gState.gps_raw_mode = 0;
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
    else if (0 == strcmp(line, "gps_raw2")) {
        ublox_reset();
        gState.gps_raw_mode = 2;
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
        gState.radio.setTXPower(13);
        gState.radio.writeFIFO(test_data, sizeof(test_data));
        gState.radio.startTX();
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "lora_test2")) {
        uint8_t test_data[64];
        gState.radio.setTXPower(23);
        gState.radio.writeFIFO(test_data, sizeof(test_data));
        gState.radio.startTX();
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "mag_cal")) {
        gState.mag_cal_enabled = true;
        gState.mag_cal_start = millis();
        gState.mag.rawData(true);
        //mag.setOffset(MAG3110::eY_AXIS, 0);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "ls")) {
        print("Files in /\n");
        lfs_dir_t dir;
        int err;

        err = lfs_dir_open(&gState.lfs, &dir, "");
        if (err < 0) {
            print("Error while opening directory\n");
        } else {
            lfs_info info;
            while (lfs_dir_read(&gState.lfs, &dir, &info) > 0) {
                if (info.type == LFS_TYPE_REG) {
                    print("  %s   [%ld]\n", info.name, info.size);
                }
                else {
                    print("  %s/\n", info.name);
                }
            }
        }
        err = lfs_dir_close(&gState.lfs, &dir);
        if (err < 0) print("Error while closing directory\n");

        err = gState.free_space();
        if (err < 0) print("Error while calculating free space\n");
        else print("Free space: %ld\n", err);

        cmd_ok = true;        
    }
    else if (0 == strcmp(line, "save")) {
        print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
        int err = gSettings.save();
        if (err) {
            print("Save err = %d\n", err);
        } else {
            print("Save OK\n");
        }
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "restore")) {
        int err = gSettings.restore();
        if (err) {
            print("Restore err = %d\n", err);
        } else {
            print("Restore OK\n");
            gState.mag.setOffset(MAG3110::eY_AXIS, (gSettings.mag_y_min + gSettings.mag_y_max) / 2);
        }
        print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
        cmd_ok = true;
    }
    else if (0 == strcmp(line, "resetf")) {
        gSettings.reset();
        print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
        cmd_ok = true;
    }
    if (cmd_ok) print(line);
    else print("?");
}
