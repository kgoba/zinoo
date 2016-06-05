#include <serial.hh>
#include <stream.hh>

#include <queue.hh>
#include <timer.hh>
#include <pins.hh>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "NTCSensor.hh"
#include "HumiSensor.hh"
#include "Barometer.hh"
#include "UVSensor.hh"

#include "UKHAS.hh"

/******** PINOUT ********

Digital extension bus
   PD2     RADIO_EN
   PD3     FSK_CMOS
   PD4  
   PD5     GPS_TXO
   PD6
   PD7
Digital/Analog extension bus
   ADC0/PC0
   ADC1/PC1
   ADC2/PC2
   ADC3/PC3
Analog internal
   ADC6    ADC_UNREG
   ADC7    ADC_NTC 
Load switches
   PB0     RELEASE
   PB1     BUZZER (OC1A)
MicroSD Card
   PB2     SS_SD
   PB3     MOSI
   PB4     MISO
   PB5     SCK
I2C Bus
   PC4     SDA
   PC5     SCL

************************/


/********** I2C devices ************
  0x77    Barometer MS5607-02BA03
  0x40    Humidity sensor HTU21D
  0x38    UV sensor VEML6070
  0x39    UV sensor VEML6070
  
 *************************/

class NMEAParser;
//class UKHASBuilder;
class RTTYEncoder;
class FSKTransmitter;
class Manager;



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


/******* PERIPHERAL definitions ********/

const long baudrate  = 9600;
const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us

typedef SoftSerial<9600, ArduPin8, GPSBaudTimer, GPSCallback> GPSSerial;

//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
typedef BufferedSerial<SimpleSerial<baudrate> > Serial;
//typedef SimpleSerial<baudrate> Serial;

typedef TextStream<Serial> SerialStream;

typedef TWIMaster I2CBus;


/******* PERIPHERAL objects (for convenience) *******/

ArduPin13 led;

Serial hwSerial;
SerialStream debug;

GPSSerial gpsSerial;


const NTCSensor::Config ntcConfig = {
  .r0 = 0,
  .r1 = 17,
  .r2 = 22,
  .r3 = 75
};

NTCSensor ntc(ntcConfig);


class Sensors {
public:
  static void initialize() {
    baroSensor::initialize();
    humiSensor::initialize();
    uvSensor::initialize();
  }
  
  static void update() {
    baroSensor::update();  
    humiSensor::update();
    uvSensor::update();    
  }
  
  static uint16_t getPressure() {
    return baroSensor::getPressure();
  }

  static int16_t getTemperatureInt() {
    return baroSensor::getTemperature();
  }
  
  static int16_t getTemperature() {
    return humiSensor::getTemperature();
  }

  static int16_t getHumidity() {
    return humiSensor::getHumidity();
  }
  
  static uint16_t getUVLevel() {
    return uvSensor::getUVLevel();
  }
  
private:
  typedef Barometer<TWIMaster, 1>   baroSensor;
  typedef HumiditySensor<TWIMaster> humiSensor;
  typedef UVSensor<TWIMaster>       uvSensor;
};

Sensors sensors;

/*********** Hardware setup *************/

void setup()
{
  TWIMaster::setup();
	
  //gpsSerial.setup();
  hwSerial.setup();
  hwSerial.enable();

	/*
  ntc.update();
  for (byte i = 0; i < ntc.getTemperature().getValue(); i++) {    
    led.set();
    _delay_ms(1000);
    led.clear();
  }
  */
  
  //byte b = 72;  
  //serial.print("Decimal: ").print(b).eol();
  //serial.print("Binary : ").print(b, serial.eBinary).eol();
  //serial.print("Hex    : ").print(b, serial.eHex).eol();
  
  sei();
  
  _delay_ms(50);

  sensors.initialize();
}


/*********** Processing loop *************/

void loop()
{
  static word time = 0;
  //if (hwSerial.readByte(b)) {
  //  hwSerial.writeByte(b);
  //}  
  
  _delay_ms(1000);
  debug.print(time).tab();

  sensors.update();
  
  debug.print(sensors.getTemperatureInt()).tab().print(sensors.getPressure());
  debug.tab();
  debug.print(sensors.getTemperature()).tab().print(sensors.getHumidity());
  debug.tab();
  debug.print(sensors.getUVLevel());

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

ISR(TWI_vect) {
  TWIMaster::onTWIInterrupt();
}
