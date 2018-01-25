#include <Arduino.h>

#include <SdFat.h>

#include <pins.hh>
#include <i2c.hh>
#include <fstring.hh>

#include "UVSensor.hh"
#include "Barometer.hh"
#include "HumiSensor.hh"
#include "MagAccSensor.hh"
#include "Buzzer.hh"

#include "FSKTransmitter.hh"
#include "GPS.hh"
#include "UKHAS.hh"

#include "debug.hh"

#include "Config.hh"

const char *crlf = "\r\n";

DigitalOut<PortD, 2>  pinRadioEnable;
DigitalOut<PortD, 3>  pinRadioData;
DigitalOut<PortD, 4>  pinESPEnable;
DigitalIn<PortD, 5>   pinGPSData;
DigitalIn<PortD, 6>   pinD6;
DigitalIn<PortD, 7>   pinD7;

DigitalOut<PortB, 0>  pinRelease;
DigitalOut<PortB, 1>  pinBuzzer;

Barometer<TWIMaster, 1>     baroSensor;
MagAccSensor<TWIMaster, 1>  magSensor;
HumiditySensor<TWIMaster>   humiSensor;
UVSensor<TWIMaster>         uvSensor;
BusBuzzer<TWIMaster>        buzzer;

FSKTransmitter<DigitalOut<PortD, 3>, DigitalOut<PortD, 2> > fskTransmitter;

TWIMaster   i2cBus;

SdFat SD;
File logfile;

GPSParser gpsParser;

FlightData gFlightData;
UKHASPacketizer gPacketizer;

enum {
  kERROR_RESET,  // 1 if less than minute after Reset
  kERROR_GPSFIX, // 1 if GPS lock not found in 30 seconds
  kERROR_CARD,   // 1 if card not found
  kERROR_FILE,   // 1 if log file cannot be open
  kERROR_BARO,
  kERROR_MAG,
  kERROR_HUMID,
  kERROR_UVSENS
};

uint8_t gError;
bool    gRecoveryMode;

volatile uint16_t gSeconds;     // Seconds from reboot, 18h rollover

bool initSD();
void initTimer();
void initSensors();

void errorHalt(const char* msg)
{
  dbg.print("Error: ");
  dbg.println(msg);
  while(1);
}

void setup()
{
  dbg.begin(115200);
  dbg.println(F("Reset!"));

  i2cBus.begin(20000);  // Slow I2C

  initTimer();

  set_sleep_mode(SLEEP_MODE_IDLE);
  sei();

  bit_set(gError, kERROR_RESET);

  analogReference(INTERNAL);
  initSensors();

  dbg.println(F("Starting GPS serial..."));
  gpsBegin();

  if (!initSD()) {
    dbg << F("Could not access SD card!") << crlf;
    bit_set(gError, kERROR_CARD);
  }
  logfile = SD.open("LOG.TXT", FILE_WRITE);
  if (!logfile) {
    bit_set(gError, kERROR_FILE);
    dbg << F("Could not open file!") << crlf;
  }

  gPacketizer.setPayloadName(kPayloadName);

  dbg << F("Setup complete") << crlf;
  logfile.println("*** INIT ****");
}

