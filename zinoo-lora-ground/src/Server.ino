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

// Singleton instance of the radio driver
RH_RF95 rf95;

const int led = 4;
const int reset_lora = 9;

String dataString = "";

void setup() 
{
  Serial.begin(9600);
  Serial.println("RESET");

  pinMode(led, OUTPUT); 
  pinMode(reset_lora, OUTPUT);     

  // reset lora module first. to make sure it will works properly
  digitalWrite(reset_lora, LOW);   
  delay(1000);
  digitalWrite(reset_lora, HIGH); 
  
  if (!rf95.init()) {
      Serial.println("LoRa init failed");
  }
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // Change by calling rf96.setFrequency(mhz)
}

void loop()
{
  dataString="";
  if (rf95.available())
  {
    Serial.println("Get new message");
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      digitalWrite(led, HIGH);
      //RH_RF95::printBuffer("request: ", buf, len);
      Serial.print("got message: "); Serial.println((char*)buf);
      Serial.print("RSSI: "); Serial.println(rf95.lastRssi(), DEC);

      //make a string that start with a timestamp for assembling the data to log:
      dataString += String((char*)buf);
      dataString += ",";
      dataString += getTimeStamp();

      // Send a reply to client as ACK
      uint8_t data[] = "200 OK";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");

      // Log the received message
      Serial.println(dataString);
      
      digitalWrite(led, LOW);      
    }
    else
    {
      Serial.println("recv failed");
    }
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
