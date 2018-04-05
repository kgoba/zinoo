#include "rfm96.h"

#include "systick.h"

#include <ptlib/ptlib.h>

typedef SPI_T<SPI1, PB_5, PB_4, PB_3> SPI;
typedef DigitalOut<PA_9>  RST;
typedef DigitalOut<PA_10> NSS;

void RFM96::init() {
    SX1276_Base::init();
    SX1276_Base::setPABoost(true);
}

uint8_t RFM96::hal_spi_transfer (uint8_t outval) {
    uint8_t value = SPI::write(outval);
    //while (SPI::is_busy()) {}
    return value;
}

void RFM96::hal_pin_nss (uint8_t val) {
    //hal_delay_us(1000);
    NSS::write(val);
    //hal_delay_us(1000);
}

void RFM96::hal_pin_rxtx (uint8_t val) {
}

void RFM96::hal_pin_rst (uint8_t val) {
    RST::write(val);
}

void RFM96::hal_disableIRQs (void) {

}

void RFM96::hal_enableIRQs (void) {

}

void RFM96::hal_delay_us (uint32_t us) {
    delay_us(us);    
}

// uint32_t RFM96::hal_ticks (void) {
//     return millis();
// }

// void RFM96::hal_waitUntil (uint32_t time) {
//     delay(time - millis());
// }

// uint32_t RFM96::hal_ms2ticks(uint32_t ms) {
//     return ms;
// }
