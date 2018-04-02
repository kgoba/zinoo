/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#include "lmic.h"
#include "radio.h"

#define ASSERT(x)  
 
#define CFG_RADIO SX1276

#include "radio_regs.h"

// RADIO STATE
// (initialized by radio_init(), used by radio_rand1())
static uint8_t randbuf[16];


#if CFG_RADIO == SX1276
#define LNA_RX_GAIN (0x20|0x1)
#else
#define LNA_RX_GAIN (0x20|0x03)
#endif

void SPIDeviceBase::writeReg (uint8_t addr, uint8_t data ) {
    hal_pin_nss(0);
    hal_spi(addr | 0x80);
    hal_spi(data);
    hal_pin_nss(1);
}

uint8_t SPIDeviceBase::readReg (uint8_t addr) {
    hal_pin_nss(0);
    hal_spi(addr & 0x7F);
    uint8_t val = hal_spi(0x00);
    hal_pin_nss(1);
    return val;
}

void SPIDeviceBase::writeBuf (uint8_t addr, uint8_t * buf, uint8_t len) {
    hal_pin_nss(0);
    hal_spi(addr | 0x80);
    for (uint8_t i=0; i<len; i++) {
        hal_spi(buf[i]);
    }
    hal_pin_nss(1);
}

void SPIDeviceBase::readBuf (uint8_t addr, uint8_t * buf, uint8_t len) {
    hal_pin_nss(0);
    hal_spi(addr & 0x7F);
    for (uint8_t i=0; i<len; i++) {
        buf[i] = hal_spi(0x00);
    }
    hal_pin_nss(1);
}



void SX1276_Base::opmode (uint8_t mode) {
    writeReg(RegOpMode, (readReg(RegOpMode) & ~OPMODE_MASK) | mode);
}

void SX1276_Base::opmodeLora() {
    uint8_t u = OPMODE_LORA;
#if CFG_RADIO==SX1276
    u |= 0x8;   // TBD: sx1276 high freq
#endif
    writeReg(RegOpMode, u);
}

void SX1276_Base::opmodeFSK() {
    uint8_t u = 0;
#if CFG_RADIO==SX1276
    u |= 0x8;   // TBD: sx1276 high freq
#endif
    writeReg(RegOpMode, u);
}

