#pragma once

// ---------------------------------------- 
// Registers Mapping
#define RegFifo                                    0x00 // common
#define RegOpMode                                  0x01 // common
#define FSKRegBitrateMsb                           0x02
#define FSKRegBitrateLsb                           0x03
#define FSKRegFdevMsb                              0x04
#define FSKRegFdevLsb                              0x05
#define RegFrfMsb                                  0x06 // common
#define RegFrfMid                                  0x07 // common
#define RegFrfLsb                                  0x08 // common
#define RegPaConfig                                0x09 // common
#define RegPaRamp                                  0x0A // common
#define RegOcp                                     0x0B // common
#define RegLna                                     0x0C // common
#define FSKRegRxConfig                             0x0D
#define LORARegFifoAddrPtr                         0x0D
#define FSKRegRssiConfig                           0x0E
#define LORARegFifoTxBaseAddr                      0x0E
#define FSKRegRssiCollision                        0x0F
#define LORARegFifoRxBaseAddr                      0x0F 
#define FSKRegRssiThresh                           0x10
#define LORARegFifoRxCurrentAddr                   0x10
#define FSKRegRssiValue                            0x11
#define LORARegIrqFlagsMask                        0x11 
#define FSKRegRxBw                                 0x12
#define LORARegIrqFlags                            0x12 
#define FSKRegAfcBw                                0x13
#define LORARegRxNbBytes                           0x13 
#define FSKRegOokPeak                              0x14
#define LORARegRxHeaderCntValueMsb                 0x14 
#define FSKRegOokFix                               0x15
#define LORARegRxHeaderCntValueLsb                 0x15 
#define FSKRegOokAvg                               0x16
#define LORARegRxPacketCntValueMsb                 0x16 
#define LORARegRxpacketCntValueLsb                 0x17 
#define LORARegModemStat                           0x18 
#define LORARegPktSnrValue                         0x19 
#define FSKRegAfcFei                               0x1A
#define LORARegPktRssiValue                        0x1A 
#define FSKRegAfcMsb                               0x1B
#define LORARegRssiValue                           0x1B 
#define FSKRegAfcLsb                               0x1C
#define LORARegHopChannel                          0x1C 
#define FSKRegFeiMsb                               0x1D
#define LORARegModemConfig1                        0x1D 
#define FSKRegFeiLsb                               0x1E
#define LORARegModemConfig2                        0x1E 
#define FSKRegPreambleDetect                       0x1F
#define LORARegSymbTimeoutLsb                      0x1F 
#define FSKRegRxTimeout1                           0x20
#define LORARegPreambleMsb                         0x20 
#define FSKRegRxTimeout2                           0x21
#define LORARegPreambleLsb                         0x21 
#define FSKRegRxTimeout3                           0x22
#define LORARegPayloadLength                       0x22 
#define FSKRegRxDelay                              0x23
#define LORARegPayloadMaxLength                    0x23 
#define FSKRegOsc                                  0x24
#define LORARegHopPeriod                           0x24 
#define FSKRegPreambleMsb                          0x25
#define LORARegFifoRxByteAddr                      0x25
#define LORARegModemConfig3                        0x26
#define FSKRegPreambleLsb                          0x26
#define FSKRegSyncConfig                           0x27
#define LORARegFeiMsb                              0x28
#define FSKRegSyncValue1                           0x28
#define LORAFeiMib                                 0x29
#define FSKRegSyncValue2                           0x29
#define LORARegFeiLsb                              0x2A
#define FSKRegSyncValue3                           0x2A
#define FSKRegSyncValue4                           0x2B
#define LORARegRssiWideband                        0x2C
#define FSKRegSyncValue5                           0x2C
#define FSKRegSyncValue6                           0x2D
#define FSKRegSyncValue7                           0x2E
#define FSKRegSyncValue8                           0x2F
#define FSKRegPacketConfig1                        0x30
#define FSKRegPacketConfig2                        0x31
#define LORARegDetectOptimize                      0x31
#define FSKRegPayloadLength                        0x32
#define FSKRegNodeAdrs                             0x33
#define LORARegInvertIQ                            0x33
#define FSKRegBroadcastAdrs                        0x34
#define FSKRegFifoThresh                           0x35
#define FSKRegSeqConfig1                           0x36
#define FSKRegSeqConfig2                           0x37
#define LORARegDetectionThreshold                  0x37
#define FSKRegTimerResol                           0x38
#define FSKRegTimer1Coef                           0x39
#define LORARegSyncWord                            0x39
#define FSKRegTimer2Coef                           0x3A
#define FSKRegImageCal                             0x3B
#define FSKRegTemp                                 0x3C
#define FSKRegLowBat                               0x3D
#define FSKRegIrqFlags1                            0x3E
#define FSKRegIrqFlags2                            0x3F
#define RegDioMapping1                             0x40 // common
#define RegDioMapping2                             0x41 // common
#define RegVersion                                 0x42 // common
// #define RegAgcRef                                  0x43 // common
// #define RegAgcThresh1                              0x44 // common
// #define RegAgcThresh2                              0x45 // common
// #define RegAgcThresh3                              0x46 // common
// #define RegPllHop                                  0x4B // common
// #define RegTcxo                                    0x58 // common
#if CFG_RADIO == SX1276
    #define RegPaDac                                   0x4D // common
