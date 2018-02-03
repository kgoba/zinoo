#include "config.h"
#include "display.h"

#include "geo.h"

#include <TinyGPS++.h>
#include <RH_RF95.h>

int16_t TimeHMS::delta_i16(const TimeHMS &t1, const TimeHMS &t2) {
    int16_t delta = 0;
    delta += (int16_t)t2.hour - t1.hour;
    delta *= 60;
    delta += (int16_t)t2.minute - t1.minute;
    delta *= 60;
    delta += (int16_t)t2.second - t1.second;
    return delta;
}

void DisplayInfo::update_remote_position(float new_lat, float new_lng, float new_alt, TimeHMS new_time) {
    // Distance travelled horizontally since last update (2D path across sphere)
    float hdist = TinyGPSPlus::distanceBetween(new_lat, new_lng, lat, lng);
    // Distance travelled vertically since last update (altitude change)
    float vdist = new_alt - (float)alt;
    // Time in seconds since the last update
    int16_t time_delta = TimeHMS::delta_i16(time, new_time);
    if (time_delta) {
        hspeed = hdist / time_delta;
        vspeed = vdist / time_delta;
    }
    else {
        hspeed = vspeed = 0;
    }
    time = new_time;
    hdg = TinyGPSPlus::courseTo(new_lat, new_lng, lat, lng);

    lat = new_lat;
    lng = new_lng;
    alt = new_alt;

    update_azim_elev();
}

void DisplayInfo::update_local_position(float new_lat, float new_lng, float new_alt) {
    loc_lat = new_lat;
    loc_lng = new_lng;
    loc_alt = new_alt;

    update_azim_elev();
}

void DisplayInfo::update_azim_elev() {
    float f_range, f_azim, f_elev;
    geo_look_at(loc_lat, loc_lng, loc_alt, lat, lng, alt, f_range, f_azim, f_elev);
    range = f_range / 1000.0;
    azim = (f_azim + 0.5);
    elev = (f_elev + 0.5);
    /*
    Serial.print("RNG "); Serial.print(gFields.range / 1000.0);
    Serial.print(" AZ "); Serial.print(gFields.azim);
    Serial.print(" EL "); Serial.print(gFields.elev);
    Serial.println();
    */
}

DisplayInfo gFields;

#ifdef WITH_TFT