void loop()
{
  // Check time after reset
  if (gSeconds >= 60) {
    bit_clear(gError, kERROR_RESET);
  }

  // Check GPS fix
  static uint16_t lastFixTime;
  if (gpsParser.gpsInfo.fix == '3') {
    lastFixTime = gSeconds;
    bit_clear(gError, kERROR_GPSFIX);
  }
  else if (gSeconds - lastFixTime > kGPSLockTimeout) {
    bit_set(gError, kERROR_GPSFIX);
  }

  int8_t minutes = gpsParser.gpsInfo.getMinutes();

  static bool espON = false;
  // start ESP every 10 minutes for 2 minutes
  if (minutes > 0 && (minutes % 10) < 2) {
    if (!espON) {
      dbg << F("Turning ESP on at ") << minutes << crlf;
      espON = true;
      pinESPEnable.set();
    }
  }
  else {
    if (espON) {
      dbg << F("Turning ESP off at ") << minutes << crlf;
      espON = false;
      pinESPEnable.clear();
    }
  }

  // Parse GPS serial buffer data
  while (gpsAvailable()) {
    char c = gpsRead();
    if (gpsParser.parse(c)) {
      gFlightData.updateGPS(gpsParser.gpsInfo);

      if (gpsParser.gpsInfo.fix != '3') {
        gRecoveryMode = true;
      }
      else {
        gRecoveryMode = gFlightData.altitude < 1000;
      }
    }
    //dbg.print(c);
  }
  //sleep_mode();
  //return;

  static FString<120> line;
  static uint16_t nextMeasureTime;

  if (gSeconds >= nextMeasureTime) {
    nextMeasureTime += kMeasureInterval;

    // Take measurement
    bool barOK = baroSensor.update();
    bool magOK = magSensor.update();
    bool humOK = humiSensor.update();
    bool uvOK  = uvSensor.update();

    // Read sensor results
    int16_t rhum = humiSensor.getHumidity();    // format D1
    int16_t temp = humiSensor.getTemperature(); // format D1

    uint16_t uvLevel = uvSensor.getUVLevel();

    uint16_t pressure = baroSensor.getPressure(); // Pascals /4
    uint16_t altitude = baroSensor.getAltitude(); // meters
    int16_t ptemp = baroSensor.getTemperature();        // format D2

    int16_t mtemp = magSensor.getTemperature();         // format F3
    int16_t mx, my, mz;                             // SF15 wrt +-2 Gauss
    int16_t ax, ay, az;                             // SF15 wrt +-4 g
    magSensor.getMagField(mx, my, mz);
    magSensor.getAccel(ax, ay, az);


    // Measure battery voltage
    int batteryVoltage=analogRead(A6);
    gFlightData.batteryVoltage=batteryVoltage;
    //volts=adcVal*0.0080929066;

    // Format logger line
    line.clear();
    line.append("$");
    line.append(kPayloadName);
    line.append(",");

    line.append(gpsParser.gpsInfo.time);
    line.append(',');
    line.append(gpsParser.gpsInfo.latitude);
    line.append(',');
    line.append(gpsParser.gpsInfo.longitude);
    line.append(',');
    line.append(gpsParser.gpsInfo.altitude);
    line.append(',');
    line.append(gpsParser.gpsInfo.satCount);
    line.append(',');

    if (humOK) line.append(rhum);
    line.append(',');
    if (humOK) line.append(temp);
    line.append(',');

    if (magOK) line.append(mx);
    line.append(',');
    if (magOK) line.append(my);
    line.append(',');
    if (magOK) line.append(mz);
    line.append(',');

    if (magOK) line.append(ax);
    line.append(',');
    if (magOK) line.append(ay);
    line.append(',');
    if (magOK) line.append(az);
    line.append(',');

    if (magOK) line.append(mtemp);
    line.append(',');

    if (barOK) line.append(pressure);
    line.append(',');
    if (barOK) line.append(ptemp);
    line.append(',');

    if (uvOK) line.append(uvLevel);
    line.append(',');

    line.append(batteryVoltage);
    line.append(',');

    line.append(gError, HEX);

    line.append(crlf);
    line.append('\0');

    logfile.print(line.buf);
    logfile.flush();    // Just to be sure

    dbg.print(line.buf);

    if (humOK) {
      dbg << F("RH: "); dbg.print(rhum * 0.1f, 1);
      dbg << F(" t: "); dbg.println(temp * 0.1f, 1);
      gFlightData.temperatureExternal = (temp + 5) / 10;
    }
    else {
      //dbg.println("Failed to read humidity sensor");
    }

    if (magOK) {
      dbg << F("a: ");
      dbg << ax << F(" / ") << ay << F(" / ") << az;
      dbg << F(" m: ");
      dbg << mx << F(" / ") << my << F(" / ") << mz;
      dbg << F(" t: ") << mtemp * 0.125f << crlf;
    }
    else {
      //dbg.println("Failed to read acceleration sensor");
    }

    if (barOK) {
      dbg << F("p: ") << (4UL * pressure);
      dbg << F(" h: ") << altitude;
      dbg << F(" t: ") << ptemp * 0.01f;
      dbg << crlf;
      gFlightData.temperatureInternal = (ptemp + 50) / 100;
      gFlightData.pressure = pressure;
      gFlightData.barometricAltitude = altitude;
    }
    else {
      //dbg.println("Failed to read pressure sensor");
    }

    if (uvOK) {
      dbg << F("UV: ") << uvLevel;
      dbg << crlf;
    }
    else {
      //dbg.println("Failed to read UV sensor");
    }

    dbg << F("ERR: 0b");
    dbg.println(gError, BIN);

    gFlightData.status = gError;

    /*
    if (gpsParser.gpsInfo.fix != 0) {
      dbg << "GPS Fix: " << (char)gpsParser.gpsInfo.fix << crlf;
    }
    dbg << "GPS: " << (const char *)gpsParser.gpsInfo.time
        << ' ' << (const char *)gpsParser.gpsInfo.latitude
        << ' ' << (const char *)gpsParser.gpsInfo.longitude << crlf;
    */
    //dbg << "Transmitting..." << crlf;
  }

  if (!fskTransmitter.isBusy()) {
    gPacketizer.makePacket(gFlightData);
    fskTransmitter.transmit((const uint8_t *)gPacketizer.packet.buf, gPacketizer.packet.size);
    dbg << gPacketizer.packet;
    //fskTransmitter.transmit((const uint8_t *)line.buf, line.size);
  }

  //delay(1000);
  sleep_mode();
}

