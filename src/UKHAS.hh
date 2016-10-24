#pragma once

#include <crc.hh>
#include <fstring.hh>

#include "GPS.hh"

struct FlightData {
  FlightData();

  /* GPS info */
  char       fix;
  FString<6> time;
  FString<7> latitude;
  FString<7> longitude;
  uint16_t   altitude;
  uint8_t    satCount;

  /* Pressure and altitude */
  uint16_t pressure;
  uint16_t barometricAltitude;

  /* Various sensors */
  int8_t temperatureInternal;   // range 00..99 corresponding to -60C .. 40C
  int8_t temperatureExternal;   // range 00..99 corresponding to -60C .. 40C

  /* General status */
  uint16_t batteryVoltage;        // range 00..99 corresponding to 0.50V .. 1.50V
  byte status;

  void updateGPS(const GPSInfo &gps);
  void updateTemperature(int8_t tempExt, int8_t tempInt);

  int8_t getSeconds();
  int8_t getMinutes();

  void print();
};


class UKHASPacketizer {
public:
  UKHASPacketizer(const char *payloadName = "");


  void setPayloadName(const char *payloadName);

  template<uint8_t N>
  void setPayloadName(const FString<N> &payloadName) {
    this->payloadName.assign(payloadName);
  }

  void makePacket(const FlightData &data);

  const uint8_t *getPacketBuffer();
  uint8_t getPacketLength();

  uint16_t    sentenceID;
  FString<6>  payloadName;

  FString<80> packet;
  CRC16_CCITT crc;
};
