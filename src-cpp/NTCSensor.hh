#include "FixedPoint.hh"

#include <i2c.hh>

class NTCSensor {
public:
  struct Config {
    // Regression coefficients
    S11F4   r0;    // constant
    S11F4   r1;    // K
    S11F4   r2;    // K^2
    S11F4   r3;    // K^3    
  };
  
  NTCSensor(Config const& config) : _config(config) {}
  
  bool update() {
    // read ADC, store temperature value
    
    word adcValue = 77;
    
    S0F15 k(adcValue * 32);
    
    S11F4 y = ((((_config.r3 * k) + _config.r2) * k + _config.r1) * k) + _config.r0;
    //_temp = y;
  }
  
  S7F0 getTemperature() const {
    return _temp;
  }
  
private:
  S7F0    _temp;
  
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
  U12F4 mbarAtMSL;
};

// Atmospheric pressure table for altitudes 0 km .. 32 km 
// in steps of 1 km (multiply by 4 to get pascals)
// 
// 25331 22469 19874 17527 15410 13505 11795 10265 8900 7686 6609 5658 
// 4833 4128 3525 3011 2572 2197 1876 1603 1369 1169 1000 856 733 628 
// 538 462 397 341 293 252 217 