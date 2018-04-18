#pragma once

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/syscfg.h>

#include "rcc.h"

// #ifdef STM32L0
// #define ADC_RESOLUTION_12BIT	ADC_CFGR1_RES_12_BIT
// #define ADC_RESOLUTION_10BIT	ADC_CFGR1_RES_10_BIT
// #define ADC_RESOLUTION_8BIT		ADC_CFGR1_RES_8_BIT
// #define ADC_RESOLUTION_6BIT		ADC_CFGR1_RES_6_BIT
// #endif

#ifdef STM32L0
//#define ST_VREFINT_CAL			MMIO16(0x1FF80078)
#endif

class ADCBase {
public:
    enum conversion_mode_t {
        eSINGLE,
        eCONTINUOUS,
        eDISCONTINUOUS
    };

    enum scan_direction_t {
        eUPWARD,
        eBACKWARD
    };
};

template<uint32_t adc>
class ADC : public RCC<adc>, public NVIC<adc>, public ADCBase {
public:
    static void begin() {
        RCC<adc>::enableClock();

        adc_power_off(adc);
        //adc_set_clk_source(ADC1, ADC_CLKSOURCE_ADC);
        //adc_disable_analog_watchdog(ADC1);
        //adc_disable_external_trigger_regular(ADC1);
        adc_calibrate(adc);
        adc_set_right_aligned(adc);
        //adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_071DOT5);
        adc_set_resolution(adc, ADC_RESOLUTION_12BIT);
        //adc_enable_temperature_sensor();
        adc_power_on(adc);
    }
      
    static void shutdown() {
        adc_power_off(adc);
        RCC<adc>::disableClock();
        SYSCFG_CFGR3 &= ~SYSCFG_CFGR3_ENBUF_VREFINT_ADC;
    }

    static void setConversionMode(conversion_mode_t mode) {
        switch (mode) {
        case eDISCONTINUOUS: ADC_CFGR1(adc) &= ~ADC_CFGR1_CONT; ADC_CFGR1(adc) |= ADC_CFGR1_DISCEN; break;
        case eCONTINUOUS: ADC_CFGR1(adc) &= ~ADC_CFGR1_DISCEN; ADC_CFGR1(adc) |= ADC_CFGR1_CONT; break;
        case eSINGLE: ADC_CFGR1(adc) &= ~(ADC_CFGR1_DISCEN | ADC_CFGR1_CONT); break;
        }
    }

    static void setSampleTime(uint8_t time_factor) {
        ADC_SMPR(adc) = time_factor;
    }

    static void clearSequence(scan_direction_t direction = eUPWARD) {
        ADC_CHSELR(adc) = 0;

        switch (direction) {
        case eUPWARD: ADC_CFGR1(adc) &= ~ADC_CFGR1_SCANDIR; break;
        case eBACKWARD: ADC_CFGR1(adc) |= ADC_CFGR1_SCANDIR; break;
        }
    }

    static void addChannel(uint8_t channel) {
        ADC_CHSELR(adc) |= (1 << channel);
    }

    static void setChannel(uint8_t channel) {
        ADC_CHSELR(adc) = (1 << channel);
    }

    static uint32_t read() {
        return adc_read_regular(adc);
    }

    static void startConversion() {
        adc_start_conversion_regular(adc);
    }

    static bool isEOC() {
        return adc_eoc(adc);
    }

    static void enableVREFINT() {
        SYSCFG_CFGR3 |= SYSCFG_CFGR3_EN_VREFINT;
        // Wait until VREFINT becomes ready
        while (!(SYSCFG_CFGR3 & SYSCFG_CFGR3_VREFINT_RDYF)) { ; }        
        SYSCFG_CFGR3 |= SYSCFG_CFGR3_ENBUF_VREFINT_ADC;

        ADC_CCR(adc) |= ADC_CCR_VREFEN;
    }
};
