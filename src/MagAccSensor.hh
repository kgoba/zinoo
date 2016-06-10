#include "FixedPoint.hh"

#include <i2c.hh>

#include <util/delay.h>

template<class I2CPeriph, byte SA0 = 0>
class LSM303D {
public:

  static bool readTemperature(word &result) {
    return Device::readWord(kRegTemp | 0x80, result);
  }
    
  static bool readMagnetometer(word &X, word &Y, word &Z) {
    return readTriple(kOutXLoMag | 0x80);
  }
  
  static bool readAccelerometer(word &X, word &Y, word &Z) {
    return readTriple(kOutXLoMag | 0x80);
  }

  static bool getStatusMag(byte &status) {
    return Device::readByte(kRegStatusMag, status);
  }
  
  static bool getStatusAcc(byte &status) {
    return Device::readByte(kRegStatusMag, status);
  }
  
 
private:
  typedef I2CDevice<I2CPeriph, (0x1E - (SA0 & 1))> Device;

  enum Registers {
    kRegTemp         = 0x05,
    kRegStatusMag    = 0x07,
    kOutXLoMag       = 0x08,
    kOutYLoMag       = 0x0A,
    kOutZLoMag       = 0x0C,
    kOffXLoMag       = 0x16,
    kOffYLoMag       = 0x18,
    kOffZLoMag       = 0x1A,
    kRegCtrl0        = 0x1F,
    kRegCtrl1        = 0x20,
    kRegCtrl2        = 0x21,
    kRegCtrl3        = 0x22,
    kRegCtrl4        = 0x23,
    kRegCtrl5        = 0x24,
    kRegCtrl6        = 0x25,
    kRegCtrl7        = 0x26,
    kRegStatusAcc    = 0x27,
    kOutXLoAcc          = 0x28,
    kOutYLoAcc          = 0x2A,
    kOutZLoAcc          = 0x2C,
    kMultipleRead       = 0x80
  };
  
  static bool readTriple(byte cmd, word &X, word &Y, word &Z) {
    if (!Device::send(&cmd, 1, I2CPeriph::kNoStop)) {
      return false;
    }
    byte data[6];
    if (!Device::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    X = ((word)data[0] << 8) | data[1];
    Y = ((word)data[2] << 8) | data[3];
    Z = ((word)data[4] << 8) | data[5];
    return true;
  }
};


template<class I2CPeriph = TWIMaster>
class HumiditySensor : public HTU21D<I2CPeriph> {
public:
  static bool initialize() {
    Sensor::softReset();
    _delay_ms(20);
    return true;
  }
  
  static bool update() {
    word result, X, Y, Z;
    
    if (!Sensor::readMagnetometer(X, Y, Z)) {
      return false;
    }

    if (!Sensor::readAccelerometer(X, Y, Z)) {
      return false;
    }
    
    if (!Sensor::readTemperature(result)) {
      return false;
    }
    return true;
  }

private:
  typedef LSM303D<I2CPeriph> Sensor;
  
};

//template<class I2CPeriph>
//int16_t HumiditySensor<I2CPeriph>::humidity;

