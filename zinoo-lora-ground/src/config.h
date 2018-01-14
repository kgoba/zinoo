#pragma once

#define FREQUENCY_MHZ 434.25        // Transmit center frequency, MHz
#define TX_POWER_DBM    5           // Transmit power in dBm (range +5 .. +23)

/// Select LoRa mode (speed and bandwidth):
// * Bw125Cr45Sf128	    ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
// * Bw500Cr45Sf128	    ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
// * Bw31_25Cr48Sf512   ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
// * Bw125Cr48Sf4096    ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range
#define MODEM_MODE RH_RF95::Bw31_25Cr48Sf512

#define LORA_MAX_MESSAGE_LEN    80

#define LORA_RST_PIN      	9       // Hardwired on the LoRa/GPS shield
#define LORA_CS_PIN			3		// Modified on the LoRa/GPS shield!!!
#define PPS_PIN             8       // Modified on the LoRa/GPS shield!!!


#define GPS_UPDATE_INTERVAL 60      // Time syncing interval (seconds)

#define BUTTON1_PIN			A4
#define BUTTON2_PIN			A5

#define TFT_CS    7
#define TFT_DC    6
#define TFT_RST   5

#ifndef USE_TFT
#define USE_TFT		false
#endif

#define TONE_FREQUENCY		1000
#define	TONE_PERIOD_DEFAULT	16000
#define	TONE_PERIOD_MIN		15000
#define	TONE_PERIOD_MAX		17000
#define TONE_PLL_THRESHOLD	4000