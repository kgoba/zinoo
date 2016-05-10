#include "suart.h"
#include "util.h"

FIFO_DEFINE(GPSRxQueue, 32, byte);

void nmea_setup() 
{
    
}

struct {
    char time[9];
    char latitude[10];
    char latitudeHemi[1];
    char longitude[11];
    char longitudeHemi[1];
    char fixQuality[1];
    char nSatellites[2];
    char HDOP[4];
    char altitude[6];
    char altitudeUnits[1];
    char height[6];
    char heightUnits[1];
} gGPSInfo;

const char * copyField(const char *line, char *field, byte fieldLength)
{
    const char * separator = strchr(line, ',');
    word nChars = separator - line;
    if (fieldLength < nChars) nChars = fieldLength;
    strncpy(field, line, nChars);
    return separator;
}

char nmea_parseGGA(char *line)
{
    // hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
    for (byte index = 0; index < 12; index++) {
        switch (index) {
            case 0: copyField(line, gGPSInfo.time, ARRAY_SIZE(gGPSInfo.time)); break;
            case 1: copyField(line, gGPSInfo.latitude, ARRAY_SIZE(gGPSInfo.latitude)); break;
            case 2: break;
        }
        line = getField(line);
        if (line == 0) break;        
    }
    return 0;
}

static char gLine[80];
static byte gLineLength;

char nmea_parse(char *line, byte length) 
{
    if (line[0] != '$') return -1;
    if (length < 6) return -1;
    // $GPGGA
    if (strcmp(F"GPGGA", line + 1, 5) == 0) {
        return nmea_parseGGA(line + 6);
    }
    // $GPRMC
    // $GPVTG
    return 0;
}

void nmea_receive(char c) {
    // Check for newline characters
    if (c == 0x0D || c == 0x0A) { 
        
        // If line is not empty, parse it
        if (gLineLength > 0) {
            nmea_parse(gLine, gLineLength);
            gLineLength = 0;
        }
        
        // Skip newline characters
        return;
    }
    
    // Else store the received character
    gLine[gLineLength++] = c;
}


void setup()
{
    suart_setup();
}

void suart_onReceive(byte data, byte isError) 
{
    if (!isError) {
        // Put the received byte in the queue
        FIFO_PUT(GPSRxQueue, data);
    }
}

void loop()
{
    // Put MCU into sleep mode (wake on interrupt)
    sleep_mode();
}

int main(void)
{
    // Setup peripherals
    setup();

    // Enable global interrupts
    sei();
    
    // Enter endless loop
    while (1) {
        loop();
    }
    return 0;
}
