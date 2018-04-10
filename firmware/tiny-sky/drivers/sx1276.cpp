/*
 * (c) Karlis Goba 2018
 * 
 * Based on portions of LMIC code by IBM Corporation.
 */

#include "sx1276.h"
#include "sx1276_regs.h"

#define RADIOHEAD_COMPATIBLE
#define F_XOSC_HZ                32000000UL
#define THRESHOLD_LOWFREQ_HZ    525000000UL

void SPIDeviceBase::writeReg (uint8_t addr, uint8_t data ) {
    hal_pin_nss(0);
    hal_spi_transfer(addr | 0x80);
    hal_spi_transfer(data);
    hal_pin_nss(1);
}

uint8_t SPIDeviceBase::readReg (uint8_t addr) {
    hal_pin_nss(0);
    hal_spi_transfer(addr & 0x7F);
    uint8_t val = hal_spi_transfer(0x00);
    hal_pin_nss(1);
    return val;
}

void SPIDeviceBase::writeCommand (uint8_t command) {
    hal_pin_nss(0);
    hal_spi_transfer(command);
    hal_pin_nss(1);
}

void SPIDeviceBase::writeCommand (uint8_t cmd, const uint8_t *data_out, int size_out, uint8_t *data_in, int size_in)
{
    hal_pin_nss(0);
    hal_spi_transfer(cmd);
    for (uint8_t i = 0; i < size_out; i++) {
        hal_spi_transfer(data_out[i]);
    }
    for (uint8_t i = 0; i < size_in; i++) {
        data_in[i] = hal_spi_transfer(0x00);
    }
    hal_pin_nss(1);    
}

void SPIDeviceBase::writeBuf (uint8_t addr, const uint8_t * buf, uint8_t len) {
    hal_pin_nss(0);
    hal_spi_transfer(addr | 0x80);
    for (uint8_t i=0; i<len; i++) {
        hal_spi_transfer(buf[i]);
    }
    hal_pin_nss(1);
}

void SPIDeviceBase::readBuf (uint8_t addr, uint8_t * buf, uint8_t len) {
    hal_pin_nss(0);
    hal_spi_transfer(addr & 0x7F);
    for (uint8_t i=0; i<len; i++) {
        buf[i] = hal_spi_transfer(0x00);
    }
    hal_pin_nss(1);
}



// get random seed from wideband noise rssi
void SX1276_Base::init () {
    // manually reset radio
    hal_pin_rst(0); // drive RST pin low
    hal_delay_us(500);   // wait >=100us
    hal_pin_rst(1); // drive RST pin high
    hal_delay_us(10000);   // wait >=5ms

    setupLoRa(ModemSettings()); // setup LoRa with the default modem settings

    writeReg(LORARegFifoRxBaseAddr, 0);
    writeReg(LORARegFifoTxBaseAddr, 0);
    writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec

    // set max payload size
    //writeReg(LORARegPayloadMaxLength, 255); // unnecessary - it's the default after reset
    //seedRandom();
    //rxChainCalibration();
    //setTXPower(5, false);    // The minimum dBm setting that works for PA_BOOST and RFO modes

    // uint8_t u = readReg(RegPaConfig) & ~PACONFIG_PA_BOOST;
    // if () {
    //     u |= PACONFIG_PA_BOOST);
    // }
    // writeReg(RegPaConfig, u);

    setMode(eMODE_STDBY);
}

bool SX1276_Base::checkVersion() {
    // some sanity checks, e.g., read version number
    uint8_t v = readReg(RegVersion);
    return (v == 0x12); // SX1272: (v == 0x22);
}

void SX1276_Base::setMode (mode_t mode) {
    uint8_t u = readReg(RegOpMode) & ~OPMODE_MODE_MASK;
    u |= mode;
    writeReg(RegOpMode, u);
}

void SX1276_Base::setLRMode (lr_mode_t lr_mode) {
    // We only keep the LowFreq setting and force sleep mode (0)
    uint8_t u = readReg(RegOpMode) & OPMODE_LOWFREQ;
    u |= ((uint8_t)lr_mode) << 7;
    writeReg(RegOpMode, u);
}

void SX1276_Base::setFrequencyHz (uint32_t frequency) {
    // set frequency: FQ = (FRF * F_XOSC_HZ) / (2 ^ 19)
    uint32_t frf = ((uint64_t)frequency << 19) / F_XOSC_HZ;
    writeReg(RegFrfMsb, (uint8_t)(frf>>16));
    writeReg(RegFrfMid, (uint8_t)(frf>> 8));
    writeReg(RegFrfLsb, (uint8_t)(frf>> 0));

    uint8_t opmode = readReg(RegOpMode) & ~OPMODE_LOWFREQ;
    if (frequency < THRESHOLD_LOWFREQ_HZ) {
        opmode |= OPMODE_LOWFREQ;
    }
    writeReg(RegOpMode, opmode);
}

