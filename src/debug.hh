#pragma once
//#include <serial.hh>
//#include <stream.hh>

#include <MinimumSerial.h>
#include "FixedPoint.hh"

#include "fstring.h"


extern MinimumSerial dbg;


template<typename T>
MinimumSerial & operator << (MinimumSerial &str, const T &x) {
  str.print(x);
  return str;
}

template<byte capacity>
MinimumSerial & operator << (MinimumSerial &str, const FString<capacity> &x) {
  for (uint8_t idx = 0; idx < x.size; idx++) {
    str.print(x.buf[idx]);
  }
  return str;
}

template<typename T2, int fractionBits>
MinimumSerial & operator << (MinimumSerial &str, const FixedPoint<uint16_t, T2, fractionBits> &x) {
  // Determine maximum decimal digit value
  int16_t digitValue = (1000 << fractionBits);
  uint16_t fp = x.getValue();
  bool skipTrailingZeros = true;

  // Process integer part
  while (digitValue > (1 << fractionBits)) {
    uint8_t nextDigitValue = digitValue / 10;
    if (nextDigitValue < (1 << fractionBits)) {
      skipTrailingZeros = false;
    }
    uint8_t d = fp / digitValue;
    if (!skipTrailingZeros || d != 0) {
      // OUTPUT d + '0'
      str.print((char)(d + '0'));
    }
    if (d != 0) {
      fp -= d * digitValue;
      skipTrailingZeros = false;
    }
    digitValue = nextDigitValue;
  }
  // Process fractional part
  str.print('.');
  while (digitValue > 0) {
    fp *= 10;
    uint8_t d = (fp >> fractionBits);
    // OUTPUT d + '0'
    str.print((char)(d + '0'));
    fp -= d << fractionBits;
    digitValue /= 10;
  }
  return str;
}

/*
class FloatFmt {
public:
  FloatFmt(float f, uint8_t fractDigits) : _f(f), _n(fractDigits)
  {}

  float _f;
  uint8_t _n;
};

MinimumSerial & operator << (MinimumSerial &str, const FloatFmt &x) {
  str.print(x._f, x._n);
  return str;
}
*/

/*
const long debugBaudrate  = 9600;
const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us

//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
typedef BufferedSerial<SimpleSerial<debugBaudrate> > Serial;
//typedef SimpleSerial<baudrate> Serial;

typedef TextStream<Serial> SerialStream;


extern SerialStream debug;
*/
