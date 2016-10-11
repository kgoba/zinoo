#include <stdint.h>

#include <types.hh>

void I2CDetect();

template<byte fractionBits>
void fixedPointToDecimal(int16_t fp, bool skipTrailingZeros = true) {
  // Determine maximum decimal digit value
  int16_t digitValue = (1000 << fractionBits);
  
  // Process integer part
  while (digitValue > (1 << fractionBits)) {
    byte nextDigitValue = digitValue / 10;
    if (nextDigitValue < (1 << fractionBits)) {
      skipTrailingZeros = false;
    }
    byte d = fp / digitValue;
    if (!skipTrailingZeros || d != 0) {
      // OUTPUT d + '0'
    }
    if (d != 0) {
      fp -= d * digitValue;
      skipTrailingZeros = false;
    }
    digitValue = nextDigitValue;
  }
  // Process fractional part
  // OUTPUT '.'
  while (digitValue > 0) {
    fp *= 10;
    byte d = (fp >> fractionBits);
    // OUTPUT d + '0'
    fp -= (int16_t)d << fractionBits;
    digitValue /= 10;
  }
}
