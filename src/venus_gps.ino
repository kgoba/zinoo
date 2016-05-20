#include <Wire.h>
#include <Sensirion.h>

int pinLock = 5;
int pinLed  = 13;
 
const uint8_t dataPin =  A4;              // SHT serial data
const uint8_t sclkPin =  A5;              // SHT serial clock
 

const byte kBarAddress = 0b1110111;     // 7-bit slave address
const byte kBarCmdRead = 0b10100000;
const byte kBarCmdConvertD1 = 0b01000000;
const byte kBarCmdConvertD2 = 0b01010000;
const byte kBarCmdResult = 0b00000000;

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



word gBarCalibration[6];      // 6 16-bit values corresponding to C1-C6

void baroInit() {
  Serial.println("Requesting PROM read");
  byte romAddress;
  for (romAddress = 0; romAddress < 8; romAddress++) {
    Serial.print(romAddress);
    Serial.print(": ");
    word reading;
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
    int inByte = Serial.read();
    Serial.write(inByte);
  }
}

bool startPressureConversion(BarOversampling osr = kBarOSR256) {
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdConvertD1 | ((byte)osr << 1));  // send Convert instruction
  Wire.endTransmission();     // stop transmitting
  return true;
}

bool startTemperatureConversion(BarOversampling osr = kBarOSR256) {
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdConvertD2 | ((byte)osr << 1));  // send Convert instruction
  Wire.endTransmission();     // stop transmitting
  return true;
}

bool readResult(long &reading) {
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdResult);  // send Convert instruction
  Wire.endTransmission();     // stop transmitting

  
  // Request I2C read
  Wire.requestFrom(kBarAddress, 3);
  if (3 <= Wire.available()) { // if two bytes were received
    reading = Wire.read();  // receive high byte (overwrites previous reading)
    reading = reading << 8;    // shift high byte to be high 8 bits
    reading |= Wire.read(); // receive low byte as lower 8 bits
    reading = reading << 8;    // shift high byte to be high 8 bits
    reading |= Wire.read(); // receive low byte as lower 8 bits
    return true;
  }  
  return false;
}

bool readPROM(byte address, word &reading) {
  Wire.beginTransmission(kBarAddress);                // select slave address
  Wire.write(kBarCmdRead + ((address & 0x07) << 1));  // send Read instruction
  Wire.endTransmission();     // stop transmitting

  // Request I2C read
  Wire.requestFrom(kBarAddress, 2);
  if (2 <= Wire.available()) { // if two bytes were received
    reading = Wire.read();  // receive high byte (overwrites previous reading)
    reading = reading << 8;    // shift high byte to be high 8 bits
    reading |= Wire.read(); // receive low byte as lower 8 bits
    return true;
  }  
  return false;
}

void loop() {
  //digitalWrite(pinLed, digitalRead(pinLock));

  /*
  */

  long d1;
  long d2;

  startTemperatureConversion(kBarOSR2048);
  delay(100);
  if (readResult(d2)) {
      Serial.print("Temperature ADC: ");
      Serial.print(d2);   // print the reading        
      Serial.print("/");
      Serial.println(d2, HEX);   // print the reading        
      word dt = (d2 >> 8) - gBarCalibration[4];
      word t  = 2000 + (((long)dt * gBarCalibration[5]) >> 15);
      Serial.print("Temperature: ");
      Serial.println(t);   // print the result

      startPressureConversion(kBarOSR2048);
      delay(100);
      if (readResult(d1)) {
          Serial.print("Pressure ADC: ");
          Serial.print(d1);   // print the reading        
          Serial.print("/");
          Serial.println(d1, HEX);   // print the reading
          word off  = gBarCalibration[1] + ((gBarCalibration[3] * (long)dt) >> 15);
          word sens = gBarCalibration[0] + ((gBarCalibration[2] * (long)dt) >> 15);
          word p    = (((d1 >> 8) * sens) >> 14) - off;
          Serial.print("Pressure: ");
          Serial.println(4 * (long)p);   // print the result
      }
      else {
        Serial.println("ERROR: Failed to read pressure!");
      }
  }
  else {
    Serial.println("ERROR: Failed to read temperature!");    
  }
  


  /*
  //TWCR = _BV(TWINT);

  Serial.println("Reading SHT75");
  uint16_t rawData = 0;
  uint8_t rc = sht.measTemp(&rawData);
  Serial.print("RC: ");
  Serial.println(rc);   // print the result
  Serial.print("Temperature: ");
  Serial.println(rawData);   // print the result

  //TWCR = _BV(TWINT) | _BV(TWEN);
  */
  
  Serial.println();
  delay(500);
}
