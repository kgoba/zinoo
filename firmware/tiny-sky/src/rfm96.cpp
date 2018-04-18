#include "rfm96.h"

#include "systick.h"

#include <ptlib/ptlib.h>

typedef SPI<SPI1, PB_5, PB_4, PB_3> SPI_;

typedef DigitalOut<PA_9>  RFM96_RST;
typedef DigitalOut<PA_10> RFM96_NSS;

typedef DigitalOut<PA_15> Flash_NSS;

void RFM96::init() {
    SX1276_Base::init();
    SX1276_Base::setPABoost(true);
}

uint8_t RFM96::hal_spi_transfer (uint8_t outval) {
    uint8_t value = SPI_::write(outval);
    //while (SPI::is_busy()) {}
    return value;
}

void RFM96::hal_pin_nss (uint8_t val) {
    //hal_delay_us(1000);
    RFM96_NSS::write(val);
    //hal_delay_us(1000);
}

void RFM96::hal_pin_rxtx (uint8_t val) {
}

void RFM96::hal_pin_rst (uint8_t val) {
    RFM96_RST::write(val);
}

void RFM96::hal_disableIRQs (void) {

}

void RFM96::hal_enableIRQs (void) {

}

void RFM96::hal_delay_us (uint32_t us) {
    delay_us(us);    
}



void SPIFlash::select() {
    Flash_NSS::write(0);
}

void SPIFlash::release() {
    Flash_NSS::write(1);
}

void SPIFlash::transfer(const uint8_t *wr_buf, int wr_len, uint8_t *rd_buf, int rd_len) {
    while (wr_len > 0) {
        SPI_::write(*wr_buf);
        wr_buf++;
        wr_len--;
    }
    while (rd_len > 0) {
        *rd_buf = SPI_::write(0);
        rd_buf++;
        rd_len--;
    }
}
