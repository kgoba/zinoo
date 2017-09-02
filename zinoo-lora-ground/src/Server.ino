/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017 Karlis Goba
*/

#include <RH_RF95.h>
#include <TinyGPS++.h>

// All hardware and software configuration is in config.h
#include "config.h"

TinyGPSPlus gps;
RH_RF95     lora;

void setup() {
    Serial.begin(9600);
    Serial.println("RESET");

    pinMode(LORA_RESET_PIN, OUTPUT);     

    // reset LoRa module first to make sure it will works properly
    digitalWrite(LORA_RESET_PIN, LOW);   
    delay(1000);
    digitalWrite(LORA_RESET_PIN, HIGH); 
    delay(100);
  
    if (!lora.init()) {
        Serial.println("LoRa init failed");
    }
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    lora.setModemConfig(MODEM_MODE);
    lora.setFrequency(FREQUENCY_MHZ);
    
    // Setup GPS PPS synchronized timer (uses 16-bit Timer1)
    setupHiresTimer();
    attachInterrupt(digitalPinToInterrupt(PPS_PIN), resetHiresTimer, RISING);
}

void loop() {
    static uint32_t next_GPS_update = 0;
    bool newData = false;   // Did a new valid sentence come in?

    // Feed data to GPS parser
    while (Serial.available()) {
        char c = Serial.read();
        if (gps.encode(c)) {
            newData = true; 
        }
    }
    
    // Check for pending received messages on the radio
    if (lora.available()) {
        receive(); 
    }
    
    // Report our GPS location if necessary
    if (newData && (millis() > next_GPS_update) && gps.location.isValid()) {
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
        Serial.println(str);
    }
}

void receive() {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = sizeof(buf);
    if (lora.recv(buf, &len)) {
        // Add zero termination, so it's a valid C string
        buf[len] = '\0'; 
  
        // Append UKHAS checksum
        char chksum_str[8];
        sprintf(chksum_str, "*%04X", gps_CRC16_checksum(buf));
        strcat((char *)buf, chksum_str);
  
        Serial.print("$$"); Serial.println((char*)buf);
        Serial.print("  RSSI="); Serial.print(lora.lastRssi(), DEC);
        
        Serial.println();
        
        /*
        // Send a reply to client as ACK
        uint8_t data[] = "200 OK";
        lora.send(data, sizeof(data));
        lora.waitPacketSent();
        */
    }
    else {
        Serial.println("Receive failed");
    }
}

uint16_t gps_CRC16_checksum(const uint8_t *msg) {
    uint16_t crc = 0xFFFF;
    while (*msg) {
        crc = crc_xmodem_update(crc, *msg);
        msg++;
    }
    return crc;
}

uint16_t crc_xmodem_update(uint16_t crc, uint8_t data) {
    int i;
    crc = crc ^ ((uint16_t)data << 8);
    for (i=0; i<8; i++) {
        if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
        else crc <<= 1;
    }
    return crc;
}

void setupHiresTimer()
{
    // Configure Timer1: prescaler 1, normal mode, resolution 1/16 uS, TOP=0xFFFF
    TCCR1A = 0;
    TCCR1B = (1 << CS10);   
    TCCR1C = 0;
}

uint16_t ppsPeriod;

void resetHiresTimer()
{
    ppsPeriod = TCNT1;
    TCNT1 = 0;
}

uint16_t readHiresTimer()
{
    return TCNT1;
}
