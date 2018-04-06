#pragma once
#include <cstdint>
#include <cmath>

struct GPSInfo {
    // TEXT format
    char altitude[8];
    char latitude[12];
    char latitude_card;
    char longitude[12];
    char longitude_card;
    char time[12];
    char fix;

    uint8_t satCount;

    GPSInfo();

    void clear();

    void setFix(char fix);
    void setTime(const char *time);
    void setLatitude(const char *latitude);
    void setLongitude(const char *longitude);
    void setAltitude(const char *altitude);
    void setSatCount(const char *satCount);
    void print();

    //int8_t getSeconds();
    //int8_t getMinutes();
};

class GPSParserBase {
public:
    GPSParserBase();

    bool decode(char c);

    //GPSInfo gpsInfo;
    enum SentenceType {
        SENTENCE_NONE = 0,
        SENTENCE_GSA,
        SENTENCE_GGA,
        SENTENCE_RMC,
        SENTENCE_VTG,
        SENTENCE_GLL,
        SENTENCE_GSV
    };

protected:
    virtual void onChecksumError(const char *line);
    virtual void onChecksumOK(const char *line);
    virtual void onFieldData(SentenceType sentence, int index, const char *value, int length);

private:
    //void processField(SentenceType sentence, uint8_t index, const char *value, uint8_t length);

    void parseLine(const char *line);

    char    line[100];
    int     lineLength;
    int     checksumState;
    uint8_t checksum;
    uint8_t checksumReported;
};


struct GPSFix {
    class Field {
    public:
        Field() : _valid(false) {}
        bool    valid()   const { return _valid; }
    protected:
        bool    _valid;
    };

    class YMDDate : public Field {
    public:
        YMDDate() : Field() {}
        YMDDate(const char *str) { parse(str); }

        uint8_t year()   const { return _year; }
        uint8_t month() const { return _month; }
        uint8_t day() const { return _day; }

        void    parse(const char *str);

    private:
        uint8_t     _year;
        uint8_t     _month;
        uint8_t     _day;
    };

    class HMSTime : public Field {
    public:
        HMSTime() : Field() {}
        HMSTime(const char *str) { parse(str); }

        uint8_t hour()   const { return _hrs; }
        uint8_t minute() const { return _min; }
        uint8_t second() const { return _sec; }
        uint32_t totalSeconds() const { return ((_hrs * 60ul) + _min) * 60ul + _sec; }
        float    dayFraction()  const { return totalSeconds() / (24 * 3600.0f); }

        void    parse(const char *str);

    private:
        uint8_t     _hrs;
        uint8_t     _min;
        uint8_t     _sec;
    };

    class Integer : public Field {
    public:
        Integer() : Field() {}
        Integer(const char *str) { parse(str); }

        int     value() const { return _value; }

        void    parse(const char *str);

    private:
        int   _value;
    };

    class Distance : public Field {
    public:
        Distance() : Field() {}
        Distance(const char *str) { parse(str); }

        float   meters() const { return _meters; }
        float   feet()   const { return _meters / 0.3048f; }

        void    parse(const char *str);

    private:
        float _meters;
    };

    class Angle : public Field {
    public:
        Angle() : Field() {}
        Angle(const char *str) { parse(str); }
        Angle(const char *str, bool negative) { parse(str); _negative = negative; }

        bool     negative() const { return _negative; }
        uint8_t  degrees()  const { return _deg; }
        uint8_t  minutes()  const { return _min; }
        float    minutesFloat() const { return _min; }
        float    degreesFloat() const { return _deg + _min / 60; }
        float    degreesSignedFloat() const { return _negative ? -degreesFloat() : degreesFloat(); }

        uint8_t  seconds()  const { return (_min - (int)_min) * 60; }
        uint8_t  hundreths() const { 
            float _sec = (_min - (int)_min) * 60; 
            return 0.5f + (_sec - (int)_sec) * 100; 
        }

        void     parse(const char *str);

    protected:
        bool     _negative;
        uint8_t  _deg;      // [0, 180)
        float    _min;      // [0, 60)
    };

    class Latitude : public Angle {
    public:
        Latitude() : Angle() {}

        bool     north() const { return !_negative; }
        bool     south() const { return _negative; }
        char     cardinal() const { return _negative ? 'S' : 'N'; }

        void     parseCardinal(const char *str);
    };

    class Longitude : public Angle {
    public:
        Longitude() : Angle() {}

        bool     east() const { return !_negative; }
        bool     west() const { return _negative; }
        char     cardinal() const { return _negative ? 'W' : 'E'; }

        void     parseCardinal(const char *str);
    };

    class FixType {
    public:
        FixType() : _fix('0') {}

        bool     is3D() const { return (_fix == '3'); }
        bool     atLeast2D() const { return (_fix == '3' || _fix == '2'); }

        void     parse(const char *str);

    private:
        char _fix;
    };

    HMSTime   time;
    //YMDDate   date;
    Latitude  latitude;
    Longitude longitude;
    Distance  altitude;     // Comes from GGA
    Integer   tracked;      // Comes from GGA
    Integer   inView;      // Comes from GSV
    FixType   fixType;      // Comes from GSA

    void parseField(GPSParserBase::SentenceType sentence, int index, const char *value, int length);
};


class GPSParserSimple : public GPSParserBase {
public:
    GPSParserSimple() : GPSParserBase() { sentences_ok = sentences_err = 0; }

    const GPSFix::Latitude & latitude() { return latest.latitude; }
    const GPSFix::Longitude & longitude() { return latest.longitude; }
    const GPSFix::Distance & altitude() { return latest.altitude; }
    const GPSFix::FixType & fixType()  { return latest.fixType; }
    const GPSFix::HMSTime & fixTime()  { return latest.time; }
    const GPSFix::Integer & tracked()  { return latest.tracked; }
    const GPSFix::Integer & inView()  { return latest.inView; }

    uint32_t sentencesOK() const { return sentences_ok; }
    uint32_t sentencesErr() const { return sentences_err; }

private:
    GPSFix      latest;
    uint32_t    sentences_ok;
    uint32_t    sentences_err;

    virtual void onChecksumError(const char *line);
    virtual void onChecksumOK(const char *line);
    virtual void onFieldData(SentenceType sentence, int index, const char *value, int length);
};
