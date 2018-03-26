#pragma once

#include <libopencm3/stm32/spi.h>

#include "rcc.h"

template<uint32_t spi, typename Pin_mosi, typename Pin_miso, typename Pin_sclk>
class SPI : public RCC<spi> {
public:
    static void begin() {
        DigitalAF<Pin_mosi, spi>::begin(IODirection::OutputPP);
        DigitalAF<Pin_miso, spi>::begin(IODirection::Input);
        DigitalAF<Pin_sclk, spi>::begin(IODirection::OutputPP);
        
        // Clock source: PCLK(APB)
        RCC<spi>::enableClock();

        /* Set SPI to mode 0, master, MSB first, lowest baudrate */
#if defined(STM32F0)
        spi_init_master(spi, 
            SPI_CR1_BAUDRATE_FPCLK_DIV_256, 
            SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
            SPI_CR1_CPHA_CLK_TRANSITION_1, 
            SPI_CR1_CRCL_8BIT, 
            SPI_CR1_MSBFIRST);
#else
        spi_init_master(spi, 
            SPI_CR1_BAUDRATE_FPCLK_DIV_256, 
            SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
            SPI_CR1_CPHA_CLK_TRANSITION_1, 
            SPI_CR1_DFF_8BIT, 
            SPI_CR1_MSBFIRST);
#endif
        
        /* Configure NSS (slave select) behaviour */
        spi_disable_ss_output(spi);                // SSOE = 0
        spi_enable_software_slave_management(spi); // SSM = 1
        spi_set_nss_high(spi);                     // SSI = 1

#if defined(STM32F0)
        /* Switch to 8 bit frames (needed on F0 series) */
        spi_set_data_size(spi, SPI_CR2_DS_8BIT);
        spi_fifo_reception_threshold_8bit(spi);
#endif
            
        /* Enable SPI periph. */
        spi_enable(spi);
    }
    
    static void shutdown() {
        spi_disable(spi);
        RCC<spi>::disableClock();
    }
    
    static void format(uint8_t bits, uint8_t mode) {
        spi_set_standard_mode(spi, mode);
#if defined(STM32F0)
        spi_set_data_size(spi, ((uint16_t)bits - 1) << 8);
#endif
    }
    
    static uint32_t frequency(uint32_t hz) {
        if (rcc_apb1_frequency / 2 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_2);
            return rcc_apb1_frequency / 2;
        }
        if (rcc_apb1_frequency / 4 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_4);
            return rcc_apb1_frequency / 4;
        }
        if (rcc_apb1_frequency / 8 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_8);
            return rcc_apb1_frequency / 8;
        }
        if (rcc_apb1_frequency / 16 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_16);
            return rcc_apb1_frequency / 16;
        }
        if (rcc_apb1_frequency / 32 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_32);
            return rcc_apb1_frequency / 32;
        }
        if (rcc_apb1_frequency / 64 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_64);
            return rcc_apb1_frequency / 64;
        }
        if (rcc_apb1_frequency / 128 <= hz) {
            spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_128);
            return rcc_apb1_frequency / 128;
        }
        spi_set_baudrate_prescaler(spi, SPI_CR1_BR_FPCLK_DIV_256);
        return rcc_apb1_frequency / 256;
    }
    
    static uint16_t write16(uint16_t value) {
        return spi_xfer(spi, value);
    }

    static uint8_t write(uint8_t value) {
#if defined(STM32F0)
        spi_send8(spi, value);
        return spi_read8(spi);
#else
        return spi_xfer(spi, value);
#endif
    }
    
    static int write(const uint8_t *tx_buffer, int tx_length, uint8_t *rx_buffer, int rx_length) {
        int count = 0;
        while (tx_length > 0) {
            uint8_t response = write(*tx_buffer);
            tx_buffer++;
            tx_length--;
            if (rx_length > 0) {
                *rx_buffer = response;
                rx_buffer++;
                rx_length--;
                count++;
            }
        }
        return count;
    }

    static bool is_busy() {
	    return (SPI_SR(spi) & SPI_SR_BSY);
    }
};
