#pragma once

#include <libopencm3/stm32/i2c.h>

#include "rcc.h"

uint32_t millis(void);

class I2CBase {
public:
    virtual bool read(int address, uint8_t *data, int length) = 0;
    virtual bool write(int address, const uint8_t *data, int length) = 0;
    virtual bool readwrite(int address, const uint8_t *data_w, int length_w, uint8_t *data_r, int length_r) = 0;
};

template<uint32_t i2c, typename Pin_sda, typename Pin_scl>
class I2C : public RCC<i2c>, public NVIC<i2c>, public I2CBase {
public:
    static void begin() {
        // Setup clock source:
        // - I2C1: HSI@8Mhz (default) / SYSCLK
        // - I2C2: PCLK(APB)
        //rcc_set_i2c_clock_hsi(i2c);
        //RCC_CCIPR |= (RCC_CCIPR_I2C1SEL_APB << RCC_CCIPR_I2C1SEL_SHIFT);

        RCC<i2c>::enableClock();

        DigitalAF<Pin_sda, i2c>::begin(IODirection::OutputOD);
        DigitalAF<Pin_scl, i2c>::begin(IODirection::OutputOD);

        i2c_reset(i2c);
        i2c_peripheral_disable(i2c);

        i2c_enable_analog_filter(i2c);    //configure ANFOFF DNF[3:0] in CR1
        i2c_set_digital_filter(i2c, 0);

        normal_speed(); /* Configure normal speed (100 kHz) */

#if defined(STM32F1)
#else
        i2c_set_7bit_addr_mode(i2c);
#endif  

        /* Enable I2C periph. */
        i2c_peripheral_enable(i2c);
    }
      
    static void shutdown() {
        i2c_peripheral_disable(i2c);
        RCC<i2c>::disableClock();
    }

    static void enableSTOPInterrupt() {
        i2c_enable_interrupt(i2c, I2C_CR1_STOPIE);
    }

    static void disableSTOPInterrupt() {
        i2c_disable_interrupt(i2c, I2C_CR1_STOPIE);
    }

    static void enableTCInterrupt() {
        i2c_enable_interrupt(i2c, I2C_CR1_TCIE);
    }

    static void disableTCInterrupt() {
        i2c_disable_interrupt(i2c, I2C_CR1_TCIE);
    }

    static void enableTXInterrupt() {
        i2c_enable_interrupt(i2c, I2C_CR1_TXIE);
    }

    static void disableTXInterrupt() {
        i2c_disable_interrupt(i2c, I2C_CR1_TXIE);
    }

    static void enableRXInterrupt() {
        i2c_enable_interrupt(i2c, I2C_CR1_RXIE);
    }

    static void disableRXInterrupt() {
        i2c_disable_interrupt(i2c, I2C_CR1_RXIE);
    }
    
    static void enableNACKInterrupt() {
        i2c_enable_interrupt(i2c, I2C_CR1_NACKIE);
    }

    static void disableNACKInterrupt() {
        i2c_disable_interrupt(i2c, I2C_CR1_NACKIE);
    }

    static void normal_speed() {
#if defined(STM32F1)
#elif defined(STM32L0) 
        i2c_set_speed(i2c, i2c_speed_sm_100k, rcc_apb1_frequency / 1000000UL);
#else
        // Setup 100 kHz rate @ 8 MHz clock
        i2c_set_prescaler(i2c, 1);
        i2c_set_data_setup_time(i2c, 4);   // 1.25 us
        i2c_set_data_hold_time(i2c, 2);    // 0.5 us
        i2c_set_scl_high_period(i2c, 16);  // 4.0 us
        i2c_set_scl_low_period(i2c, 20);   // 5.0 us
#endif
    }

    static int check(uint16_t address) {
        // Clear the STOP, NACK (and BERR?) flags
        I2C_ICR(i2c) = I2C_ICR_NACKCF | I2C_ICR_STOPCF;

		i2c_set_7bit_address(i2c, address); // I2C_CR2_SADD   = address, CLEAR other fields
		//i2c_set_read_transfer_dir(i2c);     // I2C_CR2_RD_WRN = 1
        i2c_set_write_transfer_dir(i2c);     // I2C_CR2_RD_WRN = 0
		i2c_set_bytes_to_transfer(i2c, 0);  // I2C_CR2_NBYTES = 0
        i2c_enable_autoend(i2c);            // I2C_CR2_AUTOEND = 1

        i2c_send_start(i2c);                // I2C_CR2_START   = 1

        // Wait for STOP (no need to check TXIS)
        while (!(I2C_ISR(i2c) & I2C_ISR_STOPF)) {}

        // Check for error (indicated by NACK)
        if (I2C_ISR(i2c) & I2C_ISR_NACKF) {
            return -1;
        }

        return 0;
    }
    
