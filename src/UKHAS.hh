#pragma once

#include <crc.hh>
#include <fstring.hh>

#include "GPS.hh"

struct FlightData {
  FlightData();

  char fix;
  FString<6> time;
  FString<7> latitude;
  FString<7> longitude;
  uint16_t   altitude;
  uint8_t    satCount;

  uint16_t pressure;
  uint16_t barometricAltitude;

  int8_t temperatureInternal;   // range 00..99 corresponding to -60C .. 40C
  int8_t temperatureExternal;   // range 00..99 corresponding to -60C .. 40C
  uint16_t batteryVoltage;        // range 00..99 corresponding to 0.50V .. 1.50V
  byte status;

  void updateGPS(const GPSInfo &gps);
  void updateTemperature();
  void updateTime();

  int8_t getSeconds();
  int8_t getMinutes();

  void print();
};


class UKHASPacketizer {
public:
  UKHASPacketizer(const char *payloadName = "UNK");
  void setPayloadName(const char *payloadName);

  //template<byte N>
  //void setPayloadName(const FString<N> &payloadName);

  template<byte N>
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
