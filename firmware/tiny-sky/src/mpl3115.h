#pragma once

#include <stdint.h>

#include <ptlib/ptlib.h>

class MPL3115 : protected I2CDeviceBase {
public:
    MPL3115(I2CBase &bus) : I2CDeviceBase(bus, 0x60) {}

    enum osr_t {
        eOSR_1    = 0,
        eOSR_2    = 1,
        eOSR_4    = 2,
        eOSR_8    = 3,
        eOSR_16   = 4,
        eOSR_32   = 5,
        eOSR_64   = 6,
        eOSR_128  = 7
    };

    bool initialize();
    void reset(osr_t osr = eOSR_1);

    void start();
    void enterStandby();
    void exitStandby();

    void trigger();
    bool dataReady();

    //void readMeasurement(uint32_t &pressure, uint16_t &temperature);
    void readPressure_28q4(uint32_t &pressure);
    void readTemperature_12q4(int16_t &temperature);

    void setOSR(osr_t osr);

private:
};
