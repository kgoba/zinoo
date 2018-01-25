/*
  (c) Dragino Project, https://github.com/dragino/Lora
  (c) 2017 Karlis Goba
*/

//#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include <RH_RF95.h>
//#include <Servo.h>
#include <avr/wdt.h>

// All hardware and software configuration is in config.h
#include "config.h"

#include "Status.h"

TinyGPSPlus gps;
RH_RF95     lora(LORA_CS_PIN);
Status      status;
/*
Servo		servo1;
Servo		servo2;
*/

void setup() {
    wdt_enable(WDTO_2S);

    Serial.begin(9600);	
    Serial.println("RESET");

	gps_setup();
	lora_setup();
	pyro_setup();	
	//servo_setup();	
	buzzer_setup();
		
    status.restore();
}

void gps_setup()
{
    setSyncProvider(noSync);
    setSyncInterval(60);    // This resets timeStatus periodically to "sync"
}

bool gps_feed()
{
    bool newData = false;
    while (Serial.available()) {
        char c = Serial.read();
        //Serial.write(c);
        if (gps.encode(c)) newData = true; 
    }
    return newData;
}

void pyro_setup()
{
#ifdef WITH_PYRO
    digitalWrite(RELEASE_PIN, LOW);	
    pinMode(RELEASE_PIN, OUTPUT);
#endif    
}

void pyro_update()
{
#ifdef WITH_PYRO
    if (status.alt >= RELEASE_ALTITUDE && (millis() >= RELEASE_SAFETIME * 1000UL)) {
        // Go HIGH and never go back
        digitalWrite(RELEASE_PIN, HIGH);
    }
#endif    
}

void lora_setup()
{
    for (int n_try = 0; n_try < 5; n_try++) {
        // reset LoRa module to make sure it will works properly
        pinMode(LORA_RST_PIN, OUTPUT);
        digitalWrite(LORA_RST_PIN, LOW);   
        delay(1000);
        digitalWrite(LORA_RST_PIN, HIGH);

        wdt_reset();

        if (lora.init()) break;
        Serial.println("LoRa init failed, retrying...");
    }

    // Defaults after init are 434.0MHz, 13dBm
    // Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    lora.setModemConfig(MODEM_MODE);
    lora.setFrequency(FREQUENCY_MHZ);
    lora.setTxPower(TX_POWER_DBM);	
}

/*
void servo_setup()
{
	servo1.attach(SERVO1_PIN);
	servo2.attach(SERVO2_PIN);
	
	servo1.write(0);
	servo2.write(0);
}
*/

void buzzer_setup()
{
	TCCR2A = (1 << WGM21) | (1 << WGM20);								// Mode 7 (Fast PWM)
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20) | (1 << WGM22);	// Prescaler 1024
	TIMSK2 = (1 << TOIE2);							// Enable overflow interrupt (every 256)
	OCR2A  = 125;
	pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
    // Feed all available data to GPS parser
    bool newGPSData = gps_feed();   // Returns whether a new valid NMEA sentence came in
    
    // Sync time if necessary
    if (newGPSData && (timeStatus() != timeSet)) {
        update_time_from_gps();
    }
    
    // Update last valid coordinates if available
    if (newGPSData) {
        wdt_reset();

        update_location_from_gps();
        pyro_update();
    }
    
    // Transmit if it is our timeslot and time is valid
    //if ((timeStatus() != timeNotSet) && timeslot_go()) {
	if (timeslot_go()) {
        status.temperature_ext = read_temperature();

        transmit();
        status.msg_id++;
        status.save();
    }

	// Wait for an uplink command
    /*
    if (lora.waitAvailableTimeout(UPLINK_TIMEOUT)) {
        uint8_t reply[RH_RF95_MAX_MESSAGE_LEN];	// Uplink data
	    uint8_t len = sizeof(reply);
		if (lora.recv(reply, &len)) {
			reply[len] = '\0';	// Add zero string termination
		    // Log the command on serial
		    Serial.print(">>> "); Serial.println((char *)reply);
			
			if ((len > 1) && reply[0] == 'S') {
				uint8_t status = (reply[1] - '0');
				
				servo1.write((status & 1) ? 180 : 0);
				servo2.write((status & 2) ? 180 : 0);
			}
		}
    }
    */
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
    char    tx_buf[80];    // Temporary buffer for LoRa message

    if (status.build_string(tx_buf, 80)) {
        // Log the message on serial
        Serial.print(">>> "); Serial.println(tx_buf);
        // Send the data to server
        lora.send((const uint8_t *)tx_buf, strlen(tx_buf));
    }
}

/// Updates internal clock (using Time library) from GPS time data 
// (if it is valid)
void update_time_from_gps() {
    // Check for data validity (added satellite check, 
    // otherwise GPS sends fake time/date information)
    if (!gps.satellites.isValid() || gps.satellites.value() == 0) {
        // Do nothing, just return. Time data is not valid.
        return;
    }

    TinyGPSDate &date = gps.date;
    if (date.isValid()) {
        setTime(hour(), minute(), second(), date.day(), date.month(), date.year());
        
        // Report date update
        Serial.print("Setting date to ");
        Serial.print(date.year()); Serial.print('.');
        Serial.print(date.month()); Serial.print('.');
        Serial.println(date.day());
    }

    TinyGPSTime &time = gps.time;
    if (time.isValid() && time.age() < 500) {
        setTime(time.hour(), time.minute(), time.second(), day(), month(), year());
        
        // Report time update
        Serial.print("Setting time to ");
        Serial.print(time.hour()); Serial.print(':');
        Serial.print(time.minute()); Serial.print(':');
        Serial.println(time.second());
    }
}

/// Updates status variable with the latest GPS location data
// (if it is valid)
void update_location_from_gps() {
    //if (gps.satellites.isValid()) {
        status.n_sats = gps.satellites.value();
    //}
    //else status.n_sats = 0;

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
    float k1 = log(R / NTC_R0) / NTC_B;  // Helper value
    float T = 1 / (k1 + 1.0 / NTC_T0);     // Temperature in Kelvins

    if (T < 150) T = 150;               // Impose a limit of approx -123 C
    return (T - 273.15);                // Convert to Celsius
}

/// Reads ADC value and converts to Celsius degrees
float read_temperature() {
    return celsius_from_adc(analogRead(NTC_PIN_AN));
}

/// Callback function used by the TimeLib library
time_t noSync() {
    return 0;       // No sync, but we need this to set the status
}

// Buzzer timer ISR routine
ISR(TIMER2_OVF_vect)        // Called every 8 ms
{
	static uint16_t phase;

    // Short, fast beeps = FIX INVALID
    // Longer, rarer beeps = FIX VALID
    uint16_t period = status.fixValid ? (3 * 125) : (1 * 125);
    uint16_t pwm_on = status.fixValid ? (0.6 * 125) : (0.2 * 125);

	phase++;
	if (phase >= period) {
		phase = 0;
		digitalWrite(BUZZER_PIN, HIGH);
	}
    else {
        if (phase > pwm_on) {
            digitalWrite(BUZZER_PIN, LOW);
        }
    }
}
