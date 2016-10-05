#include <serial.hh>
#include <stream.hh>

#include <queue.hh>
#include <timer.hh>
#include <pins.hh>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "NTCSensor.hh"
#include "MagAccSensor.hh"
#include "HumiSensor.hh"
#include "Barometer.hh"
#include "UVSensor.hh"

#include "UKHAS.hh"


class NMEAParser;
//class UKHASBuilder;
class RTTYEncoder;
class FSKTransmitter;
class Manager;


/*
  initHardware();
  startConsole();

  restoreState();

  startTask(updateScienceData, 1000);
  startTask(updateFlightData, 5000);
  startTask(transmitData, 1000);
  startTask(recordData, 5000);
  startTask(saveState, 60000);
  startTask(manageBuzzer, 1000);
  startTask(consoleTask, 500);
*/


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
    magAccSensor::initialize();
    baroSensor::initialize();
    humiSensor::initialize();
    uvSensor::initialize();
  }

  static void update() {
    magAccSensor::update();
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

  static int16_t getMagField(int16_t X, int16_t Y, int16_t Z) {
    return magAccSensor::getMagField(X, Y, Z);
  }

  static bool getAccel(int16_t X, int16_t Y, int16_t Z) {
    return magAccSensor::getAccel(X, Y, Z);
  }

  static int16_t getMagTemperature() {
    return magAccSensor::getTemperature();
  }

private:
  typedef MagAccSensor<TWIMaster, 1>  magAccSensor;
  typedef Barometer<TWIMaster, 1>     baroSensor;
  typedef HumiditySensor<TWIMaster>   humiSensor;
  typedef UVSensor<TWIMaster>         uvSensor;
};

Sensors sensors;

void I2CDetect() {
	for (byte addr = 8; addr < 126; addr++) {
		byte data;
		TWIMaster::write(addr, &data, 0);

		bool success = false;

		for (byte nTry = 0; nTry < 10; nTry++) {
		  _delay_ms(5);
		  if (TWIMaster::isReady()) {
			  success = !TWIMaster::isError();
			  break;
		  }
		}

		if (success) {
		  debug.print(addr, debug.eHex).eol();
		}

    _delay_ms(1);
	}
}

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

  _delay_ms(100);
  int16_t mx, my, mz;
  if (sensors.getMagField(mx, my, mz)) {
    debug.print(mx).tab();
    debug.print(my).tab();
    debug.print(mz).tab();
    debug.print(sensors.getMagTemperature() * 10 / 8);
    debug.eol();
  }

  _delay_ms(100);
  int16_t ax, ay, az;
  if (sensors.getAccel(mx, my, mz)) {
    debug.print(ax).tab();
    debug.print(ay).tab();
    debug.print(az).tab();
    debug.eol();
  }

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
