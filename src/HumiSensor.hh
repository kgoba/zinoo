#include "FixedPoint.hh"

#include <i2c.hh>

#include <util/delay.h>

template<class I2CPeriph>
class HTU21D {
public:
  static bool measureTemperature() {
    byte cmd = kMeasureTempNoHold;
    Device::send(&cmd, 1, I2CPeriph::kNoStop);    // will send repeated start
    return true;
  }

  static bool measureTemperature(word &result) {
    byte cmd = kMeasureTemp;
    // will send repeated start
    if (!Device::send(&cmd, 1, I2CPeriph::kNoStop)) {
      return false;
    }
    return readResult(result);
  }

  static bool measureHumidity() {
    byte cmd = kMeasureHumidNoHold;
    return Device::send(&cmd, 1, I2CPeriph::kNoStop);    // will send repeated start
  }

  static bool measureHumidity(word &result) {
    byte cmd = kMeasureHumid;
    // will send repeated start
    if (!Device::send(&cmd, 1, I2CPeriph::kNoStop)) {
      return false;
    }
    return readResult(result);
  }

  static bool readResult(word &result) {
    byte data[3];
    if (!Device::receive(data, ARRAY_SIZE(data))) {
      return false;
    }
    result = data[0];
    result = (result << 8) | data[1];
    byte chkSum = data[2];
    // TODO: check checksum
    return true;
  }

  static bool softReset() {
    byte cmd = kSoftReset;
    return Device::send(&cmd, 1);
  }

private:
  typedef I2CDevice<I2CPeriph, 0x40> Device;

  enum Commands {
    kMeasureTemp        = 0xE3,
    kMeasureHumid       = 0xE5,
    kMeasureTempNoHold  = 0xF3,
    kMeasureHumidNoHold = 0xF5,
    kWriteUserReg       = 0xE6,
    kReadUserReg        = 0xE7,
    kSoftReset          = 0xFE
  };
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
    word result;
    humidity = temperature = 0;

    if (!Sensor::measureHumidity(result)) {
      return false;
    }
    // Calculate humidity as RH% x10 (1000 => 100%)
    result /= 64; // Convert to 10-bit value
    humidity = -60 + (39 * result) / 32;

    if (!Sensor::measureTemperature(result)) {
      return false;
    }
    // Calculate temperature as degrees C x10 (100 => 10 C)
    result /= 32;
    temperature = (-937 + (55 * result) / 32) >> 1;
    return true;
  }
  static int16_t getHumidity() {
    return humidity;
  }
  static int16_t getTemperature() {
    return temperature;
  }

private:
  typedef HTU21D<I2CPeriph> Sensor;

  static int16_t humidity;
  static int16_t temperature;
};

template<class I2CPeriph>
int16_t HumiditySensor<I2CPeriph>::humidity;

template<class I2CPeriph>
int16_t HumiditySensor<I2CPeriph>::temperature;
