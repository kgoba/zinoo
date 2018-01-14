/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017-2018 Karlis Goba
*/

// All hardware and software configuration is in config.h
#include "config.h"

#include <RH_RF95.h>
#include <TinyGPS++.h>

#include "tone.h"
#include "geo.h"

struct TimeHMS {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    static int16_t delta_i16(const TimeHMS &t1, const TimeHMS &t2);
};

int16_t TimeHMS::delta_i16(const TimeHMS &t1, const TimeHMS &t2) {
    int16_t delta = 0;
    delta += (int16_t)t2.hour - t1.hour;
    delta *= 60;
    delta += (int16_t)t2.minute - t1.minute;
    delta *= 60;
    delta += (int16_t)t2.second - t1.second;
    return delta;
}

struct RemoteData {
public:
    //char     callsign[8];
    //uint16_t msg_id;
    TimeHMS  time;
    float    lat;
    float    lng;
    uint16_t alt;

    void parse_string(const char *buf);
};

RemoteData  gLastPacket;

RH_RF95     lora(LORA_CS_PIN);
uint8_t     gLastPacketRaw[LORA_MAX_MESSAGE_LEN + 6];
TinyGPSPlus gps;

#if USE_TFT
#include <TFT.h>
TFT 	    display(TFT_CS, TFT_DC, TFT_RST);
#endif


void setup() {
    Serial.begin(9600);
    //Serial.println("RESET");
		
#if USE_TFT
	tft_setup();    // Initialize TFT and display static content
#endif

	lora_setup();   // Initialize LoRa module
	tone_setup();   // Setup GPS PPS synchronized timer (uses 16-bit Timer1)

	pinMode(BUTTON1_PIN, INPUT_PULLUP);
	pinMode(BUTTON2_PIN, INPUT_PULLUP);    
}


#if USE_TFT
struct DisplayInfo {
    TimeHMS  loc_time;
    int8_t   loc_fix;        // 0 5 12  
    float    loc_lat;
    float    loc_lng;
    uint16_t loc_alt; 
    
    float    lat;     // -89.12345
    float    lng;    // -150.12345
    uint16_t alt;    // 12345
    TimeHMS  time;

    uint16_t hdg;    // 0 90 359
    float    vspeed;   // 0 -5 -12 +18
    float    hspeed;   // 0 -5 -12 +18
    float    range;    // 0.00 0.99 9.99 12.3 34.9
    uint16_t azim;  // 0 90 359
    int8_t   elev;    // -9 0 14 37
    uint16_t msg_recv;
    uint16_t msg_age;
    int8_t   rssi;
};

DisplayInfo gFields;

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
    {NULL,   19,  7, 5, "m",   &gFields.alt,    eFLD_UINT16},
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
    {NULL,   19, 10, 5, "m",   &gFields.loc_alt,    eFLD_UINT16},
    // "SAT 25           12:13:15"  
    {"SAT",   4, 11, 2, NULL,  &gFields.loc_fix,    eFLD_INT8},
    {NULL,   17, 11, 8, NULL,  &gFields.loc_time,   eFLD_HMS}
};

