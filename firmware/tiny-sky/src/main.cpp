#include <cstring>
#include <cstdio>
#include <cstdarg>

#include "systick.h"
#include "strconv.h"
#include "gps.h"
#include "ublox.h"

#include "console.h"
#include "settings.h"

#include <ptlib/ptlib.h>
#include <ptlib/queue.h>

void rcc_setup_hsi16_out_16mhz() {
    // default clock after reset = MSI 2097 kHz
    rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
    rcc_set_ppre1(RCC_CFGR_PPRE1_NODIV);
    rcc_set_ppre2(RCC_CFGR_PPRE2_NODIV);

    rcc_osc_on(RCC_HSI16);
    rcc_wait_for_osc_ready(RCC_HSI16);
    rcc_set_sysclk_source(RCC_HSI16);

    rcc_ahb_frequency = 16 * 1000000UL;
    rcc_apb1_frequency = 16 * 1000000UL;
    rcc_apb2_frequency = 16 * 1000000UL;
}

void setup() {
    // Initialize MCU clocks
    rcc_setup_hsi16_out_16mhz();

    rcc_periph_clock_enable(RCC_SYSCFG);
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_MIF);

    systick_setup();    // enable millisecond counting

    // Initialize internal peripherals
    gState.init_hw();

    delay(100);         // seems to be necessary for the following delays to work properly

    // Initialize external peripherals
    gState.init_periph();
}

systime_t task_gps_func(systime_t due_time) {
    return gState.task_gps(due_time);
}

systime_t task_console_func(systime_t due_time) {
    return gState.task_console(due_time);
}

systime_t task_report_func(systime_t due_time) {
    return gState.task_report(due_time);
}

systime_t task_led_func(systime_t due_time) {
    return gState.task_led(due_time);
}

systime_t task_buzz_func(systime_t due_time) {
    return gState.task_buzz(due_time);
}

systime_t task_sensors_func(systime_t due_time) {
    return gState.task_sensors(due_time);
}

systime_t task_control_func(systime_t due_time) {
    return gState.task_control(due_time);
}

timer_task_t timer_tasks[7];

int main() {
    setup();

    add_task(&timer_tasks[0], task_led_func, 0, -3);
    add_task(&timer_tasks[1], task_console_func, 0, 4);
    add_task(&timer_tasks[2], task_gps_func, 0, 0);
    add_task(&timer_tasks[3], task_sensors_func, 0, 3);
    add_task(&timer_tasks[4], task_report_func, 0, -4);
    add_task(&timer_tasks[5], task_buzz_func, 0, -3);
    add_task(&timer_tasks[6], task_control_func, 0, 1);

    while (1) {
        schedule_tasks();
        __asm("WFI");
    }
}

// TODO: 
// - check arm/safe status
// - pyro activation, pyro safe timer
// - buzzer/LED indication (ARM state)
// - simple SPI flash logging
// - log readout (CSV generation?)
// - acc/gyro readout
// - timer-based sensor polling
// - console parameter get/set
// - battery/pyro voltage sensing
// + mag calibration, EEPROM/flash save
// + UKHAS packet forming, regular TX

typedef struct {
    uint16_t    address;    // 7/10 bit address
    uint8_t     type;       // read/write, stop/continue
    uint8_t     status;     // busy/complete/error
    uint8_t     *rx_buf;
    uint8_t     *tx_buf;
    uint16_t    rx_len;
    uint16_t    tx_len;
} i2c_request_t;

extern "C" {
    // void i2c1_isr(void) {
    //     constexpr uint32_t periph = I2C1;
    //     uint32_t flags = I2C_ISR(periph);
    //     if (flags & I2C_ISR_RXNE) {
    //         // read IC2_RXDR to clear the flag
    //         uint8_t data = I2C_RXDR(periph);
    //     }
    //     if (flags & I2C_ISR_TXIS) {
    //         // write I2C_TXDR to clear the flag
    //         uint8_t data;
    //         I2C_TXDR(periph) = data;
    //     }
    //     if (flags & I2C_ISR_TC) {
    //         // write START or STOP
    //     }
    //     if (flags & I2C_ISR_STOPF) {
    //         I2C_ICR(periph) = I2C_ICR_STOPCF;
    //     }
    //     if (flags & I2C_ISR_NACKF) {
    //         I2C_ICR(periph) = I2C_ICR_NACKCF;
    //     }
    // }
    // void tim2_isr(void);
    // void tim21_isr(void);
    // void tim22_isr(void);    
    // void tim6_dac_isr(void);    // BASIC timer
    // void lptim1_isr(void);      // Low Power timer
}