void SX1276_Base::setFrequencyMHz (float frequency) {
    // set frequency: FQ = (FRF * F_XOSC_HZ) / (2 ^ 19)
    uint32_t frf = (0.5f + frequency * (524288.0f * 1E6f / F_XOSC_HZ));
    writeReg(RegFrfMsb, (uint8_t)(frf>>16));
    writeReg(RegFrfMid, (uint8_t)(frf>> 8));
    writeReg(RegFrfLsb, (uint8_t)(frf>> 0));

    uint8_t opmode = readReg(RegOpMode) & ~OPMODE_LOWFREQ;
    if (frequency < THRESHOLD_LOWFREQ_HZ / 1E6f) {
        opmode |= OPMODE_LOWFREQ;
    }
    writeReg(RegOpMode, opmode);
}

void SX1276_Base::setPABoost(bool enabled) {
    uint8_t u = readReg(RegPaConfig) & ~PACONFIG_PA_BOOST;
    if (enabled) {
        u |= PACONFIG_PA_BOOST;
    }
    writeReg(RegPaConfig, u);    
}

void SX1276_Base::setTXPower (int8_t power_dbm) {
    uint8_t paConfig = readReg(RegPaConfig) & PACONFIG_PA_BOOST;

    if (paConfig & PACONFIG_PA_BOOST) {
        // Valid range 5 .. 23 dBm
        if (power_dbm >= 23) power_dbm = 23;
        else if (power_dbm < 5) power_dbm = 5;

        uint8_t paDAC = readReg(RegPaDac) & 0xF8;   // Retain reserved values
        if (power_dbm > 20) {
            paDAC |= 0x07;   // +20dBm on PA_BOOST when OutputPower=1111
            power_dbm -= 3;
        }
        else {
            paDAC |= 0x04;   // Default value
        }
        writeReg(RegPaDac, paDAC);

        paConfig |= (power_dbm - 5) << PACONFIG_OUTPOWER_SHIFT;  /* OutputPower */
    }
    else {
        // Valid range -1 .. 14 dBm
        if (power_dbm >= 14) power_dbm = 14;
        else if (power_dbm < -1) power_dbm = -1;
        paConfig |= 5 << PACONFIG_MAXPOWER_SHIFT;   /* MaxPower */
        paConfig |= (power_dbm + 1) << PACONFIG_OUTPOWER_SHIFT;  /* OutputPower */
    }

    writeReg(RegPaConfig, paConfig);
}

void SX1276_Base::setupLoRa(const ModemSettings &ms) {
    setLRMode(eLR_MODE_LORA);   // force sleep mode

    //setupLoRa(ms.spreading_factor, ms.bandwidth, ms.coding_rate);
    //uint8_t cfg1 = readReg(LORARegModemSettings1) & ~(LORA_CONFIG1_CR_MASK | LORA_CONFIG1_BW_MASK | LORA_CONFIG1_IMPLICIT_HEADER);
    uint8_t cfg1 = 0;
    uint8_t cfg2 = readReg(LORARegModemConfig2) & ~(LORA_CONFIG2_SF_MASK | LORA_CONFIG2_CRC_ON);
    uint8_t cfg3 = readReg(LORARegModemConfig3) & ~(LORA_CONFIG3_LOW_DATA_RATE); //  LORA_CONFIG3_AGC_ON

    cfg1 |= (uint8_t)ms.coding_rate << LORA_CONFIG1_CR_SHIFT;
    cfg1 |= (uint8_t)ms.bandwidth << LORA_CONFIG1_BW_SHIFT;
    if (ms.implicit_header || ms.spreading_factor == eSF_6) {
        cfg1 |= LORA_CONFIG1_IMPLICIT_HEADER;
    }

    cfg2 |= (uint8_t)ms.spreading_factor << LORA_CONFIG2_SF_SHIFT;
    if (!ms.disable_crc) {
        cfg2 |= LORA_CONFIG2_CRC_ON;
    }

    if (((ms.spreading_factor == eSF_11 || ms.spreading_factor == eSF_12) && ms.bandwidth == eBW_125k) ||
         (ms.spreading_factor == eSF_12 && ms.bandwidth == eBW_250k)) 
    {
        cfg3 |= LORA_CONFIG3_LOW_DATA_RATE;
    }
    //cfg3 |= LORA_CONFIG3_AGC_ON;

    writeReg(LORARegModemConfig1, cfg1);
    writeReg(LORARegModemConfig2, cfg2);
    writeReg(LORARegModemConfig3, cfg3);

    if (ms.spreading_factor == eSF_6) {
        uint8_t u = readReg(LORARegDetectOptimize) & ~LORA_DETECTIONOPTIMIZE_MASK;
        writeReg(LORARegDetectOptimize, u | LORA_DETECTIONOPTIMIZE_SF6);
        writeReg(LORARegDetectionThreshold, LORA_DETECTIONTHRESH_SF6);
    }
    else {
        uint8_t u = readReg(LORARegDetectOptimize) & ~LORA_DETECTIONOPTIMIZE_MASK;
        writeReg(LORARegDetectOptimize, u | LORA_DETECTIONOPTIMIZE_SF7_TO_SF12);
        writeReg(LORARegDetectionThreshold, LORA_DETECTIONTHRESH_SF7_TO_SF12);
    }

    writeReg(LORARegPreambleLsb, (uint8_t)(ms.preamble_length >> 0));
    writeReg(LORARegPreambleMsb, (uint8_t)(ms.preamble_length >> 8));
    writeReg(LORARegSyncWord, ms.sync_word);
}

