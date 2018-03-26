#pragma once

#include <libopencm3/stm32/usart.h>

#include "rcc.h"

template<class BasicIO> 
class TextOutStream : public BasicIO {
public:
    static void print(const char *str) {
        while (*str) {
            BasicIO::write(*str);
            str++;
        }
    }
};

/*
template<typename Pin_tx, typename Pin_rx>
class USARTPinPackage {
public:
    const static uint32_t usart;
};

template<> const uint32_t USARTPinPackage<PA9, PA10>::clock = USART1;
template<> const uint32_t USARTPinPackage<PB6, PB7>::clock = USART1;
template<> const uint32_t USARTPinPackage<PA2, PA3>::clock = USART2;
template<> const uint32_t USARTPinPackage<PA14, PA15>::clock = USART2;
*/

template<uint32_t usart, typename Pin_tx, typename Pin_rx>
class USART : public RCC<usart>, public NVIC<usart> {
public:
    static void begin(int baudrate = 9600) {
        DigitalAF<Pin_tx, usart>::begin(IODirection::OutputPP);
        DigitalAF<Pin_rx, usart>::begin(IODirection::Input);

        RCC<usart>::enableClock();

        usart_set_baudrate(usart, baudrate);
        usart_set_databits(usart, 8);
        usart_set_parity(usart, USART_PARITY_NONE);
        usart_set_stopbits(usart, USART_STOPBITS_1);
        usart_set_mode(usart, USART_MODE_TX_RX);
        usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);

        /* Finally enable the USART. */
        usart_enable(usart);
    }

    static void shutdown() {
        usart_disable(usart);
        RCC<usart>::disableClock();
    }

    static void baudrate(uint32_t baud) {
        usart_set_baudrate(usart, baud);
    }

    static int read() {
        if (!usart_get_flag(usart, USART_ISR_RXNE)) return -1;    // or USART_SR_RXNE
        return usart_recv(usart);
    }
    
    static int write(int c) {
        usart_wait_send_ready(usart);
        usart_send(usart, c);
    }

    static void enableRXInterrupt() {
        usart_enable_rx_interrupt(usart);
    }

    static void disableRXInterrupt() {
        usart_disable_rx_interrupt(usart);
    }

    static void enableTXInterrupt() {
        usart_enable_tx_interrupt(usart);
    }

    static void disableTXInterrupt() {
        usart_disable_tx_interrupt(usart);
    }
};
