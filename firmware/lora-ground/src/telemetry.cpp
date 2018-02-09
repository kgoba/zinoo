#include "telemetry.h"

#include <string.h>
#include <stdlib.h>

void RemoteData::parse_string(const char *buf) {
    uint8_t field_idx = 0;

    /*  PACKET FORMAT: 
        
        "%s,%d,%02d%02d%02d,%s,%s,%u,%u,%d,%s,%d,%d",
        CALLSIGN, msg_id,
        hour(), minute(), second(),
        lat_str, lng_str, alt, 
        n_sats, 
        temperature_ext, status_str, 
        msg_recv, rssi_last
    */

    while (field_idx < 11) {
        // Search for the next delimiter (COMMA)
        const char *f_end = strchr(buf, ',');
        uint8_t f_len;

        // Calculate field length
        if (f_end) f_len = f_end - buf;
        else f_len = strlen(buf);

        // We only process fields that are not too long
        if (f_len < 10) {
            char field[11];
            field[10] = '\0';   // Safeguard
            strncpy(field, buf, 10);

            switch (field_idx) {
            case 2: /* TIMESTAMP (hhmmss) */
                time.second = strtoul(field + 4, NULL, 10);
                field[4] = '\0';
                time.minute = strtoul(field + 2, NULL, 10);
                field[2] = '\0';
                time.hour = strtoul(field, NULL, 10);
                break;
            case 3: /* LATITUDE (float) */
                if (strlen(field) > 0) 
                    lat = strtod(field, NULL);
                //sscanf(field, "%f", &packet.lat);
                break;
            case 4: /* LONGITUDE (float) */
                if (strlen(field) > 0) 
                    lng = strtod(field, NULL);
                //sscanf(field, "%f", &packet.lng);
                break;
            case 5: /* ALTITUDE (uint16) */
                if (strlen(field) > 0) 
                    //alt = strtoul(field, NULL, 10);
                    alt = strtod(field, NULL);
                //sscanf(field, "%d", &packet.alt);
                break;
            //case 6: /* N_SATS */
            //case 7: /* T_EXT */
            //case 8: /* STATUS */
            case 9: /* MSG_RECV (uint16) */
                msg_recv = strtoul(field, NULL, 10);
                break;
            case 10: /* RSSI_LAST (int8) */
                rssi_last = strtol(field, NULL, 10);
                break;
            }
        }

        // Check for the last field (delimiter not found)
        if (!f_end) break;

        // Update search position
        buf = f_end + 1;    // Skip delimiter
        field_idx++;
    }
}
