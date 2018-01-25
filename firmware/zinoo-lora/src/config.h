#pragma once

#define FREQUENCY_MHZ 434.25        // Transmit center frequency, MHz
#define TX_POWER_DBM    5           // Transmit power in dBm (range +5 .. +23)

/// Select LoRa mode (speed and bandwidth):
// * Bw125Cr45Sf128	    ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
// * Bw500Cr45Sf128	    ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
// * Bw31_25Cr48Sf512   ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
// * Bw125Cr48Sf4096    ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range
#define MODEM_MODE RH_RF95::Bw31_25Cr48Sf512


#define LORA_RST_PIN      	9       // Hardwired on the LoRa/GPS shield
#define LORA_CS_PIN			3		// Modified on the LoRa/GPS shield!!!
#define PPS_PIN             8       // Modified on the LoRa/GPS shield!!!

#define UPLINK_TIMEOUT		3000	// Milliseconds to wait for an uplink command after transmit

#ifndef CALLSIGN
#define CALLSIGN "Z_TEST"           // Payload callsign
#endif

#ifndef TIMESLOT
#define TIMESLOT 0                  // Transmit offset in seconds
#endif

#ifndef TOTAL_SLOTS
#define TOTAL_SLOTS 10              // Transmit period in seconds 
#endif


/// NTC thermistor configuration
#define NTC_PIN     0               // Analog pin number
#define NTC_T0      (273+25)        // 25C in Kelvin
#define NTC_R0      47000           // Resistance of the NTC at 25C, in ohms
#define NTC_B       2700            // B coefficient of the NTC
#define NTC_R1      10000           // Resistance of the resistor to the ground, in ohms

#define RELEASE_PIN         3       // Digital pin number (3, 4, 5, 14-16, 18-19)
#define RELEASE_ALTITUDE    10000   // Release pin will go high above this altitude (m)
#define RELEASE_SAFETIME    120     // Duration of safe mode in seconds since start (power on)


// Servo configuration
#define SERVO1_PIN	A4
#define SERVO2_PIN  A5

#define BUZZER_PIN  A2
