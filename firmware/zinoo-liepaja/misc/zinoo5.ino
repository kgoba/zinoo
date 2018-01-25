#include <PTLib.h>

#include "FixedPoint.h"

/*
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
class HumiditySensor;
class UVSensor;

class NMEAParser;
class UKHASBuilder;
class RTTYEncoder;
class FSKTransmitter;
class Manager;
  */

//Q12_4 test;




const long baudrate  = 9600;

const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us


//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
//TextStream<Serial> serial;

class SerialTimer {
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

class SerialCallback {
public:
  static void onReceive(byte b, byte error) {
    if (!error) rxQueue.push(b);
  }
};

typedef SoftSerial<9600, ArduPin8, SerialTimer, SerialCallback> GPSSerial;

GPSSerial gpsSerial;

ArduPin13 led;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  gpsSerial.setup();
  sei();
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if (rxQueue.count() > 0) {
    char b;
    rxQueue.pop(b);
    Serial.print(b);
    led.set();
  }
  else {
    led.clear();
    delay(100);
  }
}

ISR(TIMER2_OVF_vect) {
  GPSSerial::onTimer();
}

ISR(PCINT0_vect) {
  GPSSerial::onPinChange();
}