ISR(TWI_vect) {
  i2cBus.onTWIInterrupt();
}

ISR(TIMER1_OVF_vect) {
  static uint16_t cnt2;

  if (++cnt2 >= kFSKBaudrate) {
    cnt2 = 0;
    gSeconds++;
  }


  static uint8_t errorIdx;      // index of the current error bit
  static uint8_t errorBeepCnt;
  static uint16_t errorBeepTicks;

  if (errorBeepTicks == 0) {
    if (errorBeepCnt == 0) {
      if (errorIdx == 8) {
        errorBeepCnt = 8;
        errorBeepTicks = kFSKBaudrate * 1;    // 1 second pause
      }
      else if (bit_check(gError, errorIdx)) {      // if error bit set
        errorBeepCnt = 2 * (errorIdx + 1);    // bit 0: 1 beep, bit 1: 2 beeps, etc
        errorBeepTicks = kFSKBaudrate * 1;    // 1 second pause
      }

      errorIdx++;
      if (errorIdx >= 9) {
        errorIdx = 0;
      }
      pinBuzzer.clear();
    }
    else {
      if (errorIdx > 0) {
        if (errorBeepCnt & 1) pinBuzzer.clear();
        else pinBuzzer.set();
      }
      else {
        pinBuzzer.set();
      }

      errorBeepTicks = kFSKBaudrate / 8;
      errorBeepCnt--;
    }
  }
  else {
    errorBeepTicks--;
  }

  fskTransmitter.tick();
}


/*
bool testFSWrite()
{
  if (SD.exists("OUT2.TXT")) {
    //dbg.println("Output file exists, overwriting...");
  }

  dbg.println("Starting write");
  uint32_t start = millis();

  logfile = SD.open("OUT2.TXT", FILE_WRITE);
  if (!logfile) {
    dbg.println("Could not open file!");
    return false;
  }
  for (byte idx = 0; idx < 100; idx++) {
    logfile.print("Time is: ");
    logfile.println(millis());
    dbg.print('.');
  }
  logfile.close();

  dbg.println("Written data to file");
  dbg.print("Took: "); dbg.println(millis() - start);
  return true;
}

bool testFSRead()
{
  if (!SD.exists("OUT2.TXT")) {
    dbg.println("File does not exist, skipping...");
    return false;
  }

  dbg.println("Starting read");
  uint32_t start = millis();

  logfile = SD.open("OUT2.TXT");
  if (!logfile) {
    dbg.println("Could not open file!");
    return false;
  }
  uint16_t nChars = 0, nLines = 0;
  while (logfile.available()) {
    char line[64];
    //nChars += logfile.fgets(line, sizeof(line));
    nChars += logfile.read(line, 64);
    //dbg.print(line);
    nLines++;
  }
  logfile.close();

  dbg.print("Read "); dbg.print(nLines); dbg.println(" lines from file");
  dbg.print("Read "); dbg.print(nChars); dbg.println(" bytes from file");
  dbg.print("Took: "); dbg.println(millis() - start);
  return true;
}

*/

