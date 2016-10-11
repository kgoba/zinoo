#include <serial.hh>
#include <stream.hh>

const long debugBaudrate  = 9600;
const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us

//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
typedef BufferedSerial<SimpleSerial<debugBaudrate> > Serial;
//typedef SimpleSerial<baudrate> Serial;

typedef TextStream<Serial> SerialStream;


extern SerialStream debug;
