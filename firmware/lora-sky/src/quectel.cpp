#include "quectel.h"

#include <Arduino.h>

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

static void bang_9600(int pin, const char *data);
    
void gps_set_balloon_mode() {
    const char *cmd_balloon_mode = "$PMTK886,3*2B\r\n";
    
    // Bit bang the data
    bang_9600(GPS_TX_PIN, cmd_balloon_mode);
}

static void bang_9600(int pin, const char *data) {
    const int period = (1000000/9600) - 1;
    
    while (*data) {
        // Send start bit (SPACE)
        digitalWrite(pin, LOW);
        delayMicroseconds(period);
        
        // Go through all bits in the current byte
        uint8_t byte = *data;
        for (uint8_t bit = 0; bit < 8; bit++) {
            // Send data bit
            digitalWrite(pin, (byte & 0x01) ? HIGH : LOW);
            delayMicroseconds(period);
            byte >>= 1;
        }
        
        // Send stop bit (MARK)
        digitalWrite(pin, HIGH);
        delayMicroseconds(period);
        
        data++;
    }
}
