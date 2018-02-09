#pragma once

#include <stdint.h>

void geo_look_at(
        float lat1, float lng1, float alt1, 
        float lat2, float lng2, float alt2, 
        float &range, float &azim, float &elev);


struct TimeHMS {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    static int16_t delta_i16(const TimeHMS &t1, const TimeHMS &t2);

};