// configure LoRa modem (cfg1, cfg2)
void SX1276_Base::configLoraModem () {
#if CFG_RADIO==SX1276
        uint8_t mc1 = 0, mc2 = 0, mc3 = 0;

        switch (s.bw_) {
        case BW125: mc1 |= SX1276_MC1_BW_125; break;
        case BW250: mc1 |= SX1276_MC1_BW_250; break;
        case BW500: mc1 |= SX1276_MC1_BW_500; break;
        default:
            ASSERT(0);
        }
        switch(s.cr_) {
        case CR_4_5: mc1 |= SX1276_MC1_CR_4_5; break;
        case CR_4_6: mc1 |= SX1276_MC1_CR_4_6; break;
        case CR_4_7: mc1 |= SX1276_MC1_CR_4_7; break;
        case CR_4_8: mc1 |= SX1276_MC1_CR_4_8; break;
        default:
            ASSERT(0);
        }

        if (s.ih_) {
            mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
            writeReg(LORARegPayloadLength, s.ih_); // required length
        }
        // set ModemConfig1
        writeReg(LORARegModemConfig1, mc1);

        mc2 = (SX1272_MC2_SF7 + ((s.sf_-1)<<4));
        if (!s.noCRC_) {
            mc2 |= SX1276_MC2_RX_PAYLOAD_CRCON;
        }
        writeReg(LORARegModemConfig2, mc2);
        
        mc3 = SX1276_MC3_AGCAUTO;
        if ((s.sf_ == SF11 || s.sf_ == SF12) && s.bw_ == BW125) {
            mc3 |= SX1276_MC3_LOW_DATA_RATE_OPTIMIZE;
        }
        writeReg(LORARegModemConfig3, mc3);
#else
        uint8_t mc1 = (bw_<<6);

        switch( getCr(LMIC.rps) ) {
        case CR_4_5: mc1 |= SX1272_MC1_CR_4_5; break;
        case CR_4_6: mc1 |= SX1272_MC1_CR_4_6; break;
        case CR_4_7: mc1 |= SX1272_MC1_CR_4_7; break;
        case CR_4_8: mc1 |= SX1272_MC1_CR_4_8; break;
        }
        
        if ((sf == SF11 || sf == SF12) && getBw(LMIC.rps) == BW125) {
            mc1 |= SX1272_MC1_LOW_DATA_RATE_OPTIMIZE;
        }
        
        if (getNocrc(LMIC.rps) == 0) {
            mc1 |= SX1272_MC1_RX_PAYLOAD_CRCON;
        }
        
        if (getIh(LMIC.rps)) {
            mc1 |= SX1272_MC1_IMPLICIT_HEADER_MODE_ON;
            writeReg(LORARegPayloadLength, getIh(LMIC.rps)); // required length
        }
        // set ModemConfig1
        writeReg(LORARegModemConfig1, mc1);
        
        // set ModemConfig2 (sf, AgcAutoOn=1 SymbTimeoutHi=00)
        writeReg(LORARegModemConfig2, (SX1272_MC2_SF7 + ((sf-1)<<4)) | 0x04);

#if CFG_TxContinuousMode
        // Only for testing
        // set ModemConfig2 (sf, TxContinuousMode=1, AgcAutoOn=1 SymbTimeoutHi=00)
        writeReg(LORARegModemConfig2, (SX1272_MC2_SF7 + ((sf-1)<<4)) | 0x06);
#endif
#endif /* CFG_sx1272_radio */
}

void SX1276_Base::configChannel () {
    // set frequency: FQ = (FRF * 32 Mhz) / (2 ^ 19)
    uint32_t frf = ((uint64_t)freq_ << 19) / 32000000;
    writeReg(RegFrfMsb, (uint8_t)(frf>>16));
    writeReg(RegFrfMid, (uint8_t)(frf>> 8));
    writeReg(RegFrfLsb, (uint8_t)(frf>> 0));
}


void SX1276_Base::configPower () {
#if CFG_RADIO==SX1276
    // no boost used for now
    int8_t pw = txpow_;
    if(pw >= 15) {
        pw = 15;
    } else if(pw < 2) {
        pw = 2;
    }
    // check board type for BOOST pin
    writeReg(RegPaConfig, (uint8_t)(pw&0xf));
    writeReg(RegPaDac, readReg(RegPaDac)|0x4);

#else
    // set PA config (2-17 dBm using PA_BOOST)
    int8_t pw = (int8_t)LMIC.txpow;
    if(pw > 17) {
        pw = 17;
    } else if(pw < 2) {
        pw = 2;
    }
    writeReg(RegPaConfig, (uint8_t)(0x80|(pw-2)));
#endif /* CFG_sx1272_radio */
}

