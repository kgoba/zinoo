#include "FixedPoint.hh"

#include <i2c.hh>

#include <util/delay.h>

class MS5607Base {
public:
  enum OversamplingRate {
    kOSR256   = 0,
    kOSR512   = 1,
    kOSR1024  = 2,
    kOSR2048  = 3,
    kOSR4096  = 4
  };
  
  enum ConversionType {
    kPressure       = 0,
    kTemperature    = 1
  };
};

template<class I2CPeriph, byte subAddress = 0>
class MS5607 : public MS5607Base {
public:
    
  static bool reset() {
    byte cmd = kCmdReset;
    return Device::send(cmd);
  }
  
  static bool readPROM(byte address, word &result) {
    byte cmd[1];
    cmd[0] = kCmdReadPROM | ((address & 0x07) << 1);
    if (!Device::send(cmd, 1)) {
      return false;
    }
    
    byte reply[2];
    if (!Device::receive(reply, ARRAY_SIZE(reply))) {
      return false;
    }

    result = ((word)reply[0] << 8) | reply[1];
    return true;
  }
  
  static bool startConversion(ConversionType type, OversamplingRate osr = kOSR256) {
    byte cmd[1];
    cmd[0] = kCmdConvert | ((byte)type << 4) | ((byte)osr << 1);
    return Device::send(cmd, 1);
  }
  
  static bool readResult24(long &result) {
    byte cmd[1];
    cmd[0] = kCmdReadADC;
    _delay_ms(5);
    
    byte reply[3];
    Device::receive(reply, ARRAY_SIZE(reply));
    _delay_ms(5);
    
    result = ((long)reply[0] << 16) | ((word)reply[1] << 8) | reply[2];
    return true;
  }
  
  static bool readResult16(word &result) {
    byte cmd[1];
    cmd[0] = kCmdReadADC;
    if (!Device::send(cmd, 1)) {
      return false;
    }
    
    byte reply[2];
    if (!Device::receive(reply, ARRAY_SIZE(reply))) {
      return false;
    }
    
    result = ((word)reply[0] << 8) | reply[1];
    return true;
  }
  
private:
  typedef I2CDevice<I2CPeriph, (0x76 | (subAddress & 1))> Device;
  
  enum Commands {
    kCmdReset      = 0b00011110,
    kCmdReadPROM   = 0b10100000,
    kCmdConvert    = 0b01000000,
    kCmdReadADC    = 0b00000000,
  };
};

template<class I2CPeriph, byte subAddress = 0>
class Barometer : public MS5607<I2CPeriph, subAddress> {
public:
  static void initialize() {
    for (byte idx = 0; idx < 8; idx++) {
      Base::readPROM(idx, PROM[idx]);
    }
  }
  
  /*
   * Measures and updates both temperature and pressure
   */
  static bool update() {
    p = t = 0;
    if (!Base::startConversion(Base::kTemperature, Base::kOSR1024)) {
      return false;
    }
    _delay_ms(30);
    word d2;
    if (!Base::readResult16(d2)) {
      return false;
    }
    int16_t dt = d2 - PROM[5];
    t = 2000 + (((int32_t)dt * PROM[6]) >> 15);
    
    if (!Base::startConversion(Base::kPressure, Base::kOSR1024)) {
      return false;
    }
    _delay_ms(30);
    word d1;
    if (!Base::readResult16(d1)) {
      return false;
    }
    uint16_t off  = PROM[2] + ((PROM[4] * (int32_t)dt) >> 15);
    uint16_t sens = PROM[1] + ((PROM[3] * (int32_t)dt) >> 15);
    p = (((uint32_t)d1 * sens) >> 14) - off;
    
    return true;
  }
  
  static int16_t getTemperature() {
    return t;
  }

  static uint16_t getPressure() {
    return p;
  }
  
private:
  typedef MS5607<I2CPeriph, subAddress> Base;
    
  static uint16_t PROM[8];
  static int16_t  t;        // temperature in Celsium x100 (2000 = 20.00 C)
  static uint16_t p;        // pressure in Pascals / 4 (25000 = 100000 Pa = 1000 mbar)
};

template<class I2CPeriph, byte subAddress>
uint16_t Barometer<I2CPeriph, subAddress>::PROM[8];

template<class I2CPeriph, byte subAddress>
int16_t  Barometer<I2CPeriph, subAddress>::t;

template<class I2CPeriph, byte subAddress>
uint16_t  Barometer<I2CPeriph, subAddress>::p;
