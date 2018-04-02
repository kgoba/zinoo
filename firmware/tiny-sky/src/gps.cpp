#include "gps.h"

#include "strconv.h"

#include <cstring>
#include <cstdlib>

static const char CHAR_LF = '\x0A';
static const char CHAR_CR = '\x0D';

GPSParserBase::GPSParserBase() {
    checksum = 0;
    checksumReported = 0;
    checksumState = 0;
    lineLength = 0;
}

void GPSParserBase::onChecksumError(const char *line) {
}

void GPSParserBase::onChecksumOK(const char *line) {
}

void GPSParserBase::onFieldData (SentenceType sentence, int index, const char *value, int length) {
}

bool GPSParserBase::decode(char c) {
    bool isValidSentence = false;

    if (c == CHAR_LF) {

    }
    else if (c == CHAR_CR) {
        // parse entire line
        if (lineLength > 0) {
            line[lineLength] = 0;
            if (checksum != checksumReported) {
                onChecksumError(line);
            }
            else {
                onChecksumOK(line);

                isValidSentence = true;
                parseLine(line);
            }
        }

        checksum = 0;
        checksumReported = 0;
        checksumState = 0;
        lineLength = 0;
    }
    else {
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
    return isValidSentence;
}

/*
$GPGGA,091248.00,,,,,0,04,4.90,,,,,,*59
$GPGGA,091249.00,5657.86166,N,02408.29029,E,1,04,4.90,7.5,M,22.9,M,,*59
$GPGGA,091250.00,5657.86291,N,02408.29371,E,1,04,4.90,10.3,M,22.9,M,,*64

$GPRMC,090907.00,V,,,,,,,160218,,,N*76
$GPRMC,090943.00,A,5657.88213,N,02408.34752,E,0.085,,160218,,,A*7E
$GPRMC,090944.00,A,5657.88169,N,02408.34761,E,0.215,,160218,,,A*7C

$GPVTG,,,,,,,,,N*30
$GPVTG,,T,,M,1.468,N,2.719,K,A*25
$GPVTG,,T,,M,1.139,N,2.109,K,A*23

*/

void GPSParserBase::parseLine(const char *line) {
    SentenceType sentence = SENTENCE_NONE;
    char fieldValue[13];
    int  fieldLength = 0;
    int  fieldIndex  = 0;

    while (*line) {
        char c = *line;

        if (c == ',' || c == '*') {
            // Process field
            if (fieldIndex == 0) {
                if (memcmp(fieldValue, "$GPGSA", 6) == 0) {
                    sentence = SENTENCE_GSA;
                }
                else if (memcmp(fieldValue, "$GPGGA", 6) == 0) {
                    sentence = SENTENCE_GGA;
                }
                else if (memcmp(fieldValue, "$GPRMC", 6) == 0) {
                    sentence = SENTENCE_RMC;
                }
                else if (memcmp(fieldValue, "$GPGLL", 6) == 0) {
                    sentence = SENTENCE_GLL;
                }
                else if (memcmp(fieldValue, "$GPVTG", 6) == 0) {
                    sentence = SENTENCE_VTG;
                }
                else if (memcmp(fieldValue, "$GPGSV", 6) == 0) {
                    sentence = SENTENCE_GSV;
                }
            }
            else {
                // Add terminating zero
                fieldValue[fieldLength] = 0;
                onFieldData(sentence, fieldIndex, fieldValue, fieldLength);
            }
            fieldIndex++;
            fieldLength = 0;
        }
        else {
            if (fieldLength < 12) {
                fieldValue[fieldLength++] = c;
            }
        }
        line++;
    }
}

/*
 GGA - essential fix data which provide 3D location and accuracy data.

 $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47

Where:
         GGA                    Global Positioning System Fix Data
         123519             Fix taken at 12:35:19 UTC
         4807.038,N     Latitude 48 deg 07.038' N
         01131.000,E    Longitude 11 deg 31.000' E
         1                        Fix quality: 0 = invalid
                                               1 = GPS fix (SPS)
                                               2 = DGPS fix
                                               3 = PPS fix
                                               4 = Real Time Kinematic
                                               5 = Float RTK
                                               6 = estimated (dead reckoning) (2.3 feature)
                                               7 = Manual input mode
                                               8 = Simulation mode
         08                     Number of satellites being tracked
         0.9                    Horizontal dilution of position
         545.4,M            Altitude, Meters, above mean sea level
         46.9,M             Height of geoid (mean sea level) above WGS84
                                            ellipsoid
         (empty field) time in seconds since last DGPS update
         (empty field) DGPS station ID number
         *47                    the checksum data, always begins with *

 */

/*
void GPSParserBase::onFieldData (SentenceType sentence, int index, const char *value, int length) {
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
                case 3: break; // N or S
                case 4: gpsInfo.setLongitude(value); break;
                case 5: break; // E or W
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
*/

void GPSFix::YMDDate::parse(const char *str) {
    if (strlen(str) < 6) {
        _valid = false;
        return;
    }
    
    uint32_t value;
    _valid = strparse(value, str, 2);
    if (!_valid) return;
    _year = value;

    _valid = strparse(value, str + 2, 2);
    if (!_valid) return;
    _month = value;

    _valid = strparse(value, str + 4, 2);
    if (!_valid) return;
    _day = value;
}

void GPSFix::HMSTime::parse(const char *str) {
    if (strlen(str) < 6) {
        _valid = false;
        return;
    }
    
    uint32_t value;
    _valid = strparse(value, str, 2);
    if (!_valid) return;
    _hrs = value;

    _valid = strparse(value, str + 2, 2);
    if (!_valid) return;
    _min = value;

    _valid = strparse(value, str + 4, 2);
    if (!_valid) return;
    _sec = value;
}

void GPSFix::Integer::parse(const char *str) {
    int32_t value32;
    _valid = strparse(value32, str);
    if (!_valid) return;
    _value = value32;
}

void GPSFix::Distance::parse(const char *str) {
    _valid = strparse(_meters, str);
    //char * endptr;
    //_meters = strtof(str, &endptr);
    //_valid = (endptr != str);
}

void GPSFix::Angle::parse(const char *str) {
    char *point = strchr(str, '.');
    if (!point) {
        _valid = false;
        return;
    }
    int width = (point) ? (point - str - 2) : (strlen(str) - 2);
    if (width <= 0) {
        _valid = false;
        return;
    }
    uint32_t degrees;
    _valid = strparse(degrees, str, width);
    if (!_valid) return;
    _deg = (uint8_t)degrees;

    _valid = strparse(_min, str + width);
    /*
    if (!_valid) return;
    _min = minutes;
    float seconds = 60 * (minutes - _min);
    _sec = seconds;
    float sec_100 = 100 * (seconds - _sec);
    _sec_100 = 0.5f + sec_100;
    */
}

void GPSFix::Latitude::parseCardinal(const char *str) {
    if (str[0] == 'N' && str[1] == '\0') {
        _negative = false;
    }
    else if (str[0] == 'S' && str[1] == '\0') {
        _negative = true;
    }
    else {
        _valid = false;
    }
}

void GPSFix::Longitude::parseCardinal(const char *str) {
    if (str[0] == 'E' && str[1] == '\0') {
        _negative = false;
    }
    else if (str[0] == 'W' && str[1] == '\0') {
        _negative = true;
    }
    else {
        _valid = false;
    }
}

void GPSFix::FixType::parse(const char *str) {
    _fix = str[0];
}

void GPSFix::parseField(GPSParserBase::SentenceType sentence, int index, const char *value, int length) {
    switch (sentence) {
        case GPSParserBase::SENTENCE_GGA:
            switch (index) {
                case 1: time.parse(value); break;  // time
                case 2: latitude.parse(value); break;
                case 3: latitude.parseCardinal(value); break;
                case 4: longitude.parse(value); break;
                case 5: longitude.parseCardinal(value); break;
                case 6: break;  // fix quality: 0 or more
                case 7: tracked.parse(value); break;  // num of satellites tracked
                case 9: altitude.parse(value); break;
            }
            break;
        case GPSParserBase::SENTENCE_GSA:
            switch (index) {
                case 2: fixType.parse(value); break;
            }
            break;
        case GPSParserBase::SENTENCE_GSV:
            switch (index) {
                case 3: inView.parse(value); break;  // num of satellites in view
            }
            break;
        default:
            break;
    }
    /*
    switch (sentence) {
        case SENTENCE_GGA:
            switch (index) {
                //case 1: if (length > 0) { print("GGA TIME: "); print(value); print('\n'); } break;
                case 2: if (length > 0) { print("GGA LAT: "); print(value); print('\n'); } break;
                case 3: break; // N or S
                //case 4: if (length > 0) { print("GGA LNG: "); print(value); print('\n'); } break;
                case 5: break; // E or W
                //case 6: if (length > 0) { print("GGA FIX: "); print(value); print('\n'); } break;   // 0 or more
                case 7: if (length > 0) { print("GGA SAT: "); print(value); print('\n'); } break;
                //case 9: if (length > 0) { print("GGA ALT: "); print(value); print('\n'); } break;
            }
            break;
        case SENTENCE_RMC:
            switch (index) {
                case 1: print("RMC TIME: "); print(value); print('\n'); break;
                case 2: print("RMC FIX: "); print(value); print('\n'); break;  // A or V 
                case 3: print("RMC LAT: "); print(value); print('\n'); break;
                case 5: print("RMC LNG: "); print(value); print('\n'); break;
                case 7: print("RMC SPD: "); print(value); print('\n'); break;
                case 8: print("RMC TRK: "); print(value); print('\n'); break;
                case 9: print("RMC DATE: "); print(value); print('\n'); break;
            }
            break;
        case SENTENCE_GSA:
            switch (index) {
                case 2:  print("GSA FIX: "); print(value); print('\n'); break; // 1/2/3
                case 14: print("GSA PDOP: "); print(value); print('\n'); break;
                case 15: print("GSA HDOP: "); print(value); print('\n'); break;
                case 16: print("GSA VDOP: "); print(value); print('\n'); break;
            }
            break;
        case SENTENCE_GLL:
            switch (index) {
                case 5: print("GLL TIME: "); print(value); print('\n'); break;
                case 6: print("GLL FIX: "); print(value); print('\n'); break; // A or V
                case 1: print("GLL LAT: "); print(value); print('\n'); break;
                case 3: print("GLL LNG: "); print(value); print('\n'); break;
            }
            break;
        case SENTENCE_VTG:
            switch (index) {
                case 5: print("VTG SPD: "); print(value); print('\n'); break;
                case 1: print("VTG TRK: "); print(value); print('\n'); break;
            }
            break;
        default:
            break;
    }
    */
}


void GPSParserSimple::onChecksumError(const char *line) {
    //debug.printf("NMEA checksum ERROR\n");
    sentences_err++;
}

void GPSParserSimple::onChecksumOK(const char *line) {
    sentences_ok++;
}

void GPSParserSimple::onFieldData(SentenceType sentence, int index, const char *value, int length) {
    latest.parseField(sentence, index, value, length);
}