#include <TFT.h>
TFT         display(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

enum FieldType {
    eFLD_INT8,
    eFLD_UINT16,
    eFLD_HMS,
    eFLD_FLOAT_LAT,
    eFLD_FLOAT_LNG,
    eFLD_FLOAT
};

struct FieldDescriptor {
    const char  *label;
    uint8_t     column;
    uint8_t     row;
    uint8_t     width;
    const char  *units;
    const void  *data;
    FieldType   type;
};

const FieldDescriptor kFields[] = {
    // "MSG  1345        LAST 52s"
    {"MSG",   4,  6, 5, NULL,  &gFields.msg_recv, eFLD_UINT16},
    {"LAST", 15,  6, 3, "dBm", &gFields.rssi,     eFLD_INT8},
    {NULL,   22,  6, 2, "s",   &gFields.msg_age,  eFLD_UINT16},
    // "56.95790  24.13918  12345"
    {NULL,    0,  7, 8, NULL,  &gFields.lat,    eFLD_FLOAT_LAT},
    {NULL,    9,  7, 9, NULL,  &gFields.lng,    eFLD_FLOAT_LNG},
    {NULL,   19,  7, 5, "m",   &gFields.alt,    eFLD_FLOAT},
    // "H/V +13m/s -16m/s HDG 234"
    {"H/V",   4,  8, 3, NULL,  &gFields.hspeed, eFLD_FLOAT},
    {"/",    10,  8, 4, "m/s", &gFields.vspeed, eFLD_FLOAT},
    {"HDG",  22,  8, 3, NULL,  &gFields.hdg,    eFLD_UINT16},
    // "RNG 19.8km  AZ 231  EL 78"
    {"RNG",   4,  9, 4, "km",  &gFields.range,  eFLD_FLOAT},
    {"AZ",   15,  9, 3, NULL,  &gFields.azim,   eFLD_UINT16},
    {"EL",   23,  9, 2, NULL,  &gFields.elev,   eFLD_INT8},
    // "56.95790N 124.13918E 12345m"
    {NULL,    0, 10, 8, NULL,  &gFields.loc_lat,    eFLD_FLOAT_LAT},
    {NULL,    9, 10, 9, NULL,  &gFields.loc_lng,    eFLD_FLOAT_LNG},
    {NULL,   19, 10, 5, "m",   &gFields.loc_alt,    eFLD_FLOAT},
    // "SAT 25           12:13:15"  
    {"SAT",   4, 11, 2, NULL,  &gFields.loc_fix,    eFLD_INT8},
    {NULL,   17, 11, 8, NULL,  &gFields.loc_time,   eFLD_HMS}
};

void tft_print_multiline(const char *text, uint8_t row) {
    const uint16_t max_width = 25;
    char line[max_width + 1];
    line[max_width] = '\0';

    uint16_t x = 5;
    uint16_t y = 4 + 10 * row;

    strncpy(line, text, max_width);

    ATOMIC_BLOCK_START;
    display.stroke(255, 255, 255);
    display.fill(255, 255, 255);
    display.rect(x, y, 6 * max_width, 8);
    display.stroke(0, 0, 0);
    display.text(line, x, y);

    uint16_t len = strlen(text);
    while (len >= max_width) {
        text += max_width;
        y += 10;
        len -= max_width;
        strncpy(line, text, max_width);
        
        display.stroke(255, 255, 255);
        display.fill(255, 255, 255);
        display.rect(x, y, 6 * max_width, 8);
        display.stroke(0, 0, 0);
        display.text(line, x, y);
    }
    ATOMIC_BLOCK_END;
}

void tft_setup() {
    display.begin();
    display.setRotation(1);
    display.background(255,255,255);  // clear the screen
  
	char line[40] = "LoRa RX ";

	char str[40];
    dtostrf(FREQUENCY_MHZ, 0, 3, str);
	strcat(line, str);

    display.stroke(0, 0, 0);
    display.text(line, 45, 4);
    display.line(0, 14, 159, 14);	

    //tft_print_multiline("$$RKN1,999,150251,56.95790,24.13918,27654,21,11,13*CRC16", 5, 4 + 2 * 10);

    // Draw static labels and units
    for (uint8_t i_f = 0; i_f < sizeof(kFields) / sizeof(kFields[0]); i_f++) {
        if (kFields[i_f].label) {
            uint8_t label_len = strlen(kFields[i_f].label);
            uint8_t label_col = kFields[i_f].column - label_len - 1;
            display.text(kFields[i_f].label, 5 + 6 * label_col, 4 + 10 * kFields[i_f].row);
        }
        if (kFields[i_f].units) {
            uint8_t units_col = kFields[i_f].column + kFields[i_f].width;
            display.text(kFields[i_f].units, 5 + 6 * units_col, 4 + 10 * kFields[i_f].row);
        }
    }

    //display.line(0, 53, 159, 53);
    display.line(0, 102, 159, 102);
}

void tft_update() {
    for (uint8_t i_f = 0; i_f < sizeof(kFields) / sizeof(kFields[0]); i_f++) {
        const FieldDescriptor &desc = kFields[i_f];

        char str[17];
        str[0] = '-';
        str[1] = str[16] = '\0';
        switch(desc.type) {
        case eFLD_INT8:
            //itoa(*(const int8_t *)desc.data, str, 10);
            sprintf(str, "%d", *(const int8_t *)desc.data);
            break;
        case eFLD_UINT16:
            //utoa(*(const uint16_t *)desc.data, str, 10);
            sprintf(str, "%d", *(const uint16_t *)desc.data);
            break;
        case eFLD_HMS: {
            const TimeHMS *ptr = (const TimeHMS *)desc.data;
            //sprintf(str, "%02d:%02d:%02d", ptr[0], ptr[1], ptr[2]);
            sprintf(str,     "%02d", (uint16_t)ptr->hour);
            sprintf(str + 3, "%02d", (uint16_t)ptr->minute);
            sprintf(str + 6, "%02d", (uint16_t)ptr->second);
            str[2] = str[5] = ':';
            break;
        }
        case eFLD_FLOAT_LAT: {
            const float value = *(const float *)desc.data;
            const float absval = (value < 0) ? -value : value;
            dtostrf(absval, 0, 4, str);
            strcat(str, (value < 0) ? "S" : "N");
            break;
        }
        case eFLD_FLOAT_LNG: {
            const float value = *(const float *)desc.data;
            const float absval = (value < 0) ? -value : value;
            dtostrf(absval, 0, 4, str);
            strcat(str, (value < 0) ? "W" : "E");
            break;
        }
        case eFLD_FLOAT: {
            const float value = *(const float *)desc.data;
            const float absval = (value < 0) ? -value : value;
            int8_t prec = 0;
            if (absval < 10.0) prec = desc.width - 2;
            else if (absval < 100.0) prec = desc.width - 3;
            else if (absval < 1000.0) prec = desc.width - 4;
            if (value < 0) prec--;

            if (prec >= 0) dtostrf(value, 0, prec, str);
            break;
        }
        default:
            break;            
        }

    	ATOMIC_BLOCK_START;
        uint16_t x = 5 + 6 * desc.column;
        uint16_t y = 4 + 10 * desc.row;
        if (strlen(str) > desc.width) {
            display.stroke(255, 0, 0);
            display.fill(255, 0, 0);
            display.rect(x, y, 6 * desc.width, 8);
        }
        else {
            display.stroke(255, 255, 255);
            display.fill(255, 255, 255);
            display.rect(x, y, 6 * desc.width, 8);

            display.stroke(0, 0, 0);
            display.text(str, x + 6 * (desc.width - strlen(str)), y);
        }
    	ATOMIC_BLOCK_END;
    }
}

#endif

#ifdef WITH_LCD
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C display(0x27,16,2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void tft_setup() {
    display.init(); //initialize the lcd
    display.backlight(); //open the backlight 
}

void tft_print_multiline(const char *text, uint8_t row) {
    //display.setCursor(15, 0);
    //display.print("TEST");
}

void tft_update() {
    display.clear();

    char str[16];
    uint8_t prec;

    if (digitalRead(SWITCH1_PIN)) {
        // LOCAL DATA
        //display.setCursor(0, 0);
        if (gFields.loc_fix >= 3) {
            display.setCursor(0, 0);
            display.print("RANGE");
            display.setCursor(7, 0);
            display.print("AZ");
            display.setCursor(10, 0);
            display.print("EL");

            // RANGE
            display.setCursor(0, 1);
            if (gFields.range >= 10) {
                dtostrf(gFields.range, 0, 0, str);
                display.print(str);
                display.print("km");
            }
            else {
                dtostrf(gFields.range * 1000, 0, 0, str);
                display.print(str);
                display.print("m");            
            }

            // AZIM
            display.setCursor(6, 1);
            display.print(gFields.azim);

            // ELEV
            display.setCursor(10, 1);
            display.print(gFields.elev);
        }
        else {
            display.print("NO FIX");
        }
        display.setCursor(13, 0);
        display.print("SAT");
        display.setCursor(15, 1);
        display.print((gFields.loc_fix > 9) ? 9 : gFields.loc_fix);
    }
    else {
        // REMOTE DATA
        //display.setCursor(0, 0);    // COL, ROW
        if (gFields.msg_recv > 0) {
            // LATITUDE
            dtostrf(gFields.lat, 0, 4, str);
            display.print(str);
            //display.print((gFields.lat > 0) ? 'N' : 'S');

            // LONGITUDE
            dtostrf(gFields.lng, 0, 4, str);
            display.setCursor(16 - strlen(str), 0);    // COL, ROW
            display.print(str);
            //display.print((gFields.lng > 0) ? 'E' : 'W');

            // ALTITUDE
            display.setCursor(0, 1);    // COL, ROW
            dtostrf(gFields.alt, 0, 0, str);
            display.print(str);
            //display.print(gFields.alt);
            display.print('m');

            display.setCursor(10, 1);    // COL, ROW
            //uint8_t hour_local = gFields.time.hour + LOCAL_TIME_OFFSET_HOURS;
            //if (hour_local > 24) hour_local -= 24;
            sprintf(str, "%02d%02d%02d", gFields.time.hour, gFields.time.minute, gFields.time.second);
            display.print(str);
        }
        else {
            display.print("NO DATA");
        }
    }
}

#endif
