//#include <serial.hh>
//#include <stream.hh>
#include <avr/interrupt.h>

#include <timer.hh>
#include <pins.hh>
#include <util/delay.h>

const long baudrate  = 9600;
const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us


//PWMDual<Timer0> pwm0;
//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
//TextStream<Serial> serial;

class SerialTimer {
public:
  static void setFrequency(long frequency) {
    Timer0::ClockSource source = Timer0::findClock(frequency);
    Timer0::setup(Timer0::eCTC, source);
    Timer0::setCompareA((frequency / Timer0::getPrescalerFactor(source)) - 1);
  }
  static void start() {
    Timer0::setCounter(0);
    Timer0::setOverflowInterrupt(true);
  }
  static void stop() {
    Timer0::setOverflowInterrupt(false);
  }
};

Queue<char, 32> rxQueue;

class SerialCallback {
public:
  static void onReceive(byte b, byte error) {
    
  }
};

PortB::pin0 led1;

typedef SoftSerial<9600, ArduPin8, SerialTimer, SerialCallback> Serial;

ISR(TIMER0_OVF_vect) {
  Serial::onTimer();
}

ISR(PCINT0_vect) {
  Serial::onPinChange();
}

Serial serial;

int main()
{
  serial.setup();
  //serial.enable();

  //pwm0.setup();

  led1.set();
  _delay_ms(1000);
  led1.clear();

  //pwm0.setPWMA(23);
  //pwm0.setPWMB(24);

  byte b = 72;
  
  //serial.print("Decimal: ").print(b).eol();
  //serial.print("Binary : ").print(b, serial.eBinary).eol();
  //serial.print("Hex    : ").print(b, serial.eHex).eol();
    
  while (true) {
    byte b;
    //if (serial.readByte(b)) {
    //  serial.writeByte(b);
    //}
  }
}

/*
ISR(USART_RX_vect) {
  serial.onRXComplete();
}

ISR(USART_UDRE_vect) {
  serial.onTXReady();
}
*/