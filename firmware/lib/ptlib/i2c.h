#pragma once

#include <libopencm3/stm32/i2c.h>

#include "rcc.h"

uint32_t millis(void);

template<uint32_t i2c, typename Pin_sda, typename Pin_scl>
class I2C : public RCC<i2c> {
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
        uint32_t wait_start;

        //wait_start = millis();
        while (i2c_busy(i2c)) {
            ;
        }

        // Clear the NACK, STOP and BERR flags
        //I2C_ICR(i2c) = I2C_ICR_STOPCF | I2C_ICR_NACKCF; //  | I2C_ICR_BERRCF;

		i2c_set_7bit_address(i2c, address); // I2C_CR2_SADD   = address, CLEAR other fields
		i2c_set_read_transfer_dir(i2c);     // I2C_CR2_RD_WRN = 0
		i2c_set_bytes_to_transfer(i2c, 0);  // I2C_CR2_NBYTES = 0
        i2c_enable_autoend(i2c);            // I2C_CR2_AUTOEND = 1

        i2c_send_start(i2c);                // I2C_CR2_START   = 1

        // Wait for STOP, NACK or BERR
        wait_start = millis();
        while (true) {
            if (millis() - wait_start > 10) return -1;
            if ((I2C_ISR(i2c) & I2C_ISR_NACKF)) break;
            if ((I2C_ISR(i2c) & I2C_ISR_STOPF)) break;
            //if ((I2C_ISR(i2c) & I2C_ISR_BERR)) break;
            if (i2c_transmit_int_status(i2c)) break;
        }

        I2C_ICR(i2c) = I2C_ICR_STOPCF | I2C_ICR_NACKCF; //  | I2C_ICR_BERRCF;
        return 0;

        uint32_t nackf = (I2C_ISR(i2c) & I2C_ISR_NACKF);

        // Wait until STOP flag is reset
        // wait_start = millis();
        // while (true) {
        //     if (millis() - wait_start > 10) return -2;
        //     if (!(I2C_ISR(i2c) & I2C_ISR_STOPF)) break;
        // }
        
        // Clear the NACK, STOP and BERR flags
        I2C_ICR(i2c) = I2C_ICR_STOPCF | I2C_ICR_NACKCF; //  | I2C_ICR_BERRCF;

        // Check for BERR flag
        //if ((I2C_ISR(i2c) & I2C_ISR_BERR)) return -3;
        //if ((I2C_ISR(i2c) & I2C_ISR_NACKF)) return -4;

        //i2c_send_stop(i2c);

        return nackf;
    }
    
    static int write(int address, const uint8_t *data, int length, bool repeated=false) {
        //i2c_transfer7(i2c, address, (uint8_t *)data, length, 0, 0);
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
            uint32_t wait_start = millis();
			bool wait = true;
			while (wait) {
				if (i2c_transmit_int_status(i2c)) {
					wait = false;
				}
				while (i2c_nack(i2c)) {
                    if (millis() - wait_start > 100) return -1;
                }
			}
			i2c_send_data(i2c, *data++);
		}
        /* not entirely sure this is really necessary.
		 * RM implies it will stall until it can write out the later bits
		 */
		if (repeated) {
            uint32_t wait_start = millis();
			while (!i2c_transfer_complete(i2c)) {
                if (millis() - wait_start > 100) return -1;
            }
		}
        i2c_send_stop(i2c);
        return 0;   // success
    }
    
    static int read(int address, uint8_t *data, int length, bool repeated=false) {
        //i2c_transfer7(i2c, address, 0, 0, data, length);
		/* Setting transfer properties */
		i2c_set_7bit_address(i2c, address);
		i2c_set_read_transfer_dir(i2c);
		i2c_set_bytes_to_transfer(i2c, length);
		/* start transfer */
		i2c_send_start(i2c);
		/* important to do it afterwards to do a proper repeated start! */
		i2c_enable_autoend(i2c);

		for (size_t i = 0; i < length; i++) {
            uint32_t wait_start = millis();
			while (i2c_received_data(i2c) == 0) {
                if (millis() - wait_start > 100) return -1;
            }
			data[i] = i2c_get_data(i2c);
		}
        return 0;   // success
    }
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