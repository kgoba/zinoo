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

