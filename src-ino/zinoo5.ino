#include <PTLib.h>

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

ISR(TIMER2_OVF_vect) {
  GPSSerial::onTimer();
}

ISR(PCINT0_vect) {
  GPSSerial::onPinChange();
}

GPSSerial gpsSerial;

ArduPin13 led;

void setup() {
  // put your setup code here, to run once:

  while (!Serial1) { }
  Serial1.setup();

  gpsSerial.setup();
  sei();
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if (rxQueue.count() > 0) {
    byte b;
    rxQueue.pop(b);
    Serial1.putChar(b);
    led.set();
  }
  else {
    led.clear();
    delay(100);
  }
}

