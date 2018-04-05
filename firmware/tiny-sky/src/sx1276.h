#pragma once

#include <stdint.h>

class SPIDeviceBase {
public:
    void writeReg (uint8_t addr, uint8_t data);
    uint8_t readReg (uint8_t addr);
    void writeBuf (uint8_t addr, const uint8_t * buf, uint8_t len);
    void readBuf (uint8_t addr, uint8_t * buf, uint8_t len);

protected:
    // Abstract virtual methods to be implemented in a subclass

    /*
    * perform 8-bit SPI transaction
    *   - write given byte 'outval'
    *   - read byte and return value
    */
    virtual uint8_t hal_spi_transfer (uint8_t outval) = 0;

    /*
    * drive device NSS pin (0=low, 1=high).
    */
    virtual void hal_pin_nss (uint8_t val) = 0;
};

class SX1276_Base : public SPIDeviceBase {
public:
    enum sf_t {
        eSF_6       = 6,
        eSF_7       = 7,
        eSF_8       = 8,
        eSF_9       = 9,
        eSF_10      = 10,
        eSF_11      = 11,
        eSF_12      = 12
    };

    enum cr_t { 
        eCR_4_5     = 1, 
        eCR_4_6     = 2, 
        eCR_4_7     = 3, 
        eCR_4_8     = 4
    };

    enum bw_t { 
        eBW_500k    = 9, 
        eBW_250k    = 8, 
        eBW_125k    = 7, 
        eBW_62k5    = 6, 
        eBW_41k7    = 5, 
        eBW_31k25   = 4, 
        eBW_20k8    = 3, 
        eBW_15k6    = 2, 
        eBW_10k4    = 1, 
        eBW_7k8     = 0 
    };

    struct ModemSettings {
        sf_t        spreading_factor;
        bw_t        bandwidth;
        cr_t        coding_rate;
        uint16_t    preamble_length;
        uint8_t     sync_word;
        bool        disable_crc;
        bool        implicit_header;

        ModemSettings() :
            spreading_factor(eSF_7),    // SF 2^7 = 128
            bandwidth(eBW_125k),        // 125 kHz
            coding_rate(eCR_4_5),       // 4/5 CR
            preamble_length(8),         // 8 bytes
            sync_word(0x12),            // private network
            disable_crc(false),         // CRC enabled
            implicit_header(false)      // explicit header
        {
            // Default settings: 
            // Rs = 976.5625 sym/s, Ts = 1.024 ms
            // Tpreamble = 12.544 ms
        }

        static ModemSettings defaults() { return ModemSettings(); }
        ModemSettings & setSF(sf_t new_sf) { spreading_factor = new_sf; return *this; }
    };

    void init(void);
    bool checkVersion();
    
    void setFrequencyHz (uint32_t frequency);
    void setFrequencyMHz (float frequency);
    void setTXPower(int8_t power_dbm);
    void setPABoost(bool enabled);

    //void configureModem(sf_t spreading_factor, bw_t bandwidth, cr_t coding_rate);
    void setupLoRa(const ModemSettings &settings);
    
    void writeFIFO(const uint8_t *data, uint8_t length);
    void readFIFO(uint8_t *data, uint8_t &length);

    void startTX();
    void startRX();
    void sleep();

    int8_t getRSSI();
    int8_t getPacketRSSI();
    int8_t getPacketSNR();

    uint8_t getIRQFlags();

    void handleIRQ();

protected:
    enum lr_mode_t { 
        eLR_MODE_FSK    = 0,
        eLR_MODE_LORA   = 1 
    };

    enum mode_t { 
        eMODE_SLEEP     = 0, 
        eMODE_STDBY     = 1, 
        eMODE_FSTX      = 2, 
        eMODE_TX        = 3, 
        eMODE_FSRX      = 4, 
        eMODE_RX        = 5, 
        eMODE_RXSINGLE  = 6, 
        eMODE_CAD       = 7
    };

    void setLRMode (lr_mode_t lr_mode);    // forces sleep mode
    void setMode (mode_t mode);

    //void rxChainCalibration();
    //void seedRandom();

    virtual void onTXDone();
    virtual void onRXDone();
    virtual void onRXTimeout();

protected:
    /* Virtual HAL methods */

    /*
    * drive radio RX/TX pins (0=rx, 1=tx).
    */
    virtual void hal_pin_rxtx (uint8_t val) = 0;

    /*
    * control radio RST pin (0=low, 1=high, 2=floating)
    */
    virtual void hal_pin_rst (uint8_t val) = 0;

    /*
    * disable all CPU interrupts.
    *   - might be invoked nested 
    *   - will be followed by matching call to hal_enableIRQs()
    */
    virtual void hal_disableIRQs (void) = 0;

    /*
    * enable CPU interrupts.
    */
    virtual void hal_enableIRQs (void) = 0;

    /*
    * put system and CPU in low-power mode, sleep until interrupt.
    */
    //virtual void hal_sleep (void) = 0;

    virtual void hal_delay_us (uint32_t us) = 0;

    /*
    * return 32-bit system time in ticks.
    */
    //virtual uint32_t hal_ticks (void) = 0;

    /*
    * busy-wait until specified timestamp (in ticks) is reached.
    */
    //virtual void hal_waitUntil (uint32_t time) = 0;

    //virtual uint32_t hal_ms2ticks(uint32_t ms) = 0;
    
    /*
    * check and rewind timer for target time.
    *   - return 1 if target time is close
    *   - otherwise rewind timer for target time or full period and return 0
    */
    //virtual uint8_t hal_checkTimer (uint32_t targettime) = 0;
};
