#include <Arduino.h>

#include <MinimumSerial.h>
#include <SdFat.h>

#include <pins.hh>
#include <i2c.hh>

#include "HumiSensor.hh"

template<class I2CPeriph>
class Buzzer {
public:
  static bool setMode(uint8_t mode) {
    uint8_t cmd[2];
    cmd[0] = 0x00;
    cmd[1] = mode;
    return Device::send(cmd, 2);
  }

private:
  typedef I2CDevice<I2CPeriph, 0x23> Device;
};



DigitalIn<PortD, 4>   pinD4;
DigitalIn<PortD, 5>   pinGPSData;
DigitalIn<PortD, 6>   pinD6;
DigitalIn<PortD, 7>   pinD7;

DigitalOut<PortD, 2>  pinRadioEnable;
DigitalOut<PortD, 3>  pinRadioData;
DigitalOut<PortB, 0>  pinRelease;
DigitalOut<PortB, 1>  pinBuzzer;

HumiditySensor<TWIMaster>  humiSensor;
Buzzer<TWIMaster>  buzzer;

TWIMaster   i2cBus;
MinimumSerial dbg;

SdFat SD;

//ArduinoOutStream cout(MinimumSerial);

File outfile;


void errorHalt(const char* msg)
{
  dbg.print("Error: ");
  dbg.println(msg);
  while(1);
}

bool initSD()
{
  if (!SD.begin()) {
    if (SD.card()->errorCode()) {
      dbg.print("Error code: ");
      dbg.println(SD.card()->errorCode(), HEX);
      dbg.print("Error data: ");
      dbg.println(SD.card()->errorData(), HEX);
    }
    dbg.println("Cannot communicate with SD card");
    return false;
  }

  if (SD.vol()->fatType() == 0) {
    dbg.println("Can't find a valid FAT16/FAT32 partition.");
    return false;
  }

  uint32_t size = SD.card()->cardSize();
  uint32_t sizeMB = 0.000512 * size + 0.5;
  dbg.print("SD card size, blocks: "); dbg.println(size);
  dbg.print("SD card size, MB: "); dbg.println(sizeMB);
  dbg.print("Cluster size, bytes: "); dbg.println(512L * SD.vol()->blocksPerCluster());
  return true;
}

bool testFSWrite()
{
  if (SD.exists("OUT2.TXT")) {
    dbg.println("Output file exists, overwriting...");
  }

  dbg.println("Starting write");
  uint32_t start = millis();

  outfile = SD.open("OUT2.TXT", FILE_WRITE);
  if (!outfile) {
    dbg.println("Could not open file!");
    return false;
  }
  for (byte idx = 0; idx < 100; idx++) {
    outfile.print("Time is: ");
    outfile.println(millis());
    dbg.print('.');
  }
  outfile.close();

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

  outfile = SD.open("OUT2.TXT");
  if (!outfile) {
    dbg.println("Could not open file!");
    return false;
  }
  uint16_t nChars = 0, nLines = 0;
  while (outfile.available()) {
    char line[64];
    //nChars += outfile.fgets(line, sizeof(line));
    nChars += outfile.read(line, 64);
    //dbg.print(line);
    nLines++;
  }
  outfile.close();

  dbg.print("Read "); dbg.print(nLines); dbg.println(" lines from file");
  dbg.print("Read "); dbg.print(nChars); dbg.println(" bytes from file");
  dbg.print("Took: "); dbg.println(millis() - start);
  return true;
}

void setup()
{
  dbg.begin(9600);
  dbg.println("Reset!");

  i2cBus.begin();

  TCCR1A = 0;
  TCCR1B = _BV(WGM13) | _BV(CS12) | _BV(CS10);    // Prescaler 1024
  TCCR1C = 0;
  TIMSK1 = _BV(TOIE1);
  ICR1 = 15625;

  set_sleep_mode(SLEEP_MODE_IDLE);
  sei();

  initSD();

  outfile = SD.open("LOG.TXT", FILE_WRITE);
  if (!outfile) {
    dbg.println("Could not open file!");
  }
  else {
    dbg.println("Opened log file for writing (append)");
  }

  //testFSWrite();
  bool success = false;
  for (uint8_t nTry = 10; nTry > 0; nTry--) {
    dbg.println("Initializing humidity sensor");
    if (humiSensor.begin()) {
      success = true;
      break;
    }
    delay(100);
  }

  if (!success) {
    dbg.println("Failed to initialize humidity sensor");
    outfile.println("*** INIT FAIL ****");
  }
  else {
    outfile.println("*** INIT OK ****");
  }
}

volatile uint8_t gFlags;

void loop()
{
  if (gFlags & 1) {
    gFlags &= ~1;
    // Take measurement
    bool success = humiSensor.update();
    int16_t rhum = humiSensor.getHumidity();
    int16_t temp = humiSensor.getTemperature();

    if (success) {
      dbg.print("RH: "); dbg.print(rhum * 0.1f, 1); dbg.print(" Temp: "); dbg.println(temp * 0.1f, 1);
      buzzer.setMode(0);
    }
    else {
      dbg.println("Failed to read humidity sensor");
      buzzer.setMode(1);
    }
    outfile.print(rhum * 0.1f); outfile.print('\t'); outfile.println(temp * 0.1f);
    outfile.flush();    // Just to be sure

  }

  //delay(1000);
  sleep_mode();
}

ISR(TWI_vect) {
  i2cBus.onTWIInterrupt();
}

ISR(TIMER1_OVF_vect) {
  // Once a second
  gFlags |= 1;
}
