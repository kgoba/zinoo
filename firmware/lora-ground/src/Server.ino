/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017-2018 Karlis Goba
*/

// All hardware and software configuration is in config.h
#include "config.h"

#include <RH_RF95.h>
#include <TinyGPS++.h>

#include "tone.h"
#include "display.h"

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
	Serial.print("LoRa frequency: ");
	Serial.println(str);
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
            gLastPacket.parse_string((const char *)gLastPacketRaw);

            tft_print_multiline((const char *)gLastPacketRaw, 2);

            gFields.msg_recv++;
            gFields.rssi = lora.lastRssi();
            gFields.update_remote_position(gLastPacket.lat, gLastPacket.lng, gLastPacket.alt, gLastPacket.time);
#endif
        }
    }
	
#if USE_TFT
    if (newGPSData) {
        if (gps.location.isUpdated() || gps.altitude.isUpdated()) {
            gFields.update_local_position(gps.location.lat(), gps.location.lng(), gps.altitude.meters());
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