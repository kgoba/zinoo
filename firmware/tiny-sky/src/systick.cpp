#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "systick.h"

#define SYSTICK_HZ      1000

static volatile uint32_t system_millis;

// struct timer_t {
//     bool        periodic;
//     uint32_t    next_timeout;
//     timer_t     *left;
//     timer_t     *right;
// };

// static timer_t *timer_list;

void systick_setup(void)
{
    systick_set_frequency(SYSTICK_HZ, rcc_ahb_frequency);
    //systick_set_reload((rcc_ahb_frequency / SYSTICK_HZ) - 1);
    //systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
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

void delay_us(uint32_t delay) {
    uint32_t count = (delay * (rcc_ahb_frequency / 1000000U));
    while (count != 0)
    {
        count--;
    }
}

systime_t millis(void)
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

static timer_task_t *first_task;

static void insert_task(timer_task_t *task, systime_t due_time) {
    task->due_time = due_time;
    if (!first_task) {
        first_task = task;
        task->next = 0;
        return;
    }
    timer_task_t *task_iter = first_task;
    timer_task_t *task_iter_prev = 0;
    while (task_iter) {
        if (task_iter->due_time > task->due_time || 
            (task_iter->due_time == task->due_time && task_iter->priority <= task->priority)) 
        {
            task->next = task_iter;
            if (task_iter_prev) {
                task_iter_prev->next = task;
            }
            else {
                first_task = task;
            }
            return;
        }
        task_iter_prev = task_iter;
        task_iter = task_iter->next;
    }
    // We should add the new item at the end
    task_iter_prev->next = task;
    task->next = 0;
}

void add_task(timer_task_t *task, timer_routine_t routine, systime_t due_time, int priority) {
    task->routine = routine;
    task->priority = priority;
    insert_task(task, due_time);
}

void schedule_tasks() {
    if (!first_task) return;
    if (millis() >= first_task->due_time) {
        systime_t interval = first_task->routine(first_task->due_time);
        systime_t due_next = first_task->due_time + interval;

        timer_task_t *task = first_task;

        // Remove task from the queue
        first_task = first_task->next;
        task->next = 0;
        //if (first_task) {
            //first_task->prev = 0;
        //}

        if (interval > 0) {
            // Reschedule the task in the queue
            insert_task(task, due_next);
        }
    }
}
