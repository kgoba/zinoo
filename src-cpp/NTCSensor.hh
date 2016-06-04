#include "FixedPoint.hh"

#include <i2c.hh>

class NTCSensor {
public:
  struct Config {
    // Regression coefficients
    Q11_4   r0;    // constant
    Q11_4   r1;    // K
    Q11_4   r2;    // K^2
    Q11_4   r3;    // K^3    
  };
  
  NTCSensor(Config const& config) : _config(config) {}
  
  bool update() {
    // read ADC, store temperature value
    
    word adcValue = 77;
    
    Q0_15 k(adcValue * 32);
    
    Q11_4 y = ((((_config.r3 * k) + _config.r2) * k + _config.r1) * k) + _config.r0;
    //_temp = y;
  }
  
  Q7_0 getTemperature() const {
    return _temp;
  }
  
private:
  Q7_0    _temp;
  
  Config  _config;
};


class AtmosphereModel {
public:
  AtmosphereModel() : mbarAtMSL(1000 * 16) 
  {
    
  }
  word getAltitude(word millibars) const
  {
    return 0;
  }
  
private:
  Q12_4 mbarAtMSL;
};


class BaroSensor;
class MagAccSensor;
//class HumiditySensor;
//class UVSensor;

template<class I2CPeriph = TWIMaster>
class UVSensor {
public:
  static bool measure() {
    word result;
    
    //Sensor::measure(result);
    // Calculate UV level
    
  }
  
  static word getUVLevel() {
    return uvLevel;
  }

private:
  //typedef HTU21D<I2CPeriph> Sensor;
  
  static word uvLevel;
};


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
    Device::send(&cmd, 1, I2CPeriph::kNoStop);    // will send repeated start
    return readResult(result);
  }
  
  static bool measureHumidity() {
    byte cmd = kMeasureHumidNoHold;
    Device::send(&cmd, 1, I2CPeriph::kNoStop);    // will send repeated start
    return true;
  }
  
  static bool measureHumidity(word &result) {
    byte cmd = kMeasureHumid;
    Device::send(&cmd, 1, I2CPeriph::kNoStop);    // will send repeated start
    return readResult(result);
  }
  
  static bool readResult(word &result) {
    byte data[3];
    Device::receive(data, ARRAY_SIZE(data));
    result = data[0];
    result = (result << 8) | data[1];
    byte chkSum = data[2];
    // TODO: check checksum
    return true;
  }
  
  static bool softReset() {
    byte cmd = kSoftReset;
    Device::send(&cmd, 1, I2CPeriph::kNoStop);    // will send repeated start
    return true;
  }
    
private:
  typedef I2CDevice<I2CPeriph, 0x40> Device;
  
  enum Commands {
    kMeasureTemp = 0xE3,
    kMeasureHumid = 0xE5,
    kMeasureTempNoHold = 0xF3,
    kMeasureHumidNoHold = 0xF5,
    kWriteUserReg = 0xE6,
    kReadUserReg = 0xE7,
    kSoftReset = 0xFE
  };
};


template<class I2CPeriph = TWIMaster>
class HumiditySensor {
public:
  static bool measure() {
    word result;
    
    Sensor::measureHumidity(result);
    // Calculate humidity as RH% x10 (1000 => 100%)
    result /= 64; // Convert to 10-bit value
    humidity = -60 + (39 * result) / 32;
    
    Sensor::measureTemperature(result);
    // Calculate temperature as degrees C x10 (100 => 10 C)
    result /= 64;
    humidity = -468 + (55 * result) / 32;
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