void SX1276_Base::writeFIFO(const uint8_t *data, uint8_t length) {
    // enter standby mode (required for LoRa FIFO loading)
    setMode(eMODE_STDBY);

#ifdef RADIOHEAD_COMPATIBLE
    writeReg(LORARegPayloadLength, length + 4);
#else
    writeReg(LORARegPayloadLength, length);
#endif
    writeReg(LORARegFifoAddrPtr, 0);

    // Write message data

#ifdef RADIOHEAD_COMPATIBLE
    // The headers
    writeReg(RegFifo, 0xFF);    // to:      broadcast
    writeReg(RegFifo, 0xFF);    // from:    broadcast
    writeReg(RegFifo, 0x00);    // id:      0
    writeReg(RegFifo, 0x00);    // flags:   0
#endif

    writeBuf(RegFifo, data, length);
}

void SX1276_Base::readFIFO(uint8_t *data, uint8_t &length) {
    // The PayloadCrcError flag should be checked for packet integrity.
    // In order to retrieve received data from FIFO the user must ensure that  
    // ValidHeader, PayloadCrcError, RxDone and RxTimeout interrupts in the 
    // status register RegIrqFlags are not asserted to ensure that 
    // packet reception has terminated successfully (i.e. no flags should be set). 

    // Position at the beginning of the FIFO
    writeReg(LORARegFifoAddrPtr, readReg(LORARegFifoRxCurrentAddr));
    uint8_t n_recv = readReg(LORARegRxNbBytes);
    // Read message data
    if (n_recv <= length) length = n_recv;
    readBuf(RegFifo, data, length);
}

void SX1276_Base::startTX() {
    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    // mask all IRQs but TxDone
    writeReg(LORARegIrqFlagsMask, ~IRQ_LORA_TXDONE_MASK);

    // enable antenna switch for TX
    hal_pin_rxtx(1);

    setMode(eMODE_TX); // Start the transmitter
    // Datasheet: Upon completion the TxDone interrupt is issued and the radio returns to Standby mode.
}

// start LoRa receiver (time=LMIC.rxtime, timeout=LMIC.rxsyms, result=LMIC.frame[LMIC.dataLen])
void SX1276_Base::startRX () {
    // enter standby mode (warm up))
    setMode(eMODE_STDBY);

    // set LNA gain
    //writeReg(RegLna, LNA_RX_GAIN); 

    // use inverted I/Q signal (prevent mote-to-mote communication)
    //writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ)|(1<<6));

    // set symbol timeout (for single rx)
    //writeReg(LORARegSymbTimeoutLsb, rxsyms_);
    
    writeReg(LORARegFifoAddrPtr, 0);

    // configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_RXDONE|MAP_DIO1_LORA_RXTOUT|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    // enable required radio IRQs
    writeReg(LORARegIrqFlagsMask, ~IRQ_LORA_RXDONE_MASK);   // IRQ_LORA_RXTOUT_MASK

    // enable antenna switch for RX
    hal_pin_rxtx(0);

    setMode(eMODE_RX); 
    // Datasheet: In continuous RX mode, opposite to the single RX mode,
    // the RxTimeout interrupt will never occur 
    // and the device will never go in Standby mode automatically. 
}

void SX1276_Base::sleep() {
    setMode(eMODE_SLEEP);
}

int8_t SX1276_Base::getRSSI () {
    int8_t r = readReg(LORARegRssiValue) - 157; // -164 in LF
    return r;
}

int8_t SX1276_Base::getPacketRSSI () {
    int8_t r = readReg(LORARegPktRssiValue) - 157;  // -164 in LF
    return r;
}

