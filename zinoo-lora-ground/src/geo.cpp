#include "geo.h"

#include <Arduino.h>
#include <math.h>

void geo_look_at(
        float lat1, float lng1, float alt1, 
        float lat2, float lng2, float alt2, 
        float &range, float &azim, float &elev)
{
    // Returns line-of-sight distance in meters, azimuth and elevation in degrees between two positions,
    // both specified as degrees latitude and longitude, and altitude in meters above Earth surface.
    // The observer is assumed to be located in (lat1, lng1, alt1).
    // Equations from https://celestrak.com/columns/v02n02/

    // 6372795 average radius ?
    const float Re = 6378135; // equatorial radius in meters
    // Convert to ECI XYZ frame
    float R1 = (Re + alt1) * cos(radians(lat1));
    float z1 = (Re + alt1) * sin(radians(lat1));
    float x1 = R1 * cos(radians(lng1));
    float y1 = R1 * sin(radians(lng1));
    float R2 = (Re + alt2) * cos(radians(lat2));
    float z2 = (Re + alt2) * sin(radians(lat2));
    float x2 = R2 * cos(radians(lng2));
    float y2 = R2 * sin(radians(lng2));
    // Calculate the look vector in ECI XYZ frame
    float rx = x2 - x1;
    float ry = y2 - y1;
    float rz = z2 - z1;
    // Convert the look vector to local horizon frame
    float rs = sin(radians(lat1)) * cos(radians(lng1)) * rx 
                + sin(radians(lat1)) * sin(radians(lng1)) * ry 
                - cos(radians(lat1)) * rz;
    float re = -sin(radians(lng1)) * rx + cos(radians(lng1)) * ry;
    float ru = cos(radians(lat1)) * cos(radians(lng1)) * rx
                + cos(radians(lat1)) * sin(radians(lng1)) * ry
                + sin(radians(lat1)) * rz;

    range = sqrt(rx*rx + ry*ry + rz*rz);
    elev = degrees(asin(ru / range));
    azim = degrees(atan2(re, -rs));
    if (azim < 0) azim += 360;
}