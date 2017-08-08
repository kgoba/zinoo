#include <Wire.h>

int pinLock = 5;
int pinLed  = 13;
 
const uint8_t dataPin =  A4;              // SHT serial data
const uint8_t sclkPin =  A5;              // SHT serial clock

const uint8_t kBarAddress = 0b1110111;     // 7-bit slave address
const uint8_t kBarCmdRead = 0b10100000;
const uint8_t kBarCmdConvertD1 = 0b01000000;
const uint8_t kBarCmdConvertD2 = 0b01010000;
const uint8_t kBarCmdResult = 0b00000000;

enum BarOversampling {
  kBarOSR256 = 0,
  kBarOSR512 = 1,
  kBarOSR1024 = 2,
  kBarOSR2048 = 3,
  kBarOSR4096 = 4
};

//Sensirion sht = Sensirion(dataPin, sclkPin);

// Atmospheric pressure table for altitudes 0 km .. 32 km 
// in steps of 1 km (multiply by 4 to get pascals)
// 
// 25331 22469 19874 17527 15410 13505 11795 10265 8900 7686 6609 5658 
// 4833 4128 3525 3011 2572 2197 1876 1603 1369 1169 1000 856 733 628 
// 538 462 397 341 293 252 217 



/* 
 *  Barometer calibratin data used for calculation
 */
uint16_t gBarCalibration[6];      // 6 16-bit values corresponding to C1-C6

/*  
 *  Initialize barometer by reading calibration values from PROM
 *  No specific hardware initialization is necessary.
 */
void baroInit() {
  Serial.println("Requesting PROM read");
  uint8_t romAddress;
  for (romAddress = 0; romAddress < 8; romAddress++) {
    Serial.print(romAddress);
    Serial.print(": ");
    uint16_t reading;
    if (readPROM(romAddress, reading)) {
      Serial.println(reading);   // print the reading
      if (romAddress > 0 && romAddress < 7) {
        gBarCalibration[romAddress - 1] = reading;
      }
    }
    else {
      Serial.println("Failed to read");
    }
  }
}

/*
 * Call this to start pressure ADC conversion
 */
bool startPressureConversion(BarOversampling osr = kBarOSR256) {
  // Send I2C command
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdConvertD1 | ((uint8_t)osr << 1));  // send Convert instruction
  Wire.endTransmission();     // stop transmitting
  return true;
}

/*
 * Call this to start temperature ADC conversion
 */
bool startTemperatureConversion(BarOversampling osr = kBarOSR256) {
  // Send I2C command
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdConvertD2 | ((uint8_t)osr << 1));  // send Convert instruction
  Wire.endTransmission();     // stop transmitting
  return true;
}

/*
 * Read ADC conversion result (24 bit value)
 */
bool readResult(long &reading) {
  // Send I2C command
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdResult);  // send Convert instruction
  Wire.endTransmission();     // stop transmitting
  
  // Request I2C read
  Wire.requestFrom(kBarAddress, 3);
  if (3 <= Wire.available()) { // if two uint8_ts were received
    reading = Wire.read();  // receive high uint8_t (overwrites previous reading)
    reading = reading << 8;    // shift high uint8_t to be high 8 bits
    reading |= Wire.read(); // receive low uint8_t as lower 8 bits
    reading = reading << 8;    // shift high uint8_t to be high 8 bits
    reading |= Wire.read(); // receive low uint8_t as lower 8 bits
    return true;
  }  
  return false;
}


/*
 * Read ADC conversion result (most significant 16 bits of 24 bit value)
 */
bool readResultuint16_t(uint16_t &reading) {
  // Send I2C command
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdResult);  // send Convert instruction
  Wire.endTransmission();     // stop transmitting
  
  // Request I2C read
  Wire.requestFrom(kBarAddress, 3);
  if (3 <= Wire.available()) { // if two uint8_ts were received
    reading = Wire.read();  // receive high uint8_t (overwrites previous reading)
    reading = reading << 8;    // shift high uint8_t to be high 8 bits
    reading |= Wire.read(); // receive low uint8_t as lower 8 bits
    Wire.read(); // receive low uint8_t and discard it
    return true;
  }  
  return false;
}

/*
 * Read 16-bit entry from PROM (addresses 0-7 supported)
 */
bool readPROM(uint8_t address, uint16_t &reading) {
  // Send I2C command
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdRead + ((address & 0x07) << 1));  // send Read instruction
  Wire.endTransmission();     // stop transmitting

  // Request I2C read
  Wire.requestFrom(kBarAddress, 2);
  if (2 <= Wire.available()) { // if two uint8_ts were received
    reading = Wire.read();  // receive high uint8_t (overwrites previous reading)
    reading = reading << 8;    // shift high uint8_t to be high 8 bits
    reading |= Wire.read(); // receive low uint8_t as lower 8 bits
    return true;
  }  
  return false;
}

/*
 * Measures both temperature and pressure
 * Returns temperature as Celsium x100 (2000 = 20.00 C)
 * Returns pressure as Pascals / 4 (25000 = 100000 Pa = 1000 mbar)
 */
bool readBarometer(int16_t &t, uint16_t &p) {
  long d1, d2;
  
  startTemperatureConversion(kBarOSR1024);
  delay(2);
  if (!readResult(d2)) {
    return false;
  }
  //Serial.print("Temperature ADC: ");
  //Serial.println(d2);   // print the reading        
  int16_t dt = (d2 >> 8) - gBarCalibration[4];
  t  = 2000 + (((signed long)dt * gBarCalibration[5]) >> 15);

  startPressureConversion(kBarOSR4096);
  delay(10);
  if (!readResult(d1)) {
    return false;
  }
  //Serial.print("Pressure ADC: ");
  //Serial.println(d1);   // print the reading        
  uint16_t off  = gBarCalibration[1] + ((gBarCalibration[3] * (signed long)dt) >> 15);
  uint16_t sens = gBarCalibration[0] + ((gBarCalibration[2] * (signed long)dt) >> 15);
  p    = (((d1 >> 8) * sens) >> 14) - off;

  return true;
}

void setup() {
  Wire.begin(); // join i2c bus (address optional for master)
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Serial.begin(9600);
  pinMode(pinLock, INPUT_PULLUP);
  pinMode(pinLed, OUTPUT);
  Serial.println("Reset");

  baroInit();
}

void serialEcho() {
  if (Serial.available()) {
    int value = Serial.read();
    Serial.write(value);
  }
}

void loop() {
  //digitalWrite(pinLed, digitalRead(pinLock));

  uint16_t p;
  int16_t  t;
  if (readBarometer(t, p)) {
      Serial.print("Temperature: ");
      Serial.println(t);   // print the result
      Serial.print("Pressure: ");
      Serial.println(4 * (long)p);   // print the result
  }
  
  Serial.println();
  delay(500);
}
