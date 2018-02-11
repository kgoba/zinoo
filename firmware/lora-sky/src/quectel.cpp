#include <Arduino.h>

#include "quectel.h"

#include "config.h"

/*

Packet Type: 886 PMTK_FR_MODE
This message is used to set navigation mode.

Data Field: $PMTK886,CmdType 

Example:
    Query:    $PMTK886,3*2B<CR><LF>
    Response: $PMTK001,886,3*36

Field           Description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
$               Each NMEA message starts with ‘$’ 
PMTK            MTK proprietary message
Packet Type     886
CmdType         ‘0’=Normal mode: For general purpose
                ‘1’=Fitness mode: For running and walking purpose 
                        that the low-speed (<5m/s) movement will 
                        have more effect on the position calculation.
                ‘2’=Aviation mode: For high-dynamic purpose that the 
                        large-acceleration movement will have more 
                        effect on the position calculation.
                ‘3’=Balloon mode: For high-altitude balloon purpose 
                        that the vertical movement will have more 
                        effect on the position calculation.
*               End character of data field
Checksum        Hexadecimal checksum
<CR><LF>        Each of message

*/

static void gps_bang_9600(int pin, const char *data);
static int gps_readline(char *line, int length);

bool gps_set_balloon_mode() {
    const char *cmd_balloon_mode = "$PMTK886,3*2B\x0D\x0A";

    char line[80];

    // Configure TX pin and set line in idle state (MARK)
    pinMode(GPS_TX_PIN, OUTPUT);
    digitalWrite(GPS_TX_PIN, HIGH);

    for (uint8_t n_try1 = 0; n_try1 < 10; n_try1++) {
        gps_readline(line, 80);
        Serial.println("Sending PMTK 868 command...");
        // Bit bang the data
        gps_bang_9600(GPS_TX_PIN, cmd_balloon_mode);
        for (uint8_t n_try2 = 0; n_try2 < 10; n_try2++) {
            gps_readline(line, 80);
            Serial.print("Got response: ");
            Serial.print(line);
            if (strcmp(line, "$PMTK001,886,3*36\x0D\x0A") == 0) {
                return true;
            }
        }
    }
    return false;
}

static int gps_readline(char *line, int length) {
    uint8_t count = 0;
    while (true) {
      if (Serial.available()) {
          char c = Serial.read();
          if (length > 1) {
              *line++ = c;
              length--;
          }
          count++;
          if (c == '\x0A') break;   // <LF> is the last char in string
      }
    }
    if (length > 0) {
        *line = '\0';
    }
    return count;
}

static void gps_bang_9600(int pin, const char *data) {
    const int period = (1000000/9600) - 1;
    
    while (*data) {
        // Send start bit (SPACE)
        digitalWrite(pin, LOW);
        delayMicroseconds(period);
        
        // Go through all bits in the current byte
        uint8_t b = *data;
        for (uint8_t bit = 0; bit < 8; bit++) {
            // Send data bit
            digitalWrite(pin, (b & 0x01) ? HIGH : LOW);
            delayMicroseconds(period);
            b >>= 1;
        }
        
        // Send stop bit (MARK)
        digitalWrite(pin, HIGH);
        delayMicroseconds(period);
        
        data++;
    }
}
