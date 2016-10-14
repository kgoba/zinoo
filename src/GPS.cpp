#include "GPS.hh"
#include "pins.hh"

#include "debug.hh"

const int gpsBaudrate = 9600;

DigitalIn<PortD, 5>   pinGPSRX;       // Hardware RX pin


GPSInfo::GPSInfo() {
  clear();
}

void GPSInfo::clear() {
  altitude[0] = latitude[0] = longitude[0] = time[0] = 0;
  fix = 0;
  satCount = 0;
}

void GPSInfo::setFix(char fix) {
  if (this->fix == fix) return;
  //dbg << "Fix: " << fix << "\r\n";
  this->fix = fix;
}

void GPSInfo::setTime(const char *time) {
  //dbg << "Time: " << time << "\r\n";
  strncpy(this->time, time, 12);
}

void GPSInfo::setLatitude(const char *latitude) {
  //dbg << "Lat: " << latitude << "\r\n";
  strncpy(this->latitude, latitude, 12);
}

void GPSInfo::setLongitude(const char *longitude) {
  if (*longitude == '0') longitude++;
  //dbg << "Lng: " << longitude << "\r\n";
  strncpy(this->longitude, longitude, 12);
}

void GPSInfo::setAltitude(const char *altitude) {
  //dbg << "Alt: " << altitude << "\r\n";
  strncpy(this->altitude, altitude, 8);
}

void GPSInfo::setSatCount(const char *satCount) {
  //dbg << "Sat: " << satCount << "\r\n";
  this->satCount = atoi(satCount);
}

/*
void GPSInfo::print() {
  dbg.print(F("GPS:"));
  if (fix) {
    Serial.print(F(" Fix: "));
    Serial.print(fix);
  }
  if (time[0]) {
    Serial.print(F(" Time: "));
    Serial.print(time);
  }
  if (latitude[0]) {
    Serial.print(F(" Lat: "));
    Serial.print(latitude);
  }
  if (longitude[0]) {
    Serial.print(F(" Lon: "));
    Serial.print(longitude);
  }
  if (altitude[0]) {
    Serial.print(F(" Alt: "));
    Serial.print(altitude);
  }
  if (satCount > 0) {
    Serial.print(F(" Sat: "));
    Serial.print(satCount);
  }
  Serial.println();
}
*/

void GPSParser::parse(char c) {
  // parseByChar(c);


  static char    line[100];
  static uint8_t lineLength;
  static uint8_t checksum;
  static uint8_t checksumReported;
  static uint8_t checksumState;

  if (c == '\n') {

  }
  else if (c != '\r') {
    if (checksumState == 2) {
      uint8_t digit = (c >= 'A') ? (c - 'A' + 10) : (c - '0');
      checksumReported <<= 4;
      checksumReported |= digit;
    }
    if (c == '*') {
      checksumState = 2;
    }
    if (checksumState == 1) {
      checksum ^= (uint8_t) c;
    }
    if (c == '$') {
      checksumState = 1;
    }
    if (lineLength < 99) {
      line[lineLength++] = c;
    }
  }
  else {

    // parse entire line
    if (lineLength > 0) {
      line[lineLength] = 0;
      if (checksum != checksumReported) {
        //Serial.print("ERROR: "); Serial.print(checksum, HEX); Serial.print(" ");
        //Serial.println(line);
        //Serial.println("E: chk");
      }
      else {
        char *ptr = line;
        while (lineLength > 0) {
          parseByChar(*ptr);
          ptr++;
          lineLength--;
        }
        parseByChar('\n');
      }
    }

    checksum = 0;
    checksumReported = 0;
    checksumState = 0;
    lineLength = 0;
  }

}

void GPSParser::parseByChar(char c) {
  static SentenceType sentence;
  static char fieldValue[13];
  static byte fieldLength;
  static byte fieldIndex;

  //if (sentence == SENTENCE_GGA)  Serial.print(c);

  if (c == ',' || c == '*') {
    //if (sentence == SENTENCE_GGA) Serial.print(' ');
    // Process field
    if (fieldIndex == 0) {
      if (memcmp(fieldValue, "$GPGSA", 6) == 0) {
        sentence = SENTENCE_GSA;
      }
      else if (memcmp(fieldValue, "$GPGGA", 6) == 0) {
        sentence = SENTENCE_GGA;
      }
      //else if (memcmp(fieldValue, "$GPRMC", 6) == 0) {
      //  sentence = SENTENCE_RMC;
      //}
    }
    // Add terminating zero
    fieldValue[fieldLength] = 0;
    processField(sentence, fieldIndex, fieldValue, fieldLength);
    fieldIndex++;
    fieldLength = 0;
  }
  else if (c == '\n') {
    if (sentence == SENTENCE_GGA) {
      //gpsInfo.print();
    }
    //state = INIT;
    sentence = SENTENCE_NONE;
    fieldIndex = 0;
    fieldLength = 0;
  }
  else {
    if (fieldLength < 12) {
      fieldValue[fieldLength++] = c;
    }
  }
}

