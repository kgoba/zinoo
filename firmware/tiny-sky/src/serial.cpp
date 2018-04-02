#include "serial.h"

// template<uint32_t periph>
// class USARTPeriph {
// public:
//     static void onReceived() {
//         if (handle_rxne) handle_rxne();
//     }
//     static void onTransmitEmpty() {
//         if (handle_txe) handle_txe();
//     }
// private:
//     void (* handle_rxne) (void);
//     void (* handle_txe) (void);
// };

extern "C" {
    void usart1_isr() {
        uint32_t flags = USART_ISR(USART1);
        if (flags & USART_ISR_RXNE) {
            SerialGPS::onReceived();
        }
        if (flags & USART_ISR_TXE) {
            SerialGPS::onTransmitEmpty();
        }
    }
}