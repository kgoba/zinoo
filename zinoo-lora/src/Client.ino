/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017 Karlis Goba
*/

#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <EEPROM.h>

#ifndef CALLSIGN
#define CALLSIGN "Z00"              // Payload callsign
#endif

#ifndef TIMESLOT
#define TIMESLOT 0                  // Transmit offset in seconds
#endif

#ifndef TOTAL_SLOTS
#define TOTAL_SLOTS 10              // Transmit period in seconds 
#endif

#define FREQUENCY_MHZ 434.25        // Transmit center frequency, MHz
#define TX_POWER_DBM 5              // Transmit power in dBm (range +5 .. +23)

// Select LoRa mode (speed and bandwidth):
// * Bw125Cr45Sf128	    ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
// * Bw500Cr45Sf128	    ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
// * Bw31_25Cr48Sf512   ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
// * Bw125Cr48Sf4096    ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range
#define MODEM_MODE RH_RF95::Bw31_25Cr48Sf512

struct Status {
    uint16_t msg_id;

    bool     fixValid;
    float    lat, lng;  // Last valid coordinates
    uint16_t alt;       // Last valid altitude
    uint8_t  n_sats;    // Current satellites
    
    void restore() {
        //EEPROM.get(0x00, status);
        //if (status.msg_id == 0xFFFF) {
            // Fresh EEPROM - reset the contents of status in RAM
            msg_id = 1;
            fixValid = false;
            lat = lng = 0;
            alt = n_sats = 0;
            save();
        //}        
    }
    
    void save() {
        //EEPROM.put(0x00, status);
    }
};

TinyGPS gps;
RH_RF95 lora;

Status status;

const int pin_reset_lora = 9;

void setup() {
    setSyncInterval(60);
    
    Serial.begin(9600);
    Serial.println("RESET");
    
    for (int n_try = 0; n_try < 5; n_try++) {
        if (lora.init()) break;
        Serial.println("LoRa init failed, retrying...");
        
        // reset LoRa module to make sure it will works properly
        pinMode(pin_reset_lora, OUTPUT);
        digitalWrite(pin_reset_lora, LOW);   
        delay(1000);
        digitalWrite(pin_reset_lora, HIGH); 
    }

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    lora.setModemConfig(MODEM_MODE);
    lora.setFrequency(FREQUENCY_MHZ);
    lora.setTxPower(TX_POWER_DBM);
    
    status.restore();
}

void loop() {
    bool newData = false;   // Did a new valid sentence come in?

    // Feed data to GPS parser
    if (Serial.available()) {
        char c = Serial.read();
        if (gps.encode(c)) {
            newData = true; 
        }
    }
    
    // Sync time if necessary
    if (newData && (timeStatus() != timeSet)) {
        int year;
        byte month, day, hour, minute, second;
        unsigned long age;

        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &age);
        if (age < 500) {
            // set the Time to the latest GPS reading
            setTime(hour, minute, second, day, month, year);
            Serial.print("Setting time to ");
            Serial.print(year); Serial.print('.');
            Serial.print(month); Serial.print('.');
            Serial.print(day); Serial.print(' ');
            Serial.print(hour); Serial.print(':');
            Serial.print(minute); Serial.print(':');
            Serial.println(second);
        }
    }
    
    // Update last valid coordinates if available
    if (newData) {
        float flat, flng;
        unsigned long age;
  
        gps.f_get_position(&flat, &flng, &age);
        if (age != TinyGPS::GPS_INVALID_AGE) {
            // Convert altitude to 16 bit unsigned    
            float falt = gps.f_altitude();
            if ((falt > 0) && (falt != TinyGPS::GPS_INVALID_ALTITUDE)) {
                if (falt <= 65535) status.alt = (uint16_t)falt;
                else status.alt = 65535;
            }
            else status.alt = 0;
            
            status.fixValid = true;
            status.lat = flat;
            status.lng = flng;
            status.n_sats = gps.satellites();
            status.save();
        }
    }       
    
    // Transmit if it is our timeslot 
    if ((timeStatus() != timeNotSet) && timeslot_go()) {
        transmit();
        status.msg_id++;
        status.save();
    }
}

bool timeslot_go() {
    //static uint8_t last_minute = 0xFF;
    static uint8_t last_second = 0xFF;
    
    // Is it still the same minute as previously?
    //if (minute() == last_minute) return false;

    // Is it still the same second as previously?
    if (second() == last_second) return false;
    
    last_second = second();

    return ((second() % TOTAL_SLOTS) == TIMESLOT);
}

void transmit() {
    char tx_buf[80];    // Temporary buffer for LoRa message (CHECK SIZE)
    char lat_str[11];
    char lng_str[11];

    if (status.fixValid) {
        dtostrf(status.lat, 0, 5, lat_str);
    } else {
        lat_str[0] = '\0';  // Empty latitude field in case fix is invalid
    }
    if (status.fixValid) {
        dtostrf(status.lng, 0, 5, lng_str);        
    } else {
        lng_str[0] = '\0';  // Empty longitude field in case fix is invalid
    }
    
    // Build partial UKHAS sentence (without $$ and checksum)
    // e.g. Z70,90,160900,51.03923,3.73228,31,9
    sprintf(tx_buf, "%s,%d,%02d%02d%02d,%s,%s,%u,%d",
        CALLSIGN, status.msg_id,
        hour(), minute(), second(),
        lat_str, lng_str, status.alt, status.n_sats
    );
    
    // Log the message on serial
    Serial.print(">>> "); Serial.println(tx_buf);

    // Send the data to server
    lora.send((const uint8_t *)tx_buf, strlen(tx_buf));
}
