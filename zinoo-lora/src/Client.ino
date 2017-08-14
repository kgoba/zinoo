/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017 Karlis Goba
*/

#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <EEPROM.h>


#ifndef CALLSIGN
#define CALLSIGN "Z_TEST"           // Payload callsign
#endif

#ifndef TIMESLOT
#define TIMESLOT 0                  // Transmit offset in seconds
#endif

#ifndef TOTAL_SLOTS
#define TOTAL_SLOTS 10              // Transmit period in seconds 
#endif

#define FREQUENCY_MHZ 434.25        // Transmit center frequency, MHz
#define TX_POWER_DBM    5           // Transmit power in dBm (range +5 .. +23)

/// Select LoRa mode (speed and bandwidth):
// * Bw125Cr45Sf128	    ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
// * Bw500Cr45Sf128	    ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
// * Bw31_25Cr48Sf512   ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
// * Bw125Cr48Sf4096    ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range
#define MODEM_MODE RH_RF95::Bw31_25Cr48Sf512

#define LORA_RESET_PIN      9       // Hardwired on the LoRa/GPS shield
#define PPS_PIN             17      // Hardwired on the LoRa/GPS shield

/// NTC thermistor configuration
#define NTC_PIN     0               // Analog pin number
#define NTC_T0      (273+25)        // 25C in Kelvin
#define NTC_R0      47000           // Resistance of the NTC at 25C, in ohms
#define NTC_B       2700            // B coefficient of the NTC
#define NTC_R1      10000           // Resistance of the resistor to the ground, in ohms

#define RELEASE_PIN         3       // Digital pin number (3, 4, 5, 14-16, 18-19)
#define RELEASE_ALTITUDE    10000   // Release pin will go high above this altitude (m)
#define RELEASE_SAFETIME    120     // Duration of safe mode in seconds since start (power on)

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

TinyGPSPlus gps;
RH_RF95 lora;
Status status;

void setup() {
    setSyncInterval(60);    // This resets timeStatus periodically to "sync"
    setSyncProvider(noSync);
    
    Serial.begin(9600);
    Serial.println("RESET");
    
    pinMode(RELEASE_PIN, OUTPUT);
    digitalWrite(RELEASE_PIN, LOW);
    
    for (int n_try = 0; n_try < 5; n_try++) {
        if (lora.init()) break;
        Serial.println("LoRa init failed, retrying...");
        
        // reset LoRa module to make sure it will works properly
        pinMode(LORA_RESET_PIN, OUTPUT);
        digitalWrite(LORA_RESET_PIN, LOW);   
        delay(1000);
        digitalWrite(LORA_RESET_PIN, HIGH); 
    }

    // Defaults after init are 434.0MHz, 13dBm
    // Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    lora.setModemConfig(MODEM_MODE);
    lora.setFrequency(FREQUENCY_MHZ);
    lora.setTxPower(TX_POWER_DBM);
    
    status.restore();
}

void loop() {
    bool newData = false;   // Did a new valid sentence come in?

    // Feed all available data to GPS parser
    while (Serial.available()) {
        char c = Serial.read();
        if (gps.encode(c)) {
            newData = true; 
        }
    }
    
    // Sync time if necessary
    if (newData && (timeStatus() != timeSet)) {
        update_time_from_gps();
    }
    
    // Update last valid coordinates if available
    if (newData) {
        update_location_from_gps();
        
        if (status.alt >= RELEASE_ALTITUDE && (millis() >= RELEASE_SAFETIME * 1000UL)) {
            // Go HIGH and never go back
            digitalWrite(RELEASE_PIN, HIGH);
        }
    }
    
    // Transmit if it is our timeslot and time is valid
    if ((timeStatus() != timeNotSet) && timeslot_go()) {
        transmit();
        status.msg_id++;
        status.save();
    }
}

/// Checks whether it is right time for transmission
bool timeslot_go() {
    static uint8_t last_second = 0xFF;

    // Is it still the same second as previously?
    if (second() == last_second) return false;
    
    last_second = second();

    return ((second() % TOTAL_SLOTS) == TIMESLOT);
}

/// Constructs payload message and transmits it via radio
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
    
    int8_t temperature_ext = read_temperature();
    
    // Build partial UKHAS sentence (without $$ and checksum)
    // e.g. Z70,90,160900,51.03923,3.73228,31,9,-10
    sprintf(tx_buf, "%s,%d,%02d%02d%02d,%s,%s,%u,%d,%d",
        CALLSIGN, status.msg_id,
        hour(), minute(), second(),
        lat_str, lng_str, status.alt, 
        status.n_sats, 
        temperature_ext
    );
    
    // Log the message on serial
    Serial.print(">>> "); Serial.println(tx_buf);

    // Send the data to server
    lora.send((const uint8_t *)tx_buf, strlen(tx_buf));
}

/// Updates internal clock (using Time library) from GPS time data 
// (if it is valid)
void update_time_from_gps() {
    TinyGPSDate &date = gps.date;
    TinyGPSTime &time = gps.time;

    // Check for data validity (added satellite check, 
    // otherwise GPS sends fake time/date information)
    if (date.isValid() && time.isValid() 
            && gps.satellites.isValid() && gps.satellites.value() > 0   
            && time.age() < 500) 
    {
        setTime(time.hour(), time.minute(), time.second(), date.day(), date.month(), date.year());
        
        // Report time update
        Serial.print("Setting time to ");
        Serial.print(date.year()); Serial.print('.');
        Serial.print(date.month()); Serial.print('.');
        Serial.print(date.day()); Serial.print(' ');
        Serial.print(time.hour()); Serial.print(':');
        Serial.print(time.minute()); Serial.print(':');
        Serial.println(time.second());
    }
}

/// Updates status variable with the latest GPS location data
// (if it is valid)
void update_location_from_gps() {
    if (gps.satellites.isValid()) {
        status.n_sats = gps.satellites.value();
    }
    else status.n_sats = 0;

    if (gps.location.isValid()) {
        float falt = 0;
        if (gps.altitude.isValid()) falt = gps.altitude.meters();

        // Convert altitude to 16 bit unsigned    
        if (falt > 0) {
            if (falt > 65535) status.alt = 65535;
            else status.alt = (uint16_t)falt;
        }
        else status.alt = 0;
        
        status.fixValid = true;
        status.lat = gps.location.lat();
        status.lng = gps.location.lng();
    }
    else status.fixValid = false;
}

/// Converts 10 bit ADC reading from a NTC to Celsius degrees.
// The NTC is connected to +5V/AREF, and forms a divider 
// with a resistor to the ground.
float celsius_from_adc(uint16_t adc) {
    float ratio = adc / 1024.0;          // Convert to 0..1
    float R = (NTC_R1 / ratio) - NTC_R1; // Calculate NTC resistance
    float k1 = logf(R / NTC_R0) / NTC_B; // Helper value
    float T = 1 / (1 / NTC_T0 + k1);     // Temperature in Kelvins

    return (T - 273.15);                 // Convert to Celsius
}

/// Reads ADC value and converts to Celsius degrees
float read_temperature() {
    return celsius_from_adc(analogRead(NTC_PIN));
}

time_t noSync() {
    return 0;       // No sync, but we need this to set the status
}