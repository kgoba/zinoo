/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017 Karlis Goba

  In this project,we'll show how to get GPS data from a remote Arduino via Wireless Lora Protocol 
and show the track on the GoogleEarth.The construction of this project is similar to my last one:

1) Client Side: Arduino + Lora/GPS Shield
2) Server Side: Arduino + Lora Shield

Client side will get GPS data and keep sending out to the server via Lora wireless. Server side 
will listin on the Lora wireless frequency, once it get the data from Client side, it will
turn on the LED and log the sensor data to a USB flash. 
*/

//Include required lib so Arduino can talk with the Lora Shield
#include <SPI.h>
#include <RH_RF95.h>
#include <TinyGPS.h>

// Transmit frequency in MHz
#define FREQUENCY_MHZ 434.25

// Bw125Cr45Sf128	   ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
// Bw500Cr45Sf128	   ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
// Bw31_25Cr48Sf512	   ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
// Bw125Cr48Sf4096     ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range
#define MODEM_MODE RH_RF95::Bw31_25Cr48Sf512

#define GPS_UPDATE_INTERVAL 30

TinyGPS gps;
RH_RF95 lora;

const int reset_lora = 9;

String dataString = "";

void setup() {
    Serial.begin(9600);
    Serial.println("RESET");

    pinMode(reset_lora, OUTPUT);     

    // reset LoRa module first to make sure it will works properly
    digitalWrite(reset_lora, LOW);   
    delay(1000);
    digitalWrite(reset_lora, HIGH); 
    delay(100);
  
    if (!lora.init()) {
        Serial.println("LoRa init failed");
    }
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    lora.setModemConfig(MODEM_MODE);
    lora.setFrequency(FREQUENCY_MHZ);
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
    
    if (lora.available()) {
        // Should be a message for us now   
        receive(); 
    }
    
    if (newData && (millis() > next_GPS_update)) {        
        float flat, flng;
        unsigned long age;
  
        gps.f_get_position(&flat, &flng, &age);
        if (age != TinyGPS::GPS_INVALID_AGE) {
            // Convert altitude to 16 bit unsigned    
            float falt = gps.f_altitude();
            if (falt == TinyGPS::GPS_INVALID_ALTITUDE || falt > 9999) falt = 0; // GPS problem fix
            
            byte hour, minute, second;
            gps.crack_datetime(0, 0, 0, &hour, &minute, &second, NULL, 0);
            
            char str[40], lat_str[11], lng_str[11], alt_str[8];
            dtostrf(flat, 0, 5, lat_str);
            dtostrf(flng, 0, 5, lng_str);
            dtostrf(falt, 0, 0, alt_str);
            sprintf(str, "**%02d%02d%02d,%s,%s,%s,%d", 
                hour, minute, second,
                lat_str, lng_str, alt_str, gps.satellites());
            Serial.println(str);

            next_GPS_update += GPS_UPDATE_INTERVAL * 1000;
        }
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
        //make a string that start with a timestamp for assembling the data to log:
        dataString="";
        dataString += String((char*)buf);
        dataString += ",";
        dataString += getTimeStamp();

        // Send a reply to client as ACK
        uint8_t data[] = "200 OK";
        lora.send(data, sizeof(data));
        lora.waitPacketSent();
        Serial.println("Sent a reply");

        // Log the received message
        Serial.println(dataString);
        */
    }
    else {
        Serial.println("Receive failed");
    }
}

// This function returns a string with the time stamp (in full seconds)
String getTimeStamp() {
    uint32_t millis_now = millis();
    //uint16_t seconds = millis_now / 1000;
    //uint16_t  minutes = seconds / 60;
    String result(millis_now / 1000);
    return result;
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