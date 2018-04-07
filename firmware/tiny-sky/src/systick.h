#pragma once
#include <stdint.h>

typedef uint32_t systime_t;

void systick_setup(void);

void delay(uint32_t delay);
void delay_us(uint32_t delay);

systime_t millis(void);


typedef systime_t (*timer_routine_t) (systime_t time_called);

struct timer_task_t {
    systime_t       due_time;
    timer_routine_t routine;
    int             priority;
    timer_task_t    *next;
};

void add_task(timer_task_t *task, timer_routine_t routine, systime_t due_tie = 0, int priority = 0);

void schedule_tasks();
