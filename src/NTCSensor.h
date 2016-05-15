#include "FixedPoint.h"

class NTCSensor {
public:
  bool update() {
    // read ADC, store temperature value
    
    Q0_15 k(adcValue * 32);
    
    Q11_4 y = (((_r3 * k) + _r2) * k + _r1) * k) + _r0;
    _temp = y;
  }
  
  Q7_0 getTemperature() const {
    return _temp;
  }
  
private:
  Q7_0    _temp;
  
  // Regression coefficients
  Q11_4   _r0;    // constant
  Q11_4   _r1;    // K
  Q11_4   _r2;    // K^2
  Q11_4   _r3;    // K^3
};
