#include <iostream>
#include <string>
#include <cmath>

using namespace std;

#define radians(x)      ((x) * M_PI / 180.0)
#define degrees(x)      ((x) * 180.0 / M_PI)
#define TWO_PI          (2 * M_PI)

void get_look_at(
        double lat1, double lng1, double alt1, 
        double lat2, double lng2, double alt2, 
        double &range, double &azim, double &elev)
{
    // returns line-of-sight distance in meters between two positions, both specified
    // as signed decimal-degrees latitude and longitude, as well as altitude in meters above 
    // a sphere of radius 6372795 meters.

    // 6372.795 * 1000 ?
    const double Re = 6378.135 * 1000; // equatorial radius 
    double R1 = (Re + alt1) * cos(radians(lat1));
    double z1 = (Re + alt1) * sin(radians(lat1));
    double x1 = R1 * cos(radians(lng1));
    double y1 = R1 * sin(radians(lng1));

    double R2 = (Re + alt2) * cos(radians(lat2));
    double z2 = (Re + alt2) * sin(radians(lat2));
    double x2 = R2 * cos(radians(lng2));
    double y2 = R2 * sin(radians(lng2));

    double rx = x2 - x1;
    double ry = y2 - y1;
    double rz = z2 - z1;

    double rs = sin(radians(lat1)) * cos(radians(lng1)) * rx 
                + sin(radians(lat1)) * sin(radians(lng1)) * ry 
                - cos(radians(lat1)) * rz;
    double re = -sin(radians(lng1)) * rx + cos(radians(lng1)) * ry;
    double ru = cos(radians(lat1)) * cos(radians(lng1)) * rx
                + cos(radians(lat1)) * sin(radians(lng1)) * ry
                + sin(radians(lat1)) * rz;

    cerr << "rs = " << rs;
    cerr << " re = " << re;
    cerr << " ru = " << ru << endl;

    range = sqrt(rx*rx + ry*ry + rz*rz);
    elev = degrees(asin(ru / range));
    azim = degrees(atan2(re, -rs));
    if (azim < 0) azim += 360;
}

int main() {
    double lat1, lng1, alt1;
    double lat2, lng2, alt2;
    double range, azim, elev;

    cin >> lat1 >> lng1 >> alt1;
    cin >> lat2 >> lng2 >> alt2;

    get_look_at(lat1, lng1, alt1, lat2, lng2, alt2, range, azim, elev);

    string delim = ", ";
    cout << range << delim << azim << delim << elev << endl;
}
