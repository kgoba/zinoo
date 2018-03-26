#include "Status.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <TimeLib.h>

void Status::restore() {
    uint16_t test;
    EEPROM.get(0x00, test);

    char saved_callsign[16];
    EEPROM.get(0x00, saved_callsign);

    if (strcmp(CALLSIGN, saved_callsign) != 0) {
        // Fresh EEPROM - reset the contents of status in RAM
        strcpy(saved_callsign, CALLSIGN);
        EEPROM.put(0x00, saved_callsign);
        msg_id = 1;
        save();
    } 
    else {
        //EEPROM.get(0x00, *this);    
        EEPROM.get(0x10, msg_id);
        msg_id += 16;
    }
}

void Status::save() {
    //EEPROM.put(0x00, *this);
    if (msg_id % 16 == 0) {
        EEPROM.put(0x10, msg_id);
    }
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

    uint8_t pyro_percent = 0.5f + 100 * pyro_voltage / 2.7;
    if (pyro_percent > 100) pyro_percent = 100;
    //char vpyro_str[11];
    //dtostrf(100 * pyro_voltage / 2.7, 0, 0, vpyro_str);

    int8_t rssi_slevel = (rssi_last + 127) / 6;
    if (rssi_slevel < 0) rssi_slevel = 0;
    char rssi_str[3];
    rssi_str[0] = 'S';
    rssi_str[1] = (rssi_slevel < 10) ? ('0' + rssi_slevel) : '+';
    rssi_str[2] = '\0';

    char status_str[16];

    uint8_t tmp = 0;
    if (switch_state & 8) status_str[tmp++] = 'B';  // Burst
    if (switch_state & 4) status_str[tmp++] = 'I';  // Ignite
    if (switch_state & 2) status_str[tmp++] = 'C';  // Camera
    if (switch_state & 1) status_str[tmp++] = 'A';  // Aux
    //if (tmp == 0) status_str[tmp++] = '-';
    if (tmp != 0) status_str[tmp++] = ' ';
    status_str[tmp] = '\0';

    snprintf(status_str + tmp, 16 - tmp, "%u %s %u",
        msg_recv, rssi_str, pyro_percent
    );
        
    // Build partial UKHAS sentence (without $$ and checksum)
    // e.g. Z70,90,160900,51.03923,3.73228,31,9,-10
    int buf_req = snprintf(buf, buf_len, "%s,%d,%02d%02d%02d,%s,%s,%u,%u,%d,%s",
        CALLSIGN, msg_id,
        hour(), minute(), second(),
        lat_str, lng_str, alt, 
        n_sats, 
        temperature_ext, status_str
    );

    return (buf_req < buf_len); // true if buf had sufficient space
}
