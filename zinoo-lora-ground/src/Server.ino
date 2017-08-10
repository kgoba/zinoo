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

namespace config {
    // Receive frequency. Set to 0 to use the default LoRa settings (434.0 MHz?)
    const float frequency_mhz = 434.25;
};

RH_RF95 lora;

const int led = 4;
const int reset_lora = 9;

String dataString = "";

void setup() {
    Serial.begin(9600);
    Serial.println("RESET");

    pinMode(led, OUTPUT); 
    pinMode(reset_lora, OUTPUT);     

    // reset lora module first. to make sure it will works properly
    digitalWrite(reset_lora, LOW);   
    delay(1000);
    digitalWrite(reset_lora, HIGH); 
  
    if (!lora.init()) {
        Serial.println("LoRa init failed");
    }
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    if (config::frequency_mhz != 0)
        lora.setFrequency(config::frequency_mhz);
}

void loop() {
    if (!lora.available()) return;
  
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = sizeof(buf);
    if (lora.recv(buf, &len)) {
        // Add zero termination, so it's a valid C string
        buf[len] = '\0'; 
      
        // Append UKHAS checksum and a newline
        char chksum_str[7];
        sprintf(chksum_str, "*%04X\n", gps_CRC16_checksum(buf));
        strcat((char *)buf, chksum_str);
      
        digitalWrite(led, HIGH);
        //RH_RF95::printBuffer("request: ", buf, len);
        Serial.print("$$"); Serial.println((char*)buf);
        Serial.print("RSSI: "); Serial.println(lora.lastRssi(), DEC);

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

        digitalWrite(led, LOW);      
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