#else
    #define RegPaDac                                   0x5A // common
#endif
// #define RegPll                                     0x5C // common
// #define RegPllLowPn                                0x5E // common
// #define RegFormerTemp                              0x6C // common
// #define RegBitRateFrac                             0x70 // common


// sync word for lora networks (nibbles swapped)
#define LORA_MAC_SYNC           0x34


// ---------------------------------------- 
// Constants for radio registers
#define OPMODE_LORA      0x80
#define OPMODE_LOWFREQ   0x08
#define OPMODE_MODE_MASK      0x07

#define LORA_CONFIG1_BW_MASK    0xF0
#define LORA_CONFIG1_BW_SHIFT   4
#define LORA_CONFIG1_CR_MASK    0x0E
#define LORA_CONFIG1_CR_SHIFT   1
#define LORA_CONFIG1_IMPLICIT_HEADER   0x01

#define LORA_CONFIG2_SF_MASK    0xF0
#define LORA_CONFIG2_SF_SHIFT   4
#define LORA_CONFIG2_CRC_ON     0x04

#define LORA_CONFIG3_LOW_DATA_RATE   0x08
#define LORA_CONFIG3_AGC_ON          0x04

#define LORA_DETECTIONTHRESH_SF7_TO_SF12            0x0A // Default
#define LORA_DETECTIONTHRESH_SF6                    0x0C

#define LORA_DETECTIONOPTIMIZE_MASK                 0x07
#define LORA_DETECTIONOPTIMIZE_SF7_TO_SF12          0x03 // Default
#define LORA_DETECTIONOPTIMIZE_SF6                  0x05

// transmit power configuration for RegPaConfig
#define PACONFIG_PA_BOOST       0x80
#define PACONFIG_MAXPOWER_MASK  0x70
#define PACONFIG_MAXPOWER_SHIFT 4
#define PACONFIG_OUTPOWER_MASK  0x0F
#define PACONFIG_OUTPOWER_SHIFT 0
                                                    


// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK 0x80
#define IRQ_LORA_RXDONE_MASK 0x40
#define IRQ_LORA_CRCERR_MASK 0x20
#define IRQ_LORA_HEADER_MASK 0x10
#define IRQ_LORA_TXDONE_MASK 0x08
#define IRQ_LORA_CDDONE_MASK 0x04
#define IRQ_LORA_FHSSCH_MASK 0x02
#define IRQ_LORA_CDDETD_MASK 0x01

#define IRQ_FSK1_MODEREADY_MASK         0x80
#define IRQ_FSK1_RXREADY_MASK           0x40
#define IRQ_FSK1_TXREADY_MASK           0x20
#define IRQ_FSK1_PLLLOCK_MASK           0x10
#define IRQ_FSK1_RSSI_MASK              0x08
#define IRQ_FSK1_TIMEOUT_MASK           0x04
#define IRQ_FSK1_PREAMBLEDETECT_MASK    0x02
#define IRQ_FSK1_SYNCADDRESSMATCH_MASK  0x01
#define IRQ_FSK2_FIFOFULL_MASK          0x80
#define IRQ_FSK2_FIFOEMPTY_MASK         0x40
#define IRQ_FSK2_FIFOLEVEL_MASK         0x20
#define IRQ_FSK2_FIFOOVERRUN_MASK       0x10
#define IRQ_FSK2_PACKETSENT_MASK        0x08
#define IRQ_FSK2_PAYLOADREADY_MASK      0x04
#define IRQ_FSK2_CRCOK_MASK             0x02
#define IRQ_FSK2_LOWBAT_MASK            0x01

// ----------------------------------------
// DIO function mappings                D0D1D2D3
#define MAP_DIO0_LORA_RXDONE   0x00  // 00------
#define MAP_DIO0_LORA_TXDONE   0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT   0x00  // --00----
#define MAP_DIO1_LORA_NOP      0x30  // --11----
#define MAP_DIO2_LORA_NOP      0xC0  // ----11--

#define MAP_DIO0_FSK_READY     0x00  // 00------ (packet sent / payload ready)
#define MAP_DIO1_FSK_NOP       0x30  // --11----
#define MAP_DIO2_FSK_TXNOP     0x04  // ----01--
#define MAP_DIO2_FSK_TIMEOUT   0x08  // ----10--


// FSK IMAGECAL defines
#define RF_IMAGECAL_AUTOIMAGECAL_MASK               0x7F
#define RF_IMAGECAL_AUTOIMAGECAL_ON                 0x80
#define RF_IMAGECAL_AUTOIMAGECAL_OFF                0x00  // Default

#define RF_IMAGECAL_IMAGECAL_START                  0x40

#define RF_IMAGECAL_IMAGECAL_RUNNING                0x20
#define RF_IMAGECAL_IMAGECAL_DONE                   0x00  // Default
