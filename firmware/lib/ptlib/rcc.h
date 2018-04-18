#pragma once

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/adc.h>

#include <libopencm3/cm3/nvic.h>


template<uint32_t periph>
struct PeriphTraits {};


template<uint32_t periph>
class RCC : public PeriphTraits<periph> {
public:
    static void enableClock() {
        rcc_periph_clock_enable(PeriphTraits<periph>::clock);        
    }
    static void disableClock() {
        rcc_periph_clock_disable(PeriphTraits<periph>::clock);
    }
};


template<uint32_t periph>
class NVIC : public PeriphTraits<periph> {
public:
    static void enableIRQ() {
        nvic_enable_irq(PeriphTraits<periph>::irq);
    }
    static void disableIRQ() {
        nvic_disable_irq(PeriphTraits<periph>::irq);
    }
};

template<> struct PeriphTraits<ADC1> { 
    const static rcc_periph_clken clock = RCC_ADC1;
};

template<> struct PeriphTraits<USART1> { 
    const static uint8_t irq = NVIC_USART1_IRQ; 
    const static rcc_periph_clken clock = RCC_USART1;
};

template<> struct PeriphTraits<USART2> { 
    const static rcc_periph_clken clock = RCC_USART2;
    const static uint8_t irq = NVIC_USART2_IRQ; 
};

template<> struct PeriphTraits<I2C1> { 
    const static uint8_t irq = NVIC_I2C1_IRQ; 
    const static rcc_periph_clken clock = RCC_I2C1;
};

template<> struct PeriphTraits<I2C2> { 
    const static uint8_t irq = NVIC_I2C2_IRQ; 
    const static rcc_periph_clken clock = RCC_I2C2;
};

template<> struct PeriphTraits<SPI1> { 
    const static uint8_t irq = NVIC_SPI1_IRQ; 
    const static rcc_periph_clken clock = RCC_SPI1;
};

template<> struct PeriphTraits<SPI2> { 
    const static uint8_t irq = NVIC_SPI2_IRQ; 
    const static rcc_periph_clken clock = RCC_SPI2;
};

template<> struct PeriphTraits<GPIOA> { 
    const static rcc_periph_clken clock = RCC_GPIOA;
};

template<> struct PeriphTraits<GPIOB> { 
    const static rcc_periph_clken clock = RCC_GPIOB;
};

template<> struct PeriphTraits<GPIOC> { 
    const static rcc_periph_clken clock = RCC_GPIOC;
};

template<> struct PeriphTraits<GPIOD> { 
    const static rcc_periph_clken clock = RCC_GPIOD;
};

#ifndef STM32L0
template<> struct PeriphTraits<GPIOF> { 
    const static rcc_periph_clken clock = RCC_GPIOF;
};
#endif
