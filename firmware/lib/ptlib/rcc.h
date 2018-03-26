#pragma once

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>

#include <libopencm3/cm3/nvic.h>

template<uint32_t periph>
class RCC {
public:
    static void enableClock() {
        rcc_periph_clock_enable(clock);        
    }
    static void disableClock() {
        rcc_periph_clock_disable(clock);
    }
    const static rcc_periph_clken clock;
};


template<> const rcc_periph_clken RCC<GPIOA>::clock = RCC_GPIOA;
template<> const rcc_periph_clken RCC<GPIOB>::clock = RCC_GPIOB;
template<> const rcc_periph_clken RCC<GPIOC>::clock = RCC_GPIOC;
template<> const rcc_periph_clken RCC<GPIOD>::clock = RCC_GPIOD;

#ifndef STM32L0
template<> const rcc_periph_clken RCC<GPIOF>::clock = RCC_GPIOF;
#endif

template<> const rcc_periph_clken RCC<USART1>::clock = RCC_USART1;
template<> const rcc_periph_clken RCC<USART2>::clock = RCC_USART2;

template<> const rcc_periph_clken RCC<SPI1>::clock = RCC_SPI1;
template<> const rcc_periph_clken RCC<SPI2>::clock = RCC_SPI2;

template<> const rcc_periph_clken RCC<I2C1>::clock = RCC_I2C1;
template<> const rcc_periph_clken RCC<I2C2>::clock = RCC_I2C2;

template<uint32_t periph>
class NVIC {
public:
    static void enableIRQ() {
        nvic_enable_irq(irq);
    }
    static void disableIRQ() {
        nvic_disable_irq(irq);
    }
    const static uint8_t irq;
};

template<> const uint8_t NVIC<USART1>::irq = NVIC_USART1_IRQ;
template<> const uint8_t NVIC<USART2>::irq = NVIC_USART2_IRQ;
