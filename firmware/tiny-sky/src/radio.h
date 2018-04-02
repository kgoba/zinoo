#pragma once

#include <stdint.h>

class SPIDeviceBase {
protected:
    void writeReg (uint8_t addr, uint8_t data);
    uint8_t readReg (uint8_t addr);
    void writeBuf (uint8_t addr, uint8_t * buf, uint8_t len);
    void readBuf (uint8_t addr, uint8_t * buf, uint8_t len);

protected:
    // Abstract virtual methods to be implemented in a subclass

    /*
    * perform 8-bit SPI transaction
    *   - write given byte 'outval'
    *   - read byte and return value
    */
    virtual uint8_t hal_spi (uint8_t outval) = 0;

    /*
    * drive device NSS pin (0=low, 1=high).
    */
    virtual void hal_pin_nss (uint8_t val) = 0;

    // virtual uint8_t pinWrite(pin_t pin, uint8_t value) = 0;
    // virtual uint8_t SPIBegin() = 0;
    // virtual uint8_t SPITransfer(uint8_t data = 0) = 0;
    // virtual void    SPITransferBulk(uint8_t *data, unsigned length) = 0;
    // virtual uint8_t SPIEnd() = 0;
};

class SX1276_Base : public SPIDeviceBase {
public:
    bool checkVersion();
    uint8_t rand1 (void);
    void init (void);
    void irq_handler (uint8_t dio);
    uint8_t rssi ();

    void opmode (uint8_t mode);
    void starttx ();
    void startrx (uint8_t rxmode);

private:
    enum mode_t { MODE_LORA, MODE_GFSK };
    enum cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
    enum sf_t { FSK=0, SF7, SF8, SF9, SF10, SF11, SF12, SFrfu };
    enum bw_t { BW125=0, BW250, BW500, BWrfu };

    void rxChainCalibration();
    void seedRandom();
    void opmodeLora();
    void opmodeFSK();
    void configLoraModem ();
    void configChannel ();
    void configPower ();
    void txfsk ();
    void txlora ();
    void rxlora (uint8_t rxmode);
    void rxfsk (uint8_t rxmode);

    enum { RXMODE_SINGLE, RXMODE_SCAN, RXMODE_RSSI };
    
    static const uint8_t rxlorairqmask[];
    //static const uint16_t LORA_RXDONE_FIXUP[];

    struct Settings {
        sf_t sf_;
        cr_t cr_;
        bw_t bw_;
        uint8_t ih_;
        bool noCRC_;
    };

    Settings s;
    uint8_t rxsyms_;
    int8_t txpow_;
    uint32_t freq_;

    bool noRXIQinversion_;

    static constexpr int MAX_LEN_FRAME     = 64;

    uint8_t dataLen_;
    uint8_t frame_[MAX_LEN_FRAME];

    uint32_t    txend_;
    uint32_t    rxtime_;
    int8_t      rssi_;
    int8_t      snr_;

protected:
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

    /*
    * return 32-bit system time in ticks.
    */
    virtual uint32_t hal_ticks (void) = 0;

    /*
    * busy-wait until specified timestamp (in ticks) is reached.
    */
    virtual void hal_waitUntil (uint32_t time) = 0;

    virtual uint32_t hal_ms2ticks(uint32_t ms) = 0;
    
    /*
    * check and rewind timer for target time.
    *   - return 1 if target time is close
    *   - otherwise rewind timer for target time or full period and return 0
    */
    //virtual uint8_t hal_checkTimer (uint32_t targettime) = 0;
};
