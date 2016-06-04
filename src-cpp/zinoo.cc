#include <serial.hh>
#include <stream.hh>

#include <queue.hh>
#include <timer.hh>
#include <pins.hh>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "NTCSensor.hh"

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

class NMEAParser;
//class UKHASBuilder;
class RTTYEncoder;
class FSKTransmitter;
class Manager;

const long baudrate  = 9600;
const word timeoutRX = 500;     // in multiples of 10 us
const word timeoutTX = 500;     // in multiples of 10 us

/*
uint16_t const ccitt_crc16_table[256] PROGMEM = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
*/

// CRC-16 CCITT (x^16+x^12+x^5+1 or 0x1021)
class CRC16CCITT {
public:
  CRC16CCITT() {
    reset();
  }
  
  void reset() {
    crc = 0xFFFF;
  }
  
  void update(uint8_t x) {
    uint16_t crc_new = (uint8_t)(crc >> 8) | (crc << 8);
    crc_new ^= x;
    crc_new ^= (crc_new & 0xff) >> 4;
    crc_new ^= crc_new << 12;
    crc_new ^= (crc_new & 0xff) << 5;
    crc = crc_new;
  }
  
private:
  uint16_t crc;
};

template<byte fractionBits>
void fixedPointToDecimal(int16_t fp, bool skipTrailingZeros) {
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

//class String;
typedef char *  String;

template<class SentenceClass>
struct SentenceData {
  SentenceData() {
    sentenceID = 0;
    altitude = 0;
    ascentRate = 0;
    groundSpeed = 0;
    satellites = 0;
    fixQuality = '0';
    flightMode = 'I';
    temperatureExternal = -100;
    temperatureBattery = -100;
    voltageBattery = -100;
  }

  // Bare minimum
  int     sentenceID;
  String  time;
  String  latitude;
  String  longitude;
  int     altitude;
  
  // Good to have
  int     ascentRate;
  int     groundSpeed;
  char    fixQuality;     // 0 - no fix, 2 - 2D fix, 3 - 3D fix
  int     satellites;
  char    flightMode;     // (I)nitialization, (R)eady, (A)scent, (D)escent, (T)ouchdown
  
  // Environment information
  int     temperatureExternal;
  int     temperatureBattery;
  int     voltageBattery;
  
  // Scientific payload (very optional)
  // int  uvLevel;
  // int  humidity;
  // int  pressure;
  // int  barometricAltitude;
  // int  magneticField;
  // int  acceleration;
  // int  temperatureInternal;
  
  void buildSentence(SentenceClass &s) {
    s.addIntField(sentenceID);
    s.addStringField(time);
    s.addStringField(latitude);
    s.addStringField(longitude);
    s.addIntField(altitude);
    s.finish();
    sentenceID++;
  }
};

template<int maxLength = 80>
class UKHASSentence {
public:
  UKHASSentence(const char *payloadName) {
    length = 0;
    append("$$");
    append(payloadName);
  }
  
  void addStringField(const char *data) {
    append(data);
    append(',');
  }
  
  void finish() {
    append('*');
    // add 16 bit CRC as a hex number
  }
  
  const char * getString() {
    return buffer;
  }
  
private:
  char buffer[maxLength];
  byte length;
  CRC16CCITT crc;    // CRC-16 CCITT (x^16+x^12+x^5+1 or 0x1021)

  void append(const char *str) {
    while (*str) {
      append(*str++);
    }
  }
  
  void append(char c) {
    crc.update(c);
  }
};

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
//typedef WaitableSerial<BufferedSerial<SimpleSerial<baudrate> >, timeoutRX, timeoutTX> Serial;
typedef BufferedSerial<SimpleSerial<baudrate> > Serial;
//typedef SimpleSerial<baudrate> Serial;
typedef TextStream<Serial> SerialStream;
typedef TWIMaster I2CBus;

GPSSerial gpsSerial;
Serial hwSerial;
SerialStream debug;

ArduPin13 led;

NTCSensor::Config ntcConfig = {
  .r0 = 0,
  .r1 = 17,
  .r2 = 22,
  .r3 = 75
};


NTCSensor ntc(ntcConfig);
HumiditySensor<I2CBus> humidSensor;

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
}


void loop()
{
  static byte b = 0;
  //if (hwSerial.readByte(b)) {
  //  hwSerial.writeByte(b);
  //}  
  
  
  //for (byte i = 0; i < 10; i++) 
  _delay_ms(100);
  
  //humidSensor.measure();
  int16_t temp = humidSensor.getTemperature();
  int16_t humi = humidSensor.getHumidity();
  debug.print(temp).print(" ").print(humi).eol();
  
  
  UKHASSentence<> sentence("Zinoo5");
}

ISR(USART_RX_vect) {
  hwSerial.onRXComplete();
}

ISR(USART_UDRE_vect) {
  hwSerial.onTXReady();
}

ISR(TIMER2_OVF_vect) {
  GPSSerial::onTimer();
}

ISR(PCINT0_vect) {
  GPSSerial::onPinChange();
}
