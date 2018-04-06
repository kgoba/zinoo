#include "telemetry.h"

#include <cstdio>
#include <cstdlib>

#include "strconv.h"

// void TeleMessage::restore() {
//     uint16_t test;
//     EEPROM.get(0x00, test);

//     char saved_callsign[16];
//     EEPROM.get(0x00, saved_callsign);

//     if (strcmp(CALLSIGN, saved_callsign) != 0) {
//         // Fresh EEPROM - reset the contents of status in RAM
//         strcpy(saved_callsign, CALLSIGN);
//         EEPROM.put(0x00, saved_callsign);
//         msg_id = 1;
//         save();
//     } 
//     else {
//         //EEPROM.get(0x00, *this);    
//         EEPROM.get(0x10, msg_id);
//         msg_id += 16;
//     }
// }

// void TeleMessage::save() {
//     //EEPROM.put(0x00, *this);
//     if (msg_id % 16 == 0) {
//         EEPROM.put(0x10, msg_id);
//     }
// }

// /// Constructs payload message and transmits it via radio
bool TeleMessage::build_string(char *buf, int &buf_len) {
    char lat_str[12];
    char lng_str[12];
    char alt_str[8];

    if (fixValid) {
        int lat_degrees = lat;
        int lat_fraction = abs((int)(0.5f + (lat - lat_degrees) * 100000));

        int lng_degrees = lng;
        int lng_fraction = abs((int)(0.5f + (lng - lng_degrees) * 100000));
        sprintf(lat_str, "%d.%05d", lat_degrees, lat_fraction);
        sprintf(lng_str, "%d.%05d", lng_degrees, lng_fraction);
        sprintf(alt_str, "%d", (int)(0.5f + alt));
    } else {
        lat_str[0] = '\0';  // Empty latitude field in case fix is invalid
        lng_str[0] = '\0';  // Empty longitude field in case fix is invalid
        alt_str[0] = '\0';  // Empty altitude field
    }

    //uint8_t pyro_percent = 0.5f + 100 * pyro_voltage / 2.7;
    //if (pyro_percent > 100) pyro_percent = 100;
//     //char vpyro_str[11];
//     //dtostrf(100 * pyro_voltage / 2.7, 0, 0, vpyro_str);

//     int8_t rssi_slevel = (rssi_last + 127) / 6;
//     if (rssi_slevel < 0) rssi_slevel = 0;
//     char rssi_str[3];
//     rssi_str[0] = 'S';
//     rssi_str[1] = (rssi_slevel < 10) ? ('0' + rssi_slevel) : '+';
//     rssi_str[2] = '\0';
    char pyro_voltage_str[5];
    char battery_voltage_str[5];

    //dtostrf(pyro_voltage, 0, 2, pyro_voltage_str);
    //dtostrf(battery_voltage, 0, 2, battery_voltage_str);

    char status_str[4];

    uint8_t tmp = 0;
    if (pyro_state & 1) status_str[tmp++] = '1';  // Pyro 1
    if (pyro_state & 2) status_str[tmp++] = '2';  // Pyro 2
    if (tmp == 0) status_str[tmp++] = '-';
    //if (tmp != 0) status_str[tmp++] = ' ';
    status_str[tmp] = '\0';
    
    // Build partial UKHAS sentence (without $$ and checksum)
    // e.g. Z70,90,160900,51.03923,3.73228,31,9,-10
    int buf_req = snprintf(buf, buf_len, "%s,%d,%02d%02d%02d,%s,%s,%s,%d,%d,%s", 
        callsign, msg_id,
        hour, minute, second,
        lat_str, lng_str, alt_str, n_sats,
        temperature_int, 
        //pyro_voltage_str, battery_voltage_str,
        status_str
    );

    if (buf_req < buf_len) {
        buf_len = buf_req;
        return true;
    }
    return false;
    //return (buf_req < buf_len); // true if buf had sufficient space
}
