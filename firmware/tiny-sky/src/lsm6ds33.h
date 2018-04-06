#pragma once

#include <stdint.h>

#include <ptlib/ptlib.h>

class LSM6DS33 : protected I2CDeviceBase {
public:
    LSM6DS33(I2CBase &bus) : I2CDeviceBase(bus, 0x6B) {}

    enum odr_xl_t {
        eODR_XL_POWERDOWN = 0,
        eODR_XL_12_5Hz    = 1,
        eODR_XL_26Hz      = 2,
        eODR_XL_52Hz      = 3,
        eODR_XL_104Hz     = 4,
        eODR_XL_208Hz     = 5,
        eODR_XL_416Hz     = 6,
        eODR_XL_833Hz     = 7,
        eODR_XL_1_6kHz    = 8,
        eODR_XL_3_3kHz    = 9,
        eODR_XL_6_6kHz    = 10
    };

    enum fs_xl_t {
        eFS_XL_2G   = 0,
        eFS_XL_4G   = 1,
        eFS_XL_8G   = 2,
        eFS_XL_16G  = 3
    };

    enum bw_xl_t {
        eBW_XL_400Hz   = 0,
        eBW_XL_200Hz   = 1,
        eBW_XL_100Hz   = 2,
        eBW_XL_50Hz    = 3
    };

    enum odr_g_t {
        eODR_G_POWERDOWN = 0,
        eODR_G_12_5Hz    = 1,
        eODR_G_26Hz     = 2,
        eODR_G_52Hz     = 3,
        eODR_G_104Hz    = 4,
        eODR_G_208Hz    = 5,
        eODR_G_416Hz    = 6,
        eODR_G_833Hz    = 7,
        eODR_G_1_6kHz   = 8
    };

    enum fs_g_t {
        eFS_G_125DPS   = 1,
        eFS_G_245DPS   = 0,
        eFS_G_500DPS   = 2,
        eFS_G_1000DPS  = 4,
        eFS_G_2000DPS  = 6
    };

    bool initialize();
    void reset(odr_xl_t osr = eODR_XL_POWERDOWN);

    void start();
    void enterStandby();
    void exitStandby();

    void trigger();
    bool dataReady();

    //void readMeasurement(uint32_t &pressure, uint16_t &temperature);
    void readPressure_28q4(uint32_t &pressure);
    void readTemperature_12q4(int16_t &temperature);

    void setODR(odr_xl_t odr_xl, odr_g_t odr_g);

private:
};
