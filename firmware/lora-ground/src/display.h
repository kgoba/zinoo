#pragma once

#include <stdint.h>

#include "geo.h"

struct DisplayInfo {
    int8_t   loc_fix;        // 0 5 12  
    float    loc_lat;
    float    loc_lng;
    float    loc_alt; 
    TimeHMS  loc_time;
    
    float    lat;     // -89.12345
    float    lng;    // -150.12345
    float    alt;    // 12345
    TimeHMS  time;

    uint16_t hdg;    // 0 90 359
    float    vspeed;   // 0 -5 -12 +18
    float    hspeed;   // 0 -5 -12 +18
    float    range;    // 0.00 0.99 9.99 12.3 34.9 (kilometers)
    uint16_t azim;  // 0 90 359
    int8_t   elev;    // -9 0 14 37

    uint16_t msg_recv;
    uint16_t msg_age;
    int8_t   rssi;

    void update_remote_position(float new_lat, float new_lng, float new_alt, TimeHMS new_time);
    void update_local_position(float loc_lat, float loc_lng, float loc_alt);
    void update_local_time(TimeHMS new_time);

    void update_azim_elev();
};

extern DisplayInfo gFields;

void tft_setup();
void tft_update();
void tft_print_multiline(const char *text, uint8_t row);
