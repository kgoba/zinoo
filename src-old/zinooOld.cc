#include <serial.hh>
#include <stream.hh>

#include <queue.hh>
#include <timer.hh>
#include <pins.hh>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "debug.hh"
#include "UKHAS.hh"

#ifndef ZINOO_NEW

class GPSBaudTimer {
public:
  static void setFrequency(long frequency) {
        TIMER2_SETUP(TIMER2_FAST_PWM_A, TIMER2_PRESCALER(9600));
        OCR2A = TIMER2_COUNTS(9600);
    //Timer2::ClockSource source = Timer2::findClock(frequency);
    //Timer2::setup(Timer2::eCTC, source);
    //Timer2::setCompareA((frequency / Timer2::getPrescalerFactor(source)) - 1);
  }
  static void startFullTimeout() {
      TCNT0 = 0;
      bit_set(TIMSK2, TOIE2);
      //Timer2::setCounter(0);
        //Timer2::setOverflowInterrupt(true);
  }
  static void startHalfTimeout() {
      TCNT0 = OCR2A / 2;
      bit_set(TIMSK2, TOIE2);
    //Timer2::setCounter(0);
    //Timer2::setOverflowInterrupt(true);
  }
  static void stop() {
      bit_clear(TIMSK2, TOIE2);
    //Timer2::setOverflowInterrupt(false);
  }
};

Queue<char, 32> rxQueue;

class GPSCallback {
public:
  static void onReceive(byte b, byte error) {
    if (!error) {
        rxQueue.push(b);
        debug.print(b);
    }
  }
};

/******* PERIPHERAL definitions ********/

typedef SoftSerialReceiver<9600, ArduPin8, GPSBaudTimer, GPSCallback> GPSSerial;


/******* PERIPHERAL objects (for convenience) *******/

ArduPin13 led;

GPSSerial gpsSerial;


struct SoftUART {
    byte rxBitIndex;
    byte rxData;
};

ArduPin8 gpsRxPin;
SoftUART gpsRx;

/*********** Hardware setup *************/

void setup()
{
  GPSSerial::setup();
  GPSSerial::enable();

  Serial::setup();
  Serial::enable();
  
  sei();
  
  gpsRxPin.enablePCInterrupt();

  gpsRx.rxBitIndex = 0;
  gpsRxPin.setPCMask();
}


/*********** Processing loop *************/

void loop()
{
  static word time = 0;
  //if (hwSerial.readByte(b)) {
  //  hwSerial.writeByte(b);
  //}  
  
  _delay_ms(1000);

  debug.print("Test: ");
  debug.print(time);
  debug.eol();
  
  //TWIMaster::TWISendStop();
  //UKHASSentence<> sentence("Zinoo5");
  time++;
}


/*********** Interrupt service routines *************/

ISR(USART_RX_vect) {
  Serial::onRXComplete();
}

ISR(USART_UDRE_vect) {
  Serial::onTXReady();
}

ISR(TIMER2_OVF_vect) {
    GPSSerial::onTimer();
}

ISR(PCINT0_vect) {
    GPSSerial::onPinChange();
}

#endif