/*
 GGA - essential fix data which provide 3D location and accuracy data.

 $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47

Where:
     GGA          Global Positioning System Fix Data
     123519       Fix taken at 12:35:19 UTC
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     1            Fix quality: 0 = invalid
                               1 = GPS fix (SPS)
                               2 = DGPS fix
                               3 = PPS fix
            4 = Real Time Kinematic
             5 = Float RTK
                               6 = estimated (dead reckoning) (2.3 feature)
             7 = Manual input mode
             8 = Simulation mode
     08           Number of satellites being tracked
     0.9          Horizontal dilution of position
     545.4,M      Altitude, Meters, above mean sea level
     46.9,M       Height of geoid (mean sea level) above WGS84
                      ellipsoid
     (empty field) time in seconds since last DGPS update
     (empty field) DGPS station ID number
     *47          the checksum data, always begins with *

 */

void GPSParser::processField (byte sentence, byte index, const char *value, byte length) {
  switch (sentence) {
    case SENTENCE_GSA:
      switch (index) {
        case 2: gpsInfo.setFix(value[0]); break;
      }
      break;
    case SENTENCE_GGA:
      switch (index) {
        case 1: gpsInfo.setTime(value); break;
        case 2: gpsInfo.setLatitude(value); break;
        case 4: gpsInfo.setLongitude(value); break;
        case 7: gpsInfo.setSatCount(value); break;
        case 9: gpsInfo.setAltitude(value); break;
      }
      break;
    case SENTENCE_RMC:
      switch (index) {
        case 1: gpsInfo.setTime(value); break;
        case 3: gpsInfo.setLatitude(value); break;
        case 5: gpsInfo.setLongitude(value); break;
      }
      break;
    default:
      break;
  }
}


#ifdef HARDWARE_SERIAL

void gpsBegin() {
  Serial1.begin(gpsBaudrate);
}

byte gpsAvailable() {
  return Serial1.available();
}

byte gpsRead() {
  return Serial1.read();
}

#else

#define RXBUFFER_SIZE   32
volatile byte rxBuff[RXBUFFER_SIZE];
volatile byte rxBuffHead;
volatile byte rxBuffSize;

volatile byte rxBitIdx;
volatile bool rxError;
volatile byte rxData;

void gpsReceive(byte data) {
  if (rxBuffSize >= RXBUFFER_SIZE) return;

  byte tail = (rxBuffHead + rxBuffSize) % RXBUFFER_SIZE;
  rxBuff[tail] = data;
  rxBuffSize++;
}

byte gpsAvailable() {
  return rxBuffSize;
}

byte gpsRead() {
  uint8_t sreg_save = SREG;
  cli();

  byte data = rxBuff[rxBuffHead];
  if (rxBuffSize > 0) {
    rxBuffHead = (rxBuffHead + 1) % RXBUFFER_SIZE;
    rxBuffSize--;
  }

  SREG = sreg_save;
  return data;
}

#if defined (__AVR_ATmega328P__)

#define BAUDRATE_PERIOD  (F_CPU / 8 / gpsBaudrate)

//#define GPS_PCINT_SETUP   PCICR |= _BV(PCIE2)
//#define GPS_PCINT_ENABLE  PCMSK2 |= _BV(PCINT21)
//#define GPS_PCINT_DISABLE PCMSK2 &= ~_BV(PCINT21)
#define GPS_TIMER_ENABLE  TIMSK2 |= _BV(TOIE2)
#define GPS_TIMER_DISABLE TIMSK2 &= ~_BV(TOIE2)

void gpsBegin() {
  byte mode = 7;
  byte presc = 2;     // CK/8
  TCCR2A = (mode & 3);
  TCCR2B = ((mode & 12) << 1) | presc;
  OCR2A   = BAUDRATE_PERIOD - 1;

  // Enable pullup on RX
  //pinGPSRX.mode(IOMode::InputPullup);
  pinGPSRX.set();

  // Enable pin change interrupt
  //GPS_PCINT_SETUP;
  pinGPSRX.enablePCInterrupt();

  //GPS_PCINT_ENABLE;
  pinGPSRX.setPCMask();
}

ISR(TIMER2_OVF_vect) {
  byte state = pinGPSRX.read();

  if (rxBitIdx == 10) {
    // Disable timer interrupt
    GPS_TIMER_DISABLE;
    // Transmission complete, check stop bit
    if (state == 0) {
      rxError = true;
    }
    if (!rxError) {
      gpsReceive(rxData);
    }
    // Enable pin change interrupt
    //GPS_PCINT_ENABLE;
    pinGPSRX.setPCMask();
  }
  else if (rxBitIdx == 0) {
      // Check start bit
      if (state != 0) {
        rxError = true;
      }
  }
  else {
      // Shift in bit (LSB first)
      rxData >>= 1;
      if (state) rxData |= 0x80;
  }
  // Next bit
  rxBitIdx++;
}

ISR(PCINT2_vect) {
  byte state = pinGPSRX.read();

  // Check for falling edge
  if (state == 0) {
    // Disable pin change interrupt
    //GPS_PCINT_DISABLE;
    pinGPSRX.clearPCMask();

    rxBitIdx = 0;
    rxData = 0;
    rxError = false;

    // Set up half the bit period
    TCNT2 = BAUDRATE_PERIOD / 2;
    // Enable timer interrupt
    GPS_TIMER_ENABLE;
  }
}

#endif

#endif
