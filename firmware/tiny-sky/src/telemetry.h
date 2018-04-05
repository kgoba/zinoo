#pragma once

#include <stdint.h>

// Class to keep track of our last known status (telemetry data)
struct TeleMessage {
    uint16_t msg_id;

    float    lat, lng;  // Last valid coordinates, degrees
    uint16_t alt;       // Last valid altitude, meters
    uint8_t  n_sats;    // Number of satellites used for GNSS solution
    bool     fixValid;

    int8_t   temperature_int;   // Internal temperature, Celsius

    uint16_t alt_baro;       // Barometric altitude, meters

    float    battery_voltage;
    float    pyro_voltage;

    uint8_t  pyro_state;

    uint16_t msg_recv;
    int8_t   rssi_last;

    void restore();
    void save();

    bool build_string(char *buf, uint8_t buf_len);
};