void SX1276_Base::txfsk () {
    // select FSK modem (from sleep mode)
    writeReg(RegOpMode, 0x10); // FSK, BT=0.5
    ASSERT(readReg(RegOpMode) == 0x10);
    // enter standby mode (required for FIFO loading))
    opmode(OPMODE_STANDBY);
    // set bitrate
    writeReg(FSKRegBitrateMsb, 0x02); // 50kbps
    writeReg(FSKRegBitrateLsb, 0x80);
    // set frequency deviation
    writeReg(FSKRegFdevMsb, 0x01); // +/- 25kHz
    writeReg(FSKRegFdevLsb, 0x99);
    // frame and packet handler settings
    writeReg(FSKRegPreambleMsb, 0x00);
    writeReg(FSKRegPreambleLsb, 0x05);
    writeReg(FSKRegSyncConfig, 0x12);
    writeReg(FSKRegPacketConfig1, 0xD0);
    writeReg(FSKRegPacketConfig2, 0x40);
    writeReg(FSKRegSyncValue1, 0xC1);
    writeReg(FSKRegSyncValue2, 0x94);
    writeReg(FSKRegSyncValue3, 0xC1);
    // configure frequency
    configChannel();
    // configure output power
    configPower();

    // set the IRQ mapping DIO0=PacketSent DIO1=NOP DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_FSK_READY|MAP_DIO1_FSK_NOP|MAP_DIO2_FSK_TXNOP);

    // initialize the payload size and address pointers    
    writeReg(FSKRegPayloadLength, dataLen_+1); // (insert length byte into payload))

    // download length byte and buffer to the radio FIFO
    writeReg(RegFifo, dataLen_);
    writeBuf(RegFifo, frame_, dataLen_);

    // enable antenna switch for TX
    hal_pin_rxtx(1);
    
    // now we actually start the transmission
    opmode(OPMODE_TX);
}

void SX1276_Base::txlora () {
    // select LoRa modem (from sleep mode)
    //writeReg(RegOpMode, OPMODE_LORA);
    opmodeLora();
    ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);

    // enter standby mode (required for FIFO loading))
    opmode(OPMODE_STANDBY);
    // configure LoRa modem (cfg1, cfg2)
    configLoraModem();
    // configure frequency
    configChannel();
    // configure output power
    writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
    configPower();
    // set sync word
    writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);
    
    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    // mask all IRQs but TxDone
    writeReg(LORARegIrqFlagsMask, ~IRQ_LORA_TXDONE_MASK);

    // initialize the payload size and address pointers    
    writeReg(LORARegFifoTxBaseAddr, 0x00);
    writeReg(LORARegFifoAddrPtr, 0x00);
    writeReg(LORARegPayloadLength, dataLen_);
       
    // download buffer to the radio FIFO
    writeBuf(RegFifo, frame_, dataLen_);

    // enable antenna switch for TX
    hal_pin_rxtx(1);
    
    // now we actually start the transmission
    opmode(OPMODE_TX);
}

// start transmitter (buf=frame, len=dataLen)
void SX1276_Base::starttx () {
    ASSERT( (readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP );
    if(s.sf_ == FSK) { // FSK modem
        txfsk();
    } else { // LoRa modem
        txlora();
    }
    // the radio will go back to STANDBY mode as soon as the TX is finished
    // the corresponding IRQ will inform us about completion.
}

const uint8_t SX1276_Base::rxlorairqmask[] = {
    [RXMODE_SINGLE] = IRQ_LORA_RXDONE_MASK|IRQ_LORA_RXTOUT_MASK,
    [RXMODE_SCAN]   = IRQ_LORA_RXDONE_MASK,
    [RXMODE_RSSI]   = 0x00,
};

// start LoRa receiver (time=LMIC.rxtime, timeout=LMIC.rxsyms, result=LMIC.frame[LMIC.dataLen])
void SX1276_Base::rxlora (uint8_t rxmode) {
    // select LoRa modem (from sleep mode)
    opmodeLora();
    ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);
    // enter standby mode (warm up))
    opmode(OPMODE_STANDBY);
    // don't use MAC settings at startup
    if(rxmode == RXMODE_RSSI) { // use fixed settings for rssi scan
        writeReg(LORARegModemConfig1, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1);
        writeReg(LORARegModemConfig2, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2);
    } else { // single or continuous rx mode
        // configure LoRa modem (cfg1, cfg2)
        configLoraModem();
        // configure frequency
        configChannel();
    }
    // set LNA gain
    writeReg(RegLna, LNA_RX_GAIN); 
    // set max payload size
    writeReg(LORARegPayloadMaxLength, 64);
    // use inverted I/Q signal (prevent mote-to-mote communication)

    // XXX: use flag to switch on/off inversion
    if (noRXIQinversion_) {
        writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ) & ~(1<<6));
    } else {
        writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ)|(1<<6));
    }

    // set symbol timeout (for single rx)
    writeReg(LORARegSymbTimeoutLsb, rxsyms_);
    // set sync word
    writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);
    
    // configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_RXDONE|MAP_DIO1_LORA_RXTOUT|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    // enable required radio IRQs
    writeReg(LORARegIrqFlagsMask, ~rxlorairqmask[rxmode]);

    // enable antenna switch for RX
    hal_pin_rxtx(0);

    // now instruct the radio to receive
    if (rxmode == RXMODE_SINGLE) { // single rx
        hal_waitUntil(rxtime_); // busy wait until exact rx time
        opmode(OPMODE_RX_SINGLE);
    } else { // continous rx (scan or rssi)
        opmode(OPMODE_RX); 
    }
}

