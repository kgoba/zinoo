#include "UKHAS.hh"
#include "Config.hh"
#include "debug.hh"

#include <adc.hh>

FlightData::FlightData() {
  temperatureInternal = 0;
  temperatureExternal = 0;
  batteryVoltage = 0;
  pressure = 0;
  barometricAltitude = 0;
  status = 0;
}

void FlightData::updateGPS(const GPSInfo &gps) {
  if (gps.time[0]) time.assign(gps.time);
  if (gps.latitude[0]) latitude.assign(gps.latitude);
  if (gps.longitude[0]) longitude.assign(gps.longitude);
  if (gps.altitude[0]) {
    int16_t tmp = atoi(gps.altitude);
    if (tmp > 0) {
      altitude = tmp;
    }
    else {
      altitude = 0;
    }
  }
  else {
    altitude = 0;
  }

  satCount = gps.satCount;
}

void FlightData::print() {
  char delim = ' ';

  dbg << F("FD: ");
  if (fix == '3') dbg << F("3D fix ");

  if (time.size > 0) {
    dbg << time << ' ';
  }
  if (latitude.size > 0) {
    dbg << latitude << F("N ");
  }
  if (longitude.size > 0) {
    dbg << longitude << F("E ");
  }
  if (altitude > 0) {
    dbg << altitude << F("m ");
  }

  dbg << satCount << ' ';

  if (pressure > 0) {
    dbg.print(4 * (uint32_t)pressure); dbg << F("Pa ");
    dbg.print(barometricAltitude); dbg << F("m ");
  }

  dbg.print(temperatureInternal); dbg << F("C ");
  dbg.print(temperatureExternal); dbg << F("C ");
  dbg.print(batteryVoltage / 100.0); dbg << F("V ");
  dbg.println();
}

void FlightData::updateTemperature(int8_t tempExt, int8_t tempInt) {
  temperatureInternal = tempInt;
  temperatureExternal = tempExt;
  /*
  temperatureInternal = convertTemperature(adcRead(adcChanTempInt));
  temperatureExternal = convertTemperature(adcRead(adcChanTempExt));
  batteryVoltage = adcRead(adcChanBattery) / 3;    // Approximation of x*330/1024
  */
}

UKHASPacketizer::UKHASPacketizer(const char *payloadName) {
  sentenceID = 1;
  setPayloadName(payloadName);
}

void UKHASPacketizer::setPayloadName(const char *payloadName) {
  this->payloadName.assign(payloadName);
}

void UKHASPacketizer::makePacket(const FlightData &data) {
  uint8_t nibble = data.status >> 4;
  char statusChar1 = (nibble > 9) ? nibble - 10 + 'A' : nibble + '0';
  nibble = data.status & 0x0F;
  char statusChar2 = (nibble > 9) ? nibble - 10 + 'A' : nibble + '0';

  uint16_t altitude = (data.altitude > 0) ? data.altitude : data.barometricAltitude;

  int16_t tempInt = 60 + data.temperatureInternal;
  if (tempInt > 99) tempInt = 99;
  else if (tempInt < 0) tempInt = 0;

  int16_t tempExt = 60 + data.temperatureExternal;
  if (tempExt > 99) tempExt = 99;
  else if (tempExt < 0) tempExt = 0;

  //int16_t battVoltage = -60 + data.batteryVoltage;
  //if (battVoltage > 99) battVoltage = 99;
  //else if (battVoltage < 0) battVoltage = 0;
  int16_t battVoltage = data.batteryVoltage;

  packet.clear();
  packet.append("$$");
  packet.append(payloadName);
  packet.append(','); packet.append(sentenceID);
  //packet.append(','); packet.append(FString<7>(data.gpsInfo.latitude));
  //packet.append(','); packet.append(FString<7>(data.gpsInfo.longitude));
  //packet.append(','); packet.append(FString<8>(data.gpsInfo.altitude));
  //packet.append(','); packet.append(FString<6>(data.gpsInfo.time));

  packet.append(','); packet.append(data.latitude);
  packet.append(','); packet.append(data.longitude);
  packet.append(','); packet.append((5 + altitude) / 10);
  packet.append(','); packet.append(data.time);

  packet.append(','); packet.append(data.satCount < 10 ? data.satCount : 9);

  //packet.append(','); packet.append(tempInt);
  packet.append(','); packet.append(data.temperatureInternal);

  //packet.append(','); packet.append(tempExt);
  packet.append(','); packet.append(data.temperatureExternal);

  packet.append(','); packet.append(data.batteryVoltage);
  packet.append(',');
  if (statusChar1 != '0') packet.append(statusChar1);
  packet.append(statusChar2);

  uint16_t pressureMilliBar10 = (data.pressure * 2) / 5;
  packet.append(','); packet.append(pressureMilliBar10);
  packet.append(','); packet.append((5 + data.barometricAltitude) / 10);


  // Now compute CRC checksum and append it
  crc.clear();
  uint16_t prefixLength = 2;    // Length of prefix to be ignored
  uint16_t checksum = crc.update((uint8_t *)(packet.buf + prefixLength), packet.size - prefixLength);
  packet.append('*'); packet.append(checksum, HEX);

  // Must send a newline character as well
  packet.append('\n');

  sentenceID++;
}

const uint8_t * UKHASPacketizer::getPacketBuffer() {
  return (const uint8_t *)packet.buf;
}

uint8_t UKHASPacketizer::getPacketLength() {
  return packet.size;
}
