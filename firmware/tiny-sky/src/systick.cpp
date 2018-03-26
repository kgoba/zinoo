#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "systick.h"

#define SYSTICK_HZ      1000

static volatile uint32_t system_millis;

void systick_setup(void)
{
    systick_set_reload(rcc_ahb_frequency / SYSTICK_HZ);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    //STK_CVR = 0;
    systick_interrupt_enable();
    systick_counter_enable();
    system_millis = 0;
}

void delay(uint32_t delay)
{
    uint32_t wake = system_millis + delay;
    while (wake > system_millis)
    {
        // do nothing
    }
}

uint32_t millis(void)
{
    return system_millis;
}

// ISR code
extern "C" {
    void sys_tick_handler(void)
    {
        system_millis++;
    }
}
