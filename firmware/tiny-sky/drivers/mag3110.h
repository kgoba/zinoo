#pragma once

#include <stdint.h>

#include <ptlib/ptlib.h>


class MAG3110 : protected I2CDeviceBase {
public:
    MAG3110(I2CBase &bus) : I2CDeviceBase(bus, 0x0E) {}

    enum rate_t {
        eRATE_1280 = 0,
        eRATE_640  = 1,
        eRATE_320  = 2,
        eRATE_160  = 3,
        eRATE_80   = 4,
        eRATE_40   = 5,
        eRATE_20   = 6,
        eRATE_10   = 7,
    };

    enum osr_t {
        eOSR_16    = 0,
        eOSR_32    = 1,
        eOSR_64    = 2,
        eOSR_128   = 3
    };

    enum axis_t {
        eX_AXIS = 1,
        eY_AXIS = 3,
        eZ_AXIS = 5
    };

    bool initialize();
    void reset(rate_t rate = eRATE_1280, osr_t osr = eOSR_16);

    void start();
    void enterStandby();
    void exitStandby();

    void trigger();
    bool dataReady();

    bool readMag(int16_t &x, int16_t &y, int16_t &z);
    bool readMicroTeslas(float &x, float &y, float &z);
    float readHeading();

    bool readTemperature(int &t_celsius);

    void setRate(rate_t rate, osr_t osr);
    void rawData(bool raw);

    void setOffset(axis_t axis, int16_t offset);
    int16_t  readOffset(axis_t axis);

private:
    int16_t readAxis(uint8_t axis);
};