void SX1276_Base::rxfsk (uint8_t rxmode) {
    // only single rx (no continuous scanning, no noise sampling)
    ASSERT( rxmode == RXMODE_SINGLE );
    // select FSK modem (from sleep mode)
    //writeReg(RegOpMode, 0x00); // (not LoRa)
    opmodeFSK();
    ASSERT((readReg(RegOpMode) & OPMODE_LORA) == 0);
    // enter standby mode (warm up))
    opmode(OPMODE_STANDBY);
    // configure frequency
    configChannel();
    // set LNA gain
    //writeReg(RegLna, 0x20|0x03); // max gain, boost enable
    writeReg(RegLna, LNA_RX_GAIN);
    // configure receiver
    writeReg(FSKRegRxConfig, 0x1E); // AFC auto, AGC, trigger on preamble?!?
    // set receiver bandwidth
    writeReg(FSKRegRxBw, 0x0B); // 50kHz SSb
    // set AFC bandwidth
    writeReg(FSKRegAfcBw, 0x12); // 83.3kHz SSB
    // set preamble detection
    writeReg(FSKRegPreambleDetect, 0xAA); // enable, 2 bytes, 10 chip errors
    // set sync config
    writeReg(FSKRegSyncConfig, 0x12); // no auto restart, preamble 0xAA, enable, fill FIFO, 3 bytes sync
    // set packet config
    writeReg(FSKRegPacketConfig1, 0xD8); // var-length, whitening, crc, no auto-clear, no adr filter
    writeReg(FSKRegPacketConfig2, 0x40); // packet mode
    // set sync value
    writeReg(FSKRegSyncValue1, 0xC1);
    writeReg(FSKRegSyncValue2, 0x94);
    writeReg(FSKRegSyncValue3, 0xC1);
    // set preamble timeout
    writeReg(FSKRegRxTimeout2, 0xFF);//(LMIC.rxsyms+1)/2);
    // set bitrate
    writeReg(FSKRegBitrateMsb, 0x02); // 50kbps
    writeReg(FSKRegBitrateLsb, 0x80);
    // set frequency deviation
    writeReg(FSKRegFdevMsb, 0x01); // +/- 25kHz
    writeReg(FSKRegFdevLsb, 0x99);
    
    // configure DIO mapping DIO0=PayloadReady DIO1=NOP DIO2=TimeOut
    writeReg(RegDioMapping1, MAP_DIO0_FSK_READY|MAP_DIO1_FSK_NOP|MAP_DIO2_FSK_TIMEOUT);

    // enable antenna switch for RX
    hal_pin_rxtx(0);
    
    // now instruct the radio to receive
    hal_waitUntil(rxtime_); // busy wait until exact rx time
    opmode(OPMODE_RX); // no single rx mode available in FSK
}

