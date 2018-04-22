/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017-2018 Karlis Goba
*/

// All hardware and software configuration is in config.h
#include "config.h"

#include <RH_RF95.h>
#include <TinyGPS++.h>
//#include <EEPROM.h>

#include "tone.h"
#include "display.h"
#include "telemetry.h"

RemoteData  gLastPacket;

RH_RF95     lora(LORA_CS_PIN);
uint8_t     gLastPacketRaw[LORA_MAX_MESSAGE_LEN + 6];
TinyGPSPlus gps;


void setup() {
    Serial.begin(9600);
    //Serial.println("RESET");
		
#ifdef WITH_DISPLAY
	tft_setup();    // Initialize TFT and display static content
#endif

	lora_setup();   // Initialize LoRa module
	tone_setup();   // Setup GPS PPS synchronized timer (uses 16-bit Timer1)

#ifdef WITH_BUTTONS
	pinMode(BUTTON1_PIN, INPUT_PULLUP);
	pinMode(BUTTON2_PIN, INPUT_PULLUP);    
	pinMode(BUTTON3_PIN, INPUT_PULLUP);    
	pinMode(BUTTON4_PIN, INPUT_PULLUP);    
	pinMode(BUTTON5_PIN, INPUT_PULLUP);    
	pinMode(BUTTON6_PIN, INPUT_PULLUP);    
#endif

#ifdef WITH_LCD
    pinMode(SWITCH1_PIN, INPUT_PULLUP);
    pinMode(SWITCH2_PIN, OUTPUT);
#endif
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
    lora.setSpreadingFactor(10);
    lora.setCodingRate4(8);
    lora.setFrequency(FREQUENCY_MHZ);	
	
	char str[40];
    dtostrf(FREQUENCY_MHZ, 0, 3, str);
	Serial.print("LoRa frequency: ");
	Serial.println(str);
}

void loop() {
    static uint32_t next_GPS_update = 0;
    static uint32_t next_uplink = 0;
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
            gLastPacket.parse_string((const char *)gLastPacketRaw);
#ifdef WITH_DISPLAY
            tft_print_multiline((const char *)gLastPacketRaw, 2);
#endif
            gFields.msg_recv++;
            gFields.rssi = lora.lastRssi();
            gFields.update_remote_position(gLastPacket.lat, gLastPacket.lng, gLastPacket.alt, gLastPacket.time);
        }
    }
	
#ifdef WITH_DISPLAY
    if (newGPSData) {
        if (gps.location.isUpdated() || gps.altitude.isUpdated()) {
            gFields.update_local_position(gps.location.lat(), gps.location.lng(), gps.altitude.meters());
        }
        if (gps.satellites.isUpdated()) {
            gFields.loc_fix = gps.satellites.value();
        }
        if (gps.time.isValid()) {
            TimeHMS local_time;
            local_time.hour = gps.time.hour();
            local_time.minute = gps.time.minute();
            local_time.second = gps.time.second();
            gFields.update_local_time(local_time);
        }
    }
#endif
    
	if (millis() > next_report) {
		next_report += 1000;

#ifdef WITH_DISPLAY
        tft_update();
#endif
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
        sprintf(str, "**%02d%02d%02d,%s,%s,%s,%u", 
            gps.time.hour(), gps.time.minute(), gps.time.second(),
            lat_str, lng_str, alt_str, (uint8_t)gps.satellites.value());
		Serial.println(str);
    }
	
#ifdef WITH_BUTTONS
	// Check buttons for uplink commands
	uint8_t cmd_id = 0; 
	if (digitalRead(BUTTON1_PIN) == LOW) cmd_id = 1;
	if (digitalRead(BUTTON2_PIN) == LOW) cmd_id = 2;
	if (digitalRead(BUTTON3_PIN) == LOW) cmd_id = 3;
	if (digitalRead(BUTTON4_PIN) == LOW) cmd_id = 4;
	if (digitalRead(BUTTON5_PIN) == LOW) cmd_id = 5;
	if (digitalRead(BUTTON6_PIN) == LOW) cmd_id = 6;
	if (cmd_id && (millis() > next_uplink)) {
        next_uplink = millis() + (UPLINK_INTERVAL * 1000UL);
	    char tx_buf[2];
		tx_buf[0] = 'S';
		tx_buf[1] = ('0' + cmd_id);
	    lora.send((const uint8_t *)tx_buf, 2);
        #ifdef WITH_DISPLAY
            tft_print_multiline("UPLINK", 2);
        #endif

	}
#endif
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
