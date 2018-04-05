#pragma once

#include "sx1276.h"

class RFM96 : public SX1276_Base {
public:
    void init();

private:
    virtual uint8_t hal_spi_transfer (uint8_t outval) override;
    virtual void hal_pin_nss (uint8_t val) override;
    virtual void hal_pin_rst (uint8_t val) override;
    virtual void hal_pin_rxtx (uint8_t val) override;
    virtual void hal_disableIRQs (void) override;
    virtual void hal_enableIRQs (void) override;

    //virtual uint32_t hal_ticks (void) override;
    //virtual void hal_waitUntil (uint32_t time) override;
    //virtual uint32_t hal_ms2ticks(uint32_t ms) override;
    virtual void hal_delay_us(uint32_t us) override;
};