    static int write2(int address, const uint8_t *data, int length, bool repeated=false) {
        //i2c_transfer7(i2c, address, (uint8_t *)data, length, 0, 0);

        I2C_ICR(i2c) = I2C_ICR_NACKCF | I2C_ICR_STOPCF;

		i2c_set_7bit_address(i2c, address);
		i2c_set_write_transfer_dir(i2c);
		i2c_set_bytes_to_transfer(i2c, length);
		if (repeated) {
			i2c_disable_autoend(i2c);
		} else {
			i2c_enable_autoend(i2c);
		}
		i2c_send_start(i2c);

		while (length--) {
            while (!i2c_transmit_int_status(i2c)) {
                if (i2c_nack(i2c)) {
                    if (repeated) {
                        i2c_send_stop(i2c);
                    }
                    return -1;
                }
            }
			i2c_send_data(i2c, *data++);
		}
		if (repeated) {
            // AUTOEND is disabled, so wait for the TC flag
			while (!i2c_transfer_complete(i2c)) {}
		}
        else {
            //i2c_send_stop(i2c);
        }
        return 0;   // success
    }
    
    static int read2(int address, uint8_t *data, int length, bool repeated=false) {
        //i2c_transfer7(i2c, address, 0, 0, data, length);

        I2C_ICR(i2c) = I2C_ICR_NACKCF | I2C_ICR_STOPCF;

		/* Setting transfer properties */
		i2c_set_7bit_address(i2c, address);
		i2c_set_read_transfer_dir(i2c);
		i2c_set_bytes_to_transfer(i2c, length);
		/* start transfer */
		i2c_send_start(i2c);
		i2c_enable_autoend(i2c);

		for (size_t i = 0; i < length; i++) {
			while (i2c_received_data(i2c) == 0) {}
			data[i] = i2c_get_data(i2c);
		}
        return 0;   // success
    }

    virtual bool read(int address, uint8_t *data, int length) {
        return 0 == read2(address, data, length, false);
        //i2c_transfer7(i2c, address, 0, 0, data, length);
        //return true;
    }
    virtual bool write(int address, const uint8_t *data, int length) {
        return 0 == write2(address, data, length, false);
        //i2c_transfer7(i2c, address, (uint8_t *)data, length, 0, 0);
        //return true;
    }
    virtual bool readwrite(int address, const uint8_t *data_w, int length_w, uint8_t *data_r, int length_r) {
        if (0 == write2(address, data_w, length_w, true)) {
            return 0 == read2(address, data_r, length_r, false);
        }
        return false;
        //i2c_transfer7(i2c, address, (uint8_t *)data_w, length_w, data_r, length_r);
        //return true;
    }
};

class I2CDeviceBase {
public:
    I2CDeviceBase(I2CBase &bus, uint8_t slave_address) : bus(bus), slave_address(slave_address) {}

    bool readRegister(uint8_t address, uint8_t &value) {
        //if (!bus.write(slave_address, &address, 1)) return false;
        //if (!bus.read(slave_address, &value, 1)) return false;
        if (!bus.readwrite(slave_address, &address, 1, &value, 1)) return false;
        return true;
    }

    bool readRegister(uint8_t address, uint8_t *value, uint8_t length) {
        //if (!bus.write(slave_address, &address, 1)) return false;
        //if (!bus.read(slave_address, value, length)) return false;
        if (!bus.readwrite(slave_address, &address, 1, value, length)) return false;
        return true;
    }

    bool writeRegister(uint8_t address, uint8_t value) {
        uint8_t data[] = { address, value };
        if (!bus.write(slave_address, data, 2)) return false;
        return true;
    }

protected:
    I2CBase &bus;
    uint8_t slave_address;
};

/*
template<typename I2C, int address>
class I2CDeviceBase : public I2C {
    static int write(const uint8_t *data, int length, bool repeated=false) {
        return I2C::write(address, data, length, repeated);
    }
    
    static void read(uint8_t *data, int length, bool repeated=false) {
        return I2C::read(address, data, length, repeated);
    }
};

template<typename I2C, int address>
class I2CDevice : public I2CDeviceBase<I2C, address> {
    static void writeRegister(uint8_t reg, uint8_t value) {
        I2CDeviceBase<I2C, address>::write()
    }
    static void writeRegister16(uint8_t reg, uint16_t value) {
        i2c_transfer7(i2c, address, (uint8_t *)data, length, 0, 0);
    }
    static void writeRegister(uint8_t reg, const uint8_t *data, int length) {
        i2c_transfer7(i2c, address, (uint8_t *)data, length, 0, 0);
    }
    
    static void readRegister(uint8_t reg, uint8_t &data, int length) {
        i2c_transfer7(i2c, address, 0, 0, data, length);
    }
    static void readRegister(uint8_t reg, uint8_t *data, int length) {
        i2c_transfer7(i2c, address, 0, 0, data, length);
    }
};
*/