void initTimer()
{
  // Timer1: Mode 14, Fast PWM, TOP = ICR1
  TCCR1A = _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12);
  TCCR1C = 0;
  const uint32_t clkDiv = (uint32_t)F_CPU / kFSKBaudrate;

  //dbg << "Timer1 divide = " << clkDiv << crlf;
  if (clkDiv < 65536) {
    TCCR1B |= 1;    // Prescaler 1024
    ICR1 = clkDiv - 1;
  }
  else if (clkDiv / 8 < 65536) {
    TCCR1B |= 2;
    ICR1 = clkDiv / 8 - 1;
  }
  else if (clkDiv / 64 < 65536) {
    TCCR1B |= 3;
    ICR1 = clkDiv / 64 - 1;
  }
  else if (clkDiv / 256 < 65536) {
    TCCR1B |= 4;
    ICR1 = clkDiv / 256 - 1;
  }
  else if (clkDiv / 1024 < 65536) {
    TCCR1B |= 5;
    ICR1 = clkDiv / 1024 - 1;
  }
  TIMSK1 = _BV(TOIE1);
}

void initSensors()
{
  bool success;

  dbg.print(F("Initializing pressure sensor..."));
  success = false;
  for (uint8_t nTry = 10; nTry > 0; nTry--) {
    if (baroSensor.begin()) {
      success = true;
      break;
    }
    delay(100);
  }
  if (!success) bit_set(gError, kERROR_BARO);
  dbg.println(success ? F("OK") : F("FAILED"));

  dbg.print(F("Initializing magnetic sensor..."));
  success = false;
  for (uint8_t nTry = 10; nTry > 0; nTry--) {
    if (magSensor.begin()) {
      success = true;
      break;
    }
    delay(100);
  }
  if (!success) bit_set(gError, kERROR_MAG);
  dbg.println(success ? F("OK") : F("FAILED"));

  dbg.print(F("Initializing humidity sensor..."));
  success = false;
  for (uint8_t nTry = 10; nTry > 0; nTry--) {
    if (humiSensor.begin()) {
      success = true;
      break;
    }
    delay(100);
  }
  if (!success) bit_set(gError, kERROR_HUMID);
  dbg.println(success ? F("OK") : F("FAILED"));

  dbg.print(F("Initializing UV sensor..."));
  success = false;
  for (uint8_t nTry = 10; nTry > 0; nTry--) {
    if (uvSensor.begin()) {
      success = true;
      break;
    }
    delay(100);
  }
  if (!success) bit_set(gError, kERROR_UVSENS);
  dbg.println(success ? F("OK") : F("FAILED"));
}

bool initSD()
{
  dbg << F("Initializing SD card...") << crlf;

  if (!SD.begin()) {
    if (SD.card()->errorCode()) {
      dbg.print(F("Error code: 0x"));
      dbg.println(SD.card()->errorCode(), HEX);
      dbg.print(F("Error data: 0x"));
      dbg.println(SD.card()->errorData(), HEX);
    }
    dbg.println(F("Cannot communicate with SD card"));
    return false;
  }

  if (SD.vol()->fatType() == 0) {
    dbg.println(F("Can't find a valid FAT16/FAT32 partition."));
    return false;
  }

  //uint32_t size = SD.card()->cardSize();
  //uint32_t sizeMB = 0.000512 * size + 0.5;
  //dbg.print("SD card size, blocks: "); dbg.println(size);
  //dbg.print("SD card size, MB: "); dbg.println(sizeMB);
  //dbg.print("Cluster size, bytes: "); dbg.println(512L * SD.vol()->blocksPerCluster());
  return true;
}
