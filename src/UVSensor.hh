#include <fpoint.hh>
#include <i2c.hh>

#include <util/delay.h>

template<class I2CPeriph = TWIMaster>
class VEML6070 {
public:
  static bool readMSB(byte &msb) {
    byte data[1];
    if (!Device2::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    msb = data[0];
    return true;
  }

  static bool readLSB(byte &lsb) {
    byte data[1];
    if (!Device::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    lsb = data[0];
    return true;
  }

private:
  typedef I2CDevice<I2CPeriph, 0x38> Device;
  typedef I2CDevice<I2CPeriph, 0x39> Device2;
};

template<class I2CPeriph = TWIMaster>
class UVSensor : public VEML6070<I2CPeriph> {
public:
  static bool begin() {
    return update();
  }

  static bool update() {
    uvLevel = 0;

    byte msb, lsb;

    if (!Sensor::readMSB(msb)) {
      return false;
    }
    _delay_ms(5);
    if (!Sensor::readLSB(lsb)) {
      return false;
    }

    // Calculate UV level
    uvLevel = ((word)msb << 8) | lsb;
    return true;
  }

  static word getUVLevel() {
    return uvLevel;
  }

private:
  typedef VEML6070<I2CPeriph> Sensor;

  static word uvLevel;
};

template<class I2CPeriph>
word UVSensor<I2CPeriph>::uvLevel;