int8_t SX1276_Base::getPacketSNR () {
    int8_t r = readReg(LORARegPktSnrValue) / 4;
    return r;
}

uint8_t SX1276_Base::getIRQFlags () {
    readReg(LORARegIrqFlags);
}

// const uint16_t SX1276_Base::LORA_RXDONE_FIXUP[] = {
//     [FSK]  =     us2osticks(0), // (   0 ticks)
//     [SF7]  =     us2osticks(0), // (   0 ticks)
//     [SF8]  =  us2osticks(1648), // (  54 ticks)
//     [SF9]  =  us2osticks(3265), // ( 107 ticks)
//     [SF10] =  us2osticks(7049), // ( 231 ticks)
//     [SF11] = us2osticks(13641), // ( 447 ticks)
//     [SF12] = us2osticks(31189), // (1022 ticks)
// };

// called by hal ext IRQ handler
// (radio goes to stanby mode after tx/rx operations)
void SX1276_Base::handleIRQ() {
    uint8_t flags = readReg(LORARegIrqFlags);
    // mask all radio IRQs
    writeReg(LORARegIrqFlagsMask, 0xFF);
    // clear radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);

    // go from stanby to sleep
    //setMode(eMODE_SLEEP);

    if (flags & IRQ_LORA_TXDONE_MASK) {
        onTXDone();
    }
    if (flags & IRQ_LORA_RXDONE_MASK) {
        onRXDone();
        // dataLen_ = (readReg(LORARegModemSettings1) & SX1272_MC1_IMPLICIT_HEADER_MODE_ON) ?
        //     readReg(LORARegPayloadLength) : readReg(LORARegRxNbBytes);
        // // set FIFO read address pointer
        // writeReg(LORARegFifoAddrPtr, readReg(LORARegFifoRxCurrentAddr)); 
        // // now read the FIFO
        // readBuf(RegFifo, frame_, dataLen_);
    }
    if (flags & IRQ_LORA_RXTOUT_MASK) {
        onRXTimeout();
    }

    // else { // FSK modem
    //     uint8_t flags1 = readReg(FSKRegIrqFlags1);
    //     uint8_t flags2 = readReg(FSKRegIrqFlags2);
    //     if( flags2 & IRQ_FSK2_PACKETSENT_MASK ) {
    //         onTXDone();
    //     } else if( flags2 & IRQ_FSK2_PAYLOADREADY_MASK ) {
    //         onRXDone();
    //         // // save exact rx time
    //         // rxtime_ = now;
    //         // // read the PDU and inform the MAC that we received something
    //         // dataLen_ = readReg(FSKRegPayloadLength);
    //         // // now read the FIFO
    //         // readBuf(RegFifo, frame_, dataLen_);
    //     } else if( flags1 & IRQ_FSK1_TIMEOUT_MASK ) {
    //         onRXTimeout();
    //         // indicate timeout
    //         //dataLen_ = 0;
    //     } else {
    //         //while(1);
    //     }
    // }
}

void SX1276_Base::onTXDone() {

}

void SX1276_Base::onRXDone() {

}

void SX1276_Base::onRXTimeout() {

}

// void SX1276_Base::seedRandom() {
//     // seed 15-byte randomness via noise rssi
//     rxlora(RXMODE_RSSI);
//     while( (readReg(RegOpMode) & OPMODE_MODE_MASK) != OPMODE_RX ); // continuous rx
//     for(int i=1; i<16; i++) {
//         for(int j=0; j<8; j++) {
//             uint8_t b; // wait for two non-identical subsequent least-significant bits
//             while( (b = readReg(LORARegRssiWideband) & 0x01) == (readReg(LORARegRssiWideband) & 0x01) );
//             randbuf[i] = (randbuf[i] << 1) | b;
//         }
//     }
//     randbuf[0] = 16; // set initial index    
// }

// void SX1276_Base::rxChainCalibration() {
//     // chain calibration
//     writeReg(RegPaConfig, 0);
    
//     // Launch Rx chain calibration for LF band
//     uint8_t cal = readReg(FSKRegImageCal);
//     writeReg(FSKRegImageCal, cal | RF_IMAGECAL_IMAGECAL_START);

//     while((readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING)
//         { ; }

//     // Sets a Frequency in HF band
//     uint32_t frf = 868000000;
//     writeReg(RegFrfMsb, (uint8_t)(frf>>16));
//     writeReg(RegFrfMid, (uint8_t)(frf>> 8));
//     writeReg(RegFrfLsb, (uint8_t)(frf>> 0));

//     // Launch Rx chain calibration for HF band 
//     writeReg(FSKRegImageCal, readReg(FSKRegImageCal) | RF_IMAGECAL_IMAGECAL_START);
    
//     while((readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) 
//         { ; }    
// }