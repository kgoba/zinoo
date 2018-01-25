#pragma once
#include "Arduino.h"

struct GPSInfo {
  char altitude[8];
  char latitude[12];
  char longitude[12];
  byte satCount;
  char time[12];
  char fix;

  GPSInfo();

  void clear();

  void setFix(char fix);
  void setTime(const char *time);
  void setLatitude(const char *latitude);
  void setLongitude(const char *longitude);
  void setAltitude(const char *altitude);
  void setSatCount(const char *satCount);
  void print();

  int8_t getSeconds();
  int8_t getMinutes();
};

struct GPSParser {
  enum SentenceType {
    SENTENCE_NONE = 0,
    SENTENCE_GSA,
    SENTENCE_GGA,
    SENTENCE_RMC
  };

  bool parse(char c);
  void parseByChar(char c);
  void processField(byte sentence, byte index, const char *value, byte length);

  GPSInfo gpsInfo;
};


// GPS serial (hardware/software) functions
void gpsBegin();
byte gpsAvailable();
byte gpsRead();