void SX1276_Base::startrx (uint8_t rxmode) {
    ASSERT( (readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP );
    if(s.sf_ == FSK) { // FSK modem
        rxfsk(rxmode);
    } else { // LoRa modem
        rxlora(rxmode);
    }
    // the radio will go back to STANDBY mode as soon as the RX is finished
    // or timed out, and the corresponding IRQ will inform us about completion.
}

bool SX1276_Base::checkVersion() {
    // some sanity checks, e.g., read version number
    uint8_t v = readReg(RegVersion);
#if CFG_RADIO==SX1276
    return (v == 0x12); 
#else
    return (v == 0x22);
#endif
}

void SX1276_Base::seedRandom() {
    // seed 15-byte randomness via noise rssi
    rxlora(RXMODE_RSSI);
    while( (readReg(RegOpMode) & OPMODE_MASK) != OPMODE_RX ); // continuous rx
    for(int i=1; i<16; i++) {
        for(int j=0; j<8; j++) {
            uint8_t b; // wait for two non-identical subsequent least-significant bits
            while( (b = readReg(LORARegRssiWideband) & 0x01) == (readReg(LORARegRssiWideband) & 0x01) );
            randbuf[i] = (randbuf[i] << 1) | b;
        }
    }
    randbuf[0] = 16; // set initial index    
}

// get random seed from wideband noise rssi
void SX1276_Base::init () {
    hal_disableIRQs();

    // manually reset radio
#if CFG_RADIO == SX1276
    hal_pin_rst(0); // drive RST pin low
#else
    hal_pin_rst(1); // drive RST pin high
#endif
    hal_waitUntil(hal_ticks()+hal_ms2ticks(1)); // wait >100us
    hal_pin_rst(2); // configure RST pin floating!
    hal_waitUntil(hal_ticks()+hal_ms2ticks(5)); // wait 5ms

    opmode(OPMODE_SLEEP);

    seedRandom();
  
#if CFG_RADIO==SX1276
    rxChainCalibration();
#endif /* CFG_RADIO */

    opmode(OPMODE_SLEEP);

    hal_enableIRQs();
}

void SX1276_Base::rxChainCalibration() {
    // chain calibration
    writeReg(RegPaConfig, 0);
    
    // Launch Rx chain calibration for LF band
    writeReg(FSKRegImageCal, (readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_MASK)|RF_IMAGECAL_IMAGECAL_START);
    while((readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING){ ; }

    // Sets a Frequency in HF band
    uint32_t frf = 868000000;
    writeReg(RegFrfMsb, (uint8_t)(frf>>16));
    writeReg(RegFrfMid, (uint8_t)(frf>> 8));
    writeReg(RegFrfLsb, (uint8_t)(frf>> 0));

    // Launch Rx chain calibration for HF band 
    writeReg(FSKRegImageCal, (readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_MASK)|RF_IMAGECAL_IMAGECAL_START);
    while((readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) { ; }    
}

// return next random byte derived from seed buffer
// (buf[0] holds index of next byte to be returned)
// uint8_t rand1 () {
//     uint8_t i = randbuf[0];
//     ASSERT( i != 0 );
//     if( i==16 ) {
//         os_aes(AES_ENC, randbuf, 16); // encrypt seed with any key
//         i = 0;
//     }
//     uint8_t v = randbuf[i++];
//     randbuf[0] = i;
//     return v;
// }

uint8_t SX1276_Base::rssi () {
    hal_disableIRQs();
    uint8_t r = readReg(LORARegRssiValue);
    hal_enableIRQs();
    return r;
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
void SX1276_Base::irq_handler (uint8_t dio) {
#if CFG_TxContinuousMode
    // clear radio IRQ flags
    writeReg(LORARegIrqFlags, 0xFF);
    uint8_t p = readReg(LORARegFifoAddrPtr);
    writeReg(LORARegFifoAddrPtr, 0x00);
    uint8_t s = readReg(RegOpMode);
    uint8_t c = readReg(LORARegModemConfig2);
    opmode(OPMODE_TX);
    return;
#else /* ! CFG_TxContinuousMode */
    //ostime_t now = os_getTime();
    if( (readReg(RegOpMode) & OPMODE_LORA) != 0) { // LORA modem
        uint8_t flags = readReg(LORARegIrqFlags);
        if( flags & IRQ_LORA_TXDONE_MASK ) {
            onTXDone();
            // save exact tx time
            //txend_ = now - us2osticks(43); // TXDONE FIXUP
        } else if( flags & IRQ_LORA_RXDONE_MASK ) {
            onRXDone();
            // // save exact rx time
            // if(bw_ == BW125) {
            //     now -= LORA_RXDONE_FIXUP[sf_];
            // }
            // rxtime_ = now;
            // // read the PDU and inform the MAC that we received something
            // dataLen_ = (readReg(LORARegModemConfig1) & SX1272_MC1_IMPLICIT_HEADER_MODE_ON) ?
            //     readReg(LORARegPayloadLength) : readReg(LORARegRxNbBytes);
            // // set FIFO read address pointer
            // writeReg(LORARegFifoAddrPtr, readReg(LORARegFifoRxCurrentAddr)); 
            // // now read the FIFO
            // readBuf(RegFifo, frame_, dataLen_);
            // // read rx quality parameters
            // snr_  = readReg(LORARegPktSnrValue); // SNR [dB] * 4
            // rssi_ = readReg(LORARegPktRssiValue) - 125 + 64; // RSSI [dBm] (-196...+63)
        } else if( flags & IRQ_LORA_RXTOUT_MASK ) {
            onRXTimeout();
            // indicate timeout
            //dataLen_ = 0;
        }
        // mask all radio IRQs
        writeReg(LORARegIrqFlagsMask, 0xFF);
        // clear radio IRQ flags
        writeReg(LORARegIrqFlags, 0xFF);
    } else { // FSK modem
        uint8_t flags1 = readReg(FSKRegIrqFlags1);
        uint8_t flags2 = readReg(FSKRegIrqFlags2);
        if( flags2 & IRQ_FSK2_PACKETSENT_MASK ) {
            onTXDone();
            // save exact tx time
            //txend_ = now;
        } else if( flags2 & IRQ_FSK2_PAYLOADREADY_MASK ) {
            onRXDone();
            // // save exact rx time
            // rxtime_ = now;
            // // read the PDU and inform the MAC that we received something
            // dataLen_ = readReg(FSKRegPayloadLength);
            // // now read the FIFO
            // readBuf(RegFifo, frame_, dataLen_);
            // // read rx quality parameters
            // snr_  = 0; // determine snr
            // rssi_ = 0; // determine rssi
        } else if( flags1 & IRQ_FSK1_TIMEOUT_MASK ) {
            onRXTimeout();
            // indicate timeout
            //dataLen_ = 0;
        } else {
            //while(1);
        }
    }
    // go from stanby to sleep
    opmode(OPMODE_SLEEP);
    // run os job (use preset func ptr)
    //os_setCallback(&LMIC.osjob, LMIC.osjob.func);
#endif /* ! CFG_TxContinuousMode */
}

void SX1276_Base::onTXDone() {

}

void SX1276_Base::onRXDone() {

}

void SX1276_Base::onRXTimeout() {

}

/*
void os_radio (uint8_t mode) {
    hal_disableIRQs();
    switch (mode) {
      case RADIO_RST:
        // put radio to sleep
        opmode(OPMODE_SLEEP);
        break;

      case RADIO_TX:
        // transmit frame now
        starttx(); // buf=LMIC.frame, len=LMIC.dataLen
        break;
      
      case RADIO_RX:
        // receive frame now (exactly at rxtime)
        startrx(RXMODE_SINGLE); // buf=LMIC.frame, time=LMIC.rxtime, timeout=LMIC.rxsyms
        break;

      case RADIO_RXON:
        // start scanning for beacon now
        startrx(RXMODE_SCAN); // buf=LMIC.frame
        break;
    }
    hal_enableIRQs();
}
*/