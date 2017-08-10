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

This code uses SoftwareSerial and assumes that you have a 9600-baud serial GPS device 
hooked up on pins 3(rx) and 4(tx).
*/

#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <RH_RF95.h>

namespace config {
    // Our identifier
    const char * const callsign = "Z70";
    
    // Transmit frequency. Set to 0 to use the default LoRa settings (434.0 MHz?)
    const float frequency_mhz = 434.25;

    // Transmit every minute at the specified second 
    const int tx_timeslot = 30; 
};

SoftwareSerial ss(3, 4);
TinyGPS gps;
RH_RF95 lora;

struct Status {
    uint16_t msg_id;

    float    lat, lng;  // Last valid coordinates
    uint16_t alt;       // Last valid altitude
    uint8_t  n_sats;    // Current satellites
};

Status status;


void setup() {
    Serial.begin(9600);
    ss.begin(9600);
    //ss.print("Simple TinyGPS library v. "); ss.println(TinyGPS::library_version());

    if (!lora.init()) {
        Serial.println("LoRa init failed");      
    }

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    if (config::frequency_mhz != 0)
        lora.setFrequency(config::frequency_mhz);
}

void loop() {
    bool newData = false;

    while (Serial.available()) {
        char c = Serial.read();
        // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
        if (gps.encode(c)) {// Did a new valid sentence come in?
            newData = true;
        }
    }
    if (!newData) return;
    
    // Sync time if necessary
    if (newData && (timeStatus() == timeNotSet)) {
        int year;
        byte month, day, hour, minute, second;
        unsigned long age;

        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &age);
        if (age < 500) {
            // set the Time to the latest GPS reading
            setTime(hour, minute, second, day, month, year);
        }
    }
    
    // Update coordinates if available
    if (newData) {
        float flat, flng;
        unsigned long age;
  
        gps.f_get_position(&flat, &flng, &age);
        if (age != TinyGPS::GPS_INVALID_AGE) {
            // Convert altitude to 16 bit unsigned    
            float falt = gps.f_altitude();
            if (falt > 0) {
                if (falt <= 65535) status.alt = (uint16_t)falt;
                else status.alt = 65535;
            }
            else status.alt = 0;
            
            status.lat = flat;
            status.lng = flng;
            status.n_sats = gps.satellites();
        }
    }       
    
    // Transmit if our timeslot is up
    if ((timeStatus() != timeNotSet) && timeslot_go()) {
        transmit();
    }
}

bool timeslot_go() {
    static uint8_t last_minute = 0xFF;

    // Is it still the same minute as previously?
    if (minute() == last_minute) return false;

    // Have we reached the second of our timeslot?
    if (second() < config::tx_timeslot) 
        return false;           
    
    // It's our timeslot. Update the minute so it's used up
    last_minute = minute(); 
    return true;
}

void transmit() {
    char tx_buf[80];    // Temporary buffer for LoRa message (CHECK SIZE)

    // Build partial UKHAS sentence (without $$ and checksum)
    sprintf(tx_buf, "%s,%d,%02d%02d%02d,%.5f,%.5f,%d,%d",
        config::callsign, status.msg_id,
        hour(), minute(), second(),
        status.lat, status.lng, status.alt, status.n_sats
    );
        
    // Send the data to server
    lora.send((const uint8_t *)tx_buf, strlen(tx_buf));

    /*
    // Now wait for a reply
    static uint8_t indatabuf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(indatabuf);

    if (lora.waitAvailableTimeout(3000)) { 
        // Should be a reply message for us now   
        if (lora.recv(indatabuf, &len)) {
            // Serial print "got reply:" and the reply message from the server
            ss.print("got reply: ");
            ss.println((char*)indatabuf);
        }
        else {
            ss.println("recv failed");
        }
    }
    else {
        // Serial print "No reply, is lora_server running?" if don't get the reply .
        ss.println("No reply, is lora_server running?");
    }  
    */       
}

