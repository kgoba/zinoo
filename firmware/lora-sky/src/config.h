#pragma once

/*
    To protect the binary field:

            ###
              ###
        ###     ###
          ### ###     
    ###     ###     ###
      ### ### ### ### ###
###     ###     ###     ###
  ### ### ### ### ###
    ###     ###     ###
          ### ###
        ###     ###
          ###
            ###
*/

#ifndef CALLSIGN
#define CALLSIGN "TEST"           // Payload callsign
#endif

#ifndef TIMESLOT
#define TIMESLOT 0                  // Transmit offset in seconds
#endif

#ifndef TOTAL_SLOTS
#define TOTAL_SLOTS 10              // Transmit period in seconds 
#endif


#define FREQUENCY_MHZ 434.25        // Transmit center frequency, MHz
#define TX_POWER_DBM   13           // Transmit power in dBm (range +5 .. +23)

/*
RFOP = +20 dBm, on PA_BOOST         120mA
RFOP = +17 dBm, on PA_BOOST          87mA
RFOP = +13 dBm, on RFO_LF/HF pin     29mA
RFOP = + 7 dBm, on RFO_LF/HF pin     20mA
*/

/// Select LoRa mode (speed and bandwidth):
// * Bw125Cr45Sf128        ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
// * Bw500Cr45Sf128        ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
// * Bw31_25Cr48Sf512   ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
// * Bw125Cr48Sf4096    ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range
#define MODEM_MODE RH_RF95::Bw31_25Cr48Sf512

#define LORA_RST_PIN          9       // Hardwired on the LoRa/GPS shield

#ifdef LORA_MODIFIED
#define LORA_CS_PIN           3       // Modified on the LoRa/GPS shield!!!
#define PPS_PIN               8       // Modified on the LoRa/GPS shield!!!
#else
#define LORA_CS_PIN          10       // Original configuration on the LoRa/GPS shield
#define PPS_PIN              A3       // Original configuration on the LoRa/GPS shield
#endif


// NTC thermistor configuration
#define NTC_PIN_AN  1               // Analog pin number
#define NTC_T0      (273.15+25)     // 25C in Kelvin
#define NTC_R0      22000           // Resistance of the NTC at 25C, in ohms
#define NTC_B       2700            // B coefficient of the NTC
#define NTC_R1      100000          // Value of the ground resistor, in ohms

#ifdef WITH_PYRO
// CH1 - On/Off Switch Aux cameras/router (uplink + auto turn off)
// CH2 - On/Off Switch Control Camera TX (uplink + auto turn off)
// CH3 - Ignition (uplink)
// CH4 - Balloon Burst (uplink + automatic altitude threshold w/ safe timer)

#define RELEASE_ALTITUDE    20000   // Release pin will go high above this altitude (m)
#define RELEASE_SAFETIME    120     // Duration of safe mode in seconds since start (power on/reset)

#define SWITCH1_PIN         4
#define SWITCH2_PIN         5
#define SWITCH3_PIN         6
#define SWITCH4_PIN         7

// Auto-off timeouts for switches in seconds (put 0 or negative to disable the feature)
#define SWITCH1_AUTO_OFF    -1
#define SWITCH2_AUTO_OFF    10*60
#define SWITCH3_AUTO_OFF    5
#define SWITCH4_AUTO_OFF    5
#endif

// Servo configuration
#define SERVO1_PIN    A4
#define SERVO2_PIN    A5

#define BUZZER_PIN    A0
