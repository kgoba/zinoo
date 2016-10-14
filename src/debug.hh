#pragma once
//#include <serial.hh>
//#include <stream.hh>

#include <MinimumSerial.h>

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
