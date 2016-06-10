#include "FixedPoint.hh"

#include <i2c.hh>

#include <util/delay.h>

template<class I2CPeriph, byte SA0 = 0>
class LSM303D {
public:

  static bool readTemperature(word &result) {
    return readWord(kRegTemp | 0x80, result);
  }
    
  static bool readMagnetometer(word &X, word &Y, word &Z) {
    return readTriple(kOutXLoMag | 0x80, X, Y, Z);
  }
  
  static bool readAccelerometer(word &X, word &Y, word &Z) {
    return readTriple(kOutXLoAcc | 0x80, X, Y, Z);
  }

  static bool getStatusMag(byte &status) {
    return readByte(kRegStatusMag, status);
  }
  
  static bool getStatusAcc(byte &status) {
    return readByte(kRegStatusMag, status);
  }
  
  static bool setCtrl0(byte value) {
    return writeByte(kRegCtrl0, value);
  }
  
  static bool setCtrl1(byte value) {
    return writeByte(kRegCtrl1, value);
  }
  
  static bool setCtrl2(byte value) {
    return writeByte(kRegCtrl2, value);
  }
  
  static bool setCtrl3(byte value) {
    return writeByte(kRegCtrl3, value);
  }
  
  static bool setCtrl4(byte value) {
    return writeByte(kRegCtrl4, value);
  }
  
  static bool setCtrl5(byte value) {
    return writeByte(kRegCtrl5, value);
  }
  
  static bool setCtrl6(byte value) {
    return writeByte(kRegCtrl6, value);
  }
  
  static bool setCtrl7(byte value) {
    return writeByte(kRegCtrl7, value);
  }

  static bool getCtrl5(byte &value) {
    return readByte(kRegCtrl5, value);
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
  
    static bool readWord(byte cmd, word &result) {
    if (!Device::send(&cmd, 1, I2CPeriph::kNoStop)) {
      return false;
    }
    byte data[2];
    if (!Device::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    result = ((word)data[1] << 8) | data[0];
    return true;
  }

  static bool readByte(byte cmd, byte &result) {
    if (!Device::send(&cmd, 1, I2CPeriph::kNoStop)) {
      return false;
    }
    byte data[2];
    if (!Device::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    result = ((word)data[1] << 8) | data[0];
    return true;
  }

  static bool writeByte(byte cmd, byte value) {
    byte data[2];
    data[0] = cmd;
    data[1] = value;
    if (!Device::send(data, 2)) {
      return false;
    }
    return true;
  }
  
  static bool readTriple(byte cmd, word &X, word &Y, word &Z) {
    if (!Device::send(&cmd, 1, I2CPeriph::kNoStop)) {
      return false;
    }
    byte data[6];
    if (!Device::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    X = ((word)data[1] << 8) | data[0];
    Y = ((word)data[3] << 8) | data[2];
    Z = ((word)data[5] << 8) | data[4];
    return true;
  }
};


template<class I2CPeriph = TWIMaster, byte SA0 = 0>
class MagAccSensor : public LSM303D<I2CPeriph, SA0> {
public:
  static bool initialize() {
    // byte ctrl0 = 0b00000000;    
    // Acceleration rate 12.5 Hz, enable XYZ axes
    byte ctrl1 = 0b00110111;    
    // Acceleration anti-alias filter 50 Hz, range +- 4 g
    byte ctrl2 = 0b11001000;    
    // Enable temperature sensor, high magnetic resolution, 
    // 3.125 Hz magnetic sensor sample rate
    byte ctrl5 = 0b11101000;    
    // Full scale +- 2 Gauss
    byte ctrl6 = 0b00000000;
    // Enable continuous magnetic sensor mode
    byte ctrl7 = 0b00000000;
    if (!Sensor::setCtrl1(ctrl1)) return false;
    if (!Sensor::setCtrl2(ctrl2)) return false;
    if (!Sensor::setCtrl5(ctrl5)) return false;
    if (!Sensor::setCtrl6(ctrl6)) return false;
    if (!Sensor::setCtrl7(ctrl7)) return false;
    return true;
  }
  
  static bool update() {
    word result, X, Y, Z;
    
    if (!Sensor::readMagnetometer(mag[0], mag[1], mag[2])) {
      return false;
    }

    if (!Sensor::readAccelerometer(acc[0], acc[1], acc[2])) {
      return false;
    }
    
    if (!Sensor::readTemperature(result)) {
      return false;
    }
    
    temperature = (25 << 3) + result;
    
    return true;
  }
  
  static bool getMagField(int16_t &X, int16_t &Y, int16_t &Z) {
    X = mag[0];
    Y = mag[1];
    Z = mag[2];
    return true;
  }

  static bool getAccel(int16_t &X, int16_t &Y, int16_t &Z) {
    X = acc[0];
    Y = acc[1];
    Z = acc[2];
    return true;
  }
    
  static int16_t getTemperature() {
    return temperature;
  }

private:
  typedef LSM303D<I2CPeriph, SA0> Sensor;
  
  static word   acc[3];      // signed 
  static word   mag[3];      // signed
  static word   temperature; // signed
};

template<class I2CPeriph, byte SA0>
word MagAccSensor<I2CPeriph, SA0>::acc[3];

template<class I2CPeriph, byte SA0>
word MagAccSensor<I2CPeriph, SA0>::mag[3];

template<class I2CPeriph, byte SA0>
word MagAccSensor<I2CPeriph, SA0>::temperature;