static void tft_print_multiline(const char *text, uint8_t row) {
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
    display.setRotation(3);
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

void lora_setup() {
	pinMode(LORA_CS_PIN, OUTPUT);
    digitalWrite(LORA_CS_PIN, HIGH); 

    // reset LoRa module first to make sure it will works properly
    pinMode(LORA_RST_PIN, OUTPUT);
    digitalWrite(LORA_RST_PIN, LOW);   
    delay(1000);
    digitalWrite(LORA_RST_PIN, HIGH); 
    delay(100);
  
    if (!lora.init()) {
        Serial.println("LoRa init failed");
    }
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    lora.setModemConfig(MODEM_MODE);
    lora.setFrequency(FREQUENCY_MHZ);	
	
	char str[40];
    dtostrf(FREQUENCY_MHZ, 0, 3, str);
	//Serial.print("LoRa frequency: ");
	//Serial.println(str);
}

void loop() {
    static uint32_t next_GPS_update = 0;
	static uint32_t next_report = 0;
    bool newGPSData = false;   // Did a new valid sentence come in?

    // Feed data to GPS parser
    while (Serial.available()) {
        char c = Serial.read();
        if (gps.encode(c)) {
            newGPSData = true; 
        }
    }
    
    // Check for pending received messages on the radio
    if (lora.available()) {
        if (lora_receive()) {
#if USE_TFT
            gFields.msg_recv++;
            gFields.rssi = lora.lastRssi();

            tft_print_multiline((const char *)gLastPacketRaw, 2);

            gLastPacket.parse_string((const char *)gLastPacketRaw);

            // Distance travelled horizontally since last update (2D path across sphere)
            float hdist = gps.distanceBetween(gLastPacket.lat, gLastPacket.lng, gFields.lat, gFields.lng);
            // Distance travelled vertically since last update (altitude change)
            float vdist = (float)gLastPacket.alt - (float)gFields.alt;
            // Time in seconds since the last update
            int16_t time_delta = TimeHMS::delta_i16(gFields.time, gLastPacket.time);
            if (time_delta) {
                gFields.hspeed = hdist / time_delta;
                gFields.vspeed = vdist / time_delta;
            }
            else {
                gFields.hspeed = gFields.vspeed = 0;
            }
            gFields.time = gLastPacket.time;
            gFields.hdg = gps.courseTo(gLastPacket.lat, gLastPacket.lng, gFields.lat, gFields.lng);

            gFields.lat = gLastPacket.lat;
            gFields.lng = gLastPacket.lng;
            gFields.alt = gLastPacket.alt;
            float range, azim, elev;
            geo_look_at(gFields.loc_lat, gFields.loc_lng, gFields.loc_alt, 
                gFields.lat, gFields.lng, gFields.alt,
                range, azim, elev);
            gFields.range = range / 1000.0;
            gFields.azim = (azim + 0.5);
            gFields.elev = (elev + 0.5);
#endif
        }
    }
	
#if USE_TFT
    if (newGPSData) {
        if (gps.location.isUpdated() || gps.altitude.isUpdated()) {
            gFields.loc_lat = gps.location.lat();
            gFields.loc_lng = gps.location.lng();
            gFields.loc_alt = gps.altitude.meters();
            float range, azim, elev;
            geo_look_at(gFields.loc_lat, gFields.loc_lng, gFields.loc_alt, 
                gFields.lat, gFields.lng, gFields.alt,
                range, azim, elev);
            gFields.range = range / 1000.0;
            gFields.azim = (azim + 0.5);
            gFields.elev = (elev + 0.5);

            /*
            Serial.print("RNG "); Serial.print(gFields.range / 1000.0);
            Serial.print(" AZ "); Serial.print(gFields.azim);
            Serial.print(" EL "); Serial.print(gFields.elev);
            Serial.println();
            */
        }
        if (gps.satellites.isUpdated()) {
            gFields.loc_fix = gps.satellites.value();
        }
        if (gps.time.isValid()) {
            gFields.loc_time.hour = gps.time.hour();
            gFields.loc_time.minute = gps.time.minute();
            gFields.loc_time.second = gps.time.second();
        }
    }
#endif
    
	if (millis() > next_report) {
		next_report += 1000;
		
        tft_update();

    /*
		//Serial.print(" Period="); 
		Serial.print(gTonePeriod); Serial.print(' ');
		//Serial.print(" Comp="); 
		Serial.print(gToneCompensation); Serial.print(' ');
		//Serial.print(" F="); Serial.print(gToneFrequency);
		//Serial.print(" TCNT1="); 
		Serial.print(gTonePhase); Serial.print(' ');
		//Serial.print(gToneDelta); Serial.print(' ');
		Serial.println();
    */
	}
	
    // Report our GPS location if necessary
    if (newGPSData && (millis() > next_GPS_update) && gps.location.isValid()) {
        next_GPS_update = millis() + (GPS_UPDATE_INTERVAL * 1000UL);

        float falt = 0;
        if (gps.altitude.isValid()) falt = gps.altitude.meters();
    
        char str[40], lat_str[11], lng_str[11], alt_str[8];
        dtostrf(gps.location.lat(), 0, 5, lat_str);
        dtostrf(gps.location.lng(), 0, 5, lng_str);
        dtostrf(falt, 0, 0, alt_str);
        sprintf(str, "**%02d%02d%02d,%s,%s,%s,%u,%lu", 
            gps.time.hour(), gps.time.minute(), gps.time.second(),
            lat_str, lng_str, alt_str, (uint8_t)gps.satellites.value(), gps.location.age());
		//Serial.println(str);
    }
	
	// Check buttons for uplink commands
	uint8_t status = 0; 
	if (digitalRead(BUTTON1_PIN) == LOW) status |= 1;
	if (digitalRead(BUTTON2_PIN) == LOW) status |= 2;
	if (false) {
	    char tx_buf[3];
		tx_buf[0] = 'S';
		tx_buf[1] = ('0' + status);
		tx_buf[2] = '\0';
	    lora.send((const uint8_t *)tx_buf, strlen(tx_buf));
	}
}

void RemoteData::parse_string(const char *buf) {
    uint8_t field_idx = 0;

    while (field_idx < 6) {
        char *f_end = strchr(buf, ',');
        uint8_t f_len;

        if (f_end) f_len = f_end - buf;
        else f_len = strlen(buf);

        if (f_len < 10) {
            char field[11];
            field[10] = '\0';
            strncpy(field, buf, 10);
            switch (field_idx) {
            case 2:
                time.second = strtoul(field + 4, NULL, 10);
                field[4] = '\0';
                time.minute = strtoul(field + 2, NULL, 10);
                field[2] = '\0';
                time.hour = strtoul(field, NULL, 10);
                break;
            case 3:
                lat = strtod(field, NULL);
                //sscanf(field, "%f", &packet.lat);
                break;
            case 4:
                lng = strtod(field, NULL);
                //sscanf(field, "%f", &packet.lng);
                break;
            case 5:
                alt = strtoul(field, NULL, 10);
                //sscanf(field, "%d", &packet.alt);
                break;
            }
        }

        if (!f_end) break;
        buf = f_end + 1;
        field_idx++;
    }
}

bool lora_receive() {    
    // Copy the received message to RAM
    uint8_t *buf = gLastPacketRaw;
    uint8_t len = LORA_MAX_MESSAGE_LEN;
    if (lora.recv(buf, &len)) {
        // Add zero termination to make a valid C string
        char *str = (char *)buf;
        str[len] = '\0';
          
        // Calculate UKHAS checksum
        char chksum_str[8];
        sprintf(chksum_str, "*%04X", gps_CRC16_checksum(buf));
        //strcat((char *)buf, chksum_str);

        Serial.print("$$"); Serial.print(str); Serial.println(chksum_str);
        Serial.print("  RSSI="); Serial.print(lora.lastRssi(), DEC);
        Serial.println();
        return true;
    }
    else {
        Serial.println("Receive failed");
    }
    return false;
}

static uint16_t crc_xmodem_update(uint16_t crc, uint8_t data) {
    int i;
    crc = crc ^ ((uint16_t)data << 8);
    for (i=0; i<8; i++) {
        if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
        else crc <<= 1;
    }
    return crc;
}

/** Calculates 16-bit CRC checksum from a zero-terminated 8-bit string */
uint16_t gps_CRC16_checksum(const uint8_t *msg) {
    uint16_t crc = 0xFFFF;
    while (*msg) {
        crc = crc_xmodem_update(crc, *msg);
        msg++;
    }
    return crc;
}
