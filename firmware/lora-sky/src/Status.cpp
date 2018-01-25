#include "Status.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <TimeLib.h>

void Status::restore() {
    //EEPROM.get(0x00, status);
    //if (status.msg_id == 0xFFFF) {
        // Fresh EEPROM - reset the contents of status in RAM
        msg_id = 1;
        fixValid = false;
        lat = lng = 0;
        alt = n_sats = 0;
        save();
    //}        
}

void Status::save() {
    //EEPROM.put(0x00, status);
}

/// Constructs payload message and transmits it via radio
bool Status::build_string(char *buf, uint8_t buf_len) {
    char lat_str[11];
    char lng_str[11];

    if (fixValid) {
        dtostrf(lat, 0, 5, lat_str);
    } else {
        lat_str[0] = '\0';  // Empty latitude field in case fix is invalid
    }
    if (fixValid) {
        dtostrf(lng, 0, 5, lng_str);        
    } else {
        lng_str[0] = '\0';  // Empty longitude field in case fix is invalid
    }
        
    // Build partial UKHAS sentence (without $$ and checksum)
    // e.g. Z70,90,160900,51.03923,3.73228,31,9,-10
    int buf_req = snprintf(buf, buf_len, "%s,%d,%02d%02d%02d,%s,%s,%u,%d,%d",
        CALLSIGN, msg_id,
        hour(), minute(), second(),
        lat_str, lng_str, alt, 
        n_sats, 
        temperature_ext
    );

    return (buf_req < buf_len); // true if buf had sufficient space
}
