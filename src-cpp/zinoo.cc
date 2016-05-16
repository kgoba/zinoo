#include <serial.hh>
//#include <stream.hh>

#include <queue.hh>
#include <timer.hh>
#include <pins.hh>

#include <avr/interrupt.h>
#include <util/delay.h>

#include "NTCSensor.hh"


class NMEAParser;
class UKHASBuilder;
class RTTYEncoder;
class FSKTransmitter;
class Manager;

const long baudrate  = 9600;
const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us


//PWMDual<Timer0> pwm0;
//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
//TextStream<Serial> serial;

class GPSBaudTimer {
public:
  static void setFrequency(long frequency) {
    Timer2::ClockSource source = Timer2::findClock(frequency);
    Timer2::setup(Timer2::eCTC, source);
    Timer2::setCompareA((frequency / Timer2::getPrescalerFactor(source)) - 1);
  }
  static void start() {
    Timer2::setCounter(0);
    Timer2::setOverflowInterrupt(true);
  }
  static void stop() {
    Timer2::setOverflowInterrupt(false);
  }
};


Queue<char, 32> rxQueue;

class GPSCallback {
public:
  static void onReceive(byte b, byte error) {
    if (!error) rxQueue.push(b);
  }
};

typedef SoftSerial<9600, ArduPin8, GPSBaudTimer, GPSCallback> GPSSerial;
typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;


GPSSerial gpsSerial;
Serial hwSerial;

ArduPin13 led;

NTCSensor::Config ntcConfig = {
  .r0 = 0,
  .r1 = 17,
  .r2 = 22,
  .r3 = 75
};


NTCSensor ntc(ntcConfig);


void setup()
{
  gpsSerial.setup();
  hwSerial.setup();
  
  //serial.enable();

  ntc.update();
  //pwm0.setup();

  for (byte i = 0; i < ntc.getTemperature().getValue(); i++) {
    
  led.set();
  _delay_ms(1000);
  led.clear();

  }
  
  //byte b = 72;  
  //serial.print("Decimal: ").print(b).eol();
  //serial.print("Binary : ").print(b, serial.eBinary).eol();
  //serial.print("Hex    : ").print(b, serial.eHex).eol();
  
  sei();
}


void loop()
{
  byte b;
  //if (serial.readByte(b)) {
  //  serial.writeByte(b);
  //}
}


/*
ISR(USART_RX_vect) {
  serial.onRXComplete();
}

ISR(USART_UDRE_vect) {
  serial.onTXReady();
}
*/

ISR(TIMER2_OVF_vect) {
  GPSSerial::onTimer();
}

ISR(PCINT0_vect) {
  GPSSerial::onPinChange();
}