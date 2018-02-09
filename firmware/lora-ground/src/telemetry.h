#pragma once

#include <stdint.h>

#include "geo.h"

struct RemoteData {
public:
    //char     callsign[8];
    uint16_t msg_id;            // Message identifier
    
    TimeHMS  time;              // Timestamp (hour/minute/second) of position
    float    lat, lng;          // Latitude/longitude in degrees
    float    alt;               // Altitude in meters

    uint8_t  n_sats;            // Current satellites
    int8_t   temperature_ext;   // External temperature, Celsius
    uint8_t  switch_state;      // Bit field of switch states

    uint16_t msg_recv;  // Number of received messages
    int8_t   rssi_last; // RSSI of the last rx message

    void parse_string(const char *buf);
};
