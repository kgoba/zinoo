#pragma once

#include <stdint.h>

// Class to keep track of our last known status (telemetry data)
struct TeleMessage {
    char     callsign[16];
    uint16_t msg_id;

    uint8_t  hour, minute, second;

    float    lat, lng;  // Last valid coordinates, degrees
    float    alt;       // Last valid altitude, meters
    uint8_t  n_sats;    // Number of satellites used for GNSS solution
    bool     fixValid;

    int8_t   temperature_int;   // Internal temperature, Celsius

    float    alt_baro;          // Barometric altitude, meters

    uint16_t battery_voltage;   // millivolts
    uint16_t pyro_voltage;      // millivolts

    uint8_t  pyro_state;

    uint16_t msg_recv;
    int8_t   rssi_last;

    void restore();
    void save();

    bool build_string(char *buf, int &buf_len);
};
