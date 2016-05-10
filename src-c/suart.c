#include <avr/io.h>
#include <avr/interrupt.h>

#include "config.h"
#include "suart.h"

// Used to start a periodic timer
// Resets timer counter and enables overflow interrupt
#define TIMER_START     { TCNT0 = 0; bit_set(TIMSK0, TOIE0); }

// Stop timer (disable overflow interrupt)
#define TIMER_STOP      { bit_clear(TIMSK0, TOIE0); }

// Used to check RX pin state
#define RX_READ         bit_check(PINB, PB6)

static byte gBitIndex;
static byte gRxData;
static byte gRxError;

extern void suart_onReceive(byte data, byte isError);

void suart_setup()
{
    // Setup RX pin
    bit_set(PORTB, PB6);        // Enable pullup
    bit_set(PCMSK0, PCINT6);    // Enable pin change mask
    bit_set(PCICR, PCIE0);      // Enable pin change interrupt
}

void suart_onChange()
{
    byte state = RX_READ;
    if (state) {
      // Logical high
    }
    else {
        // Logical low
        if (gBitIndex == 0) {
            // Falling edge (start bit) received
            gRxData = 0;
            gRxError = 0;
            gBitIndex++;
            TIMER_START;
        }
    }
}

void suart_onTimer()
{
    byte state = RX_READ;
    if (gBitIndex == 0) {
      // Start bit (low) expected
      if (state) {
        gRxError = 1;
        gBitIndex = 0;
        TIMER_STOP;
      }
    }
    else if (gBitIndex >= 10) {
      // Expecting the stop bit (high)
      if (!state) {
        gRxError = 1;
      }
      gBitIndex = 0;
      TIMER_STOP;
      suart_onReceive(gRxData, gRxError);
    }
    else {
      // Start bit or data bits
      // Shift bit in 
      gRxData <<= 1;
      if (state) {
        // Logical high
        gRxData |= 1;
      }
      gBitIndex++;
    }
}

ISR(TIMER0_OVF_vect) {
    suart_onTimer();
}

ISR(PCINT0_vect) 
{
    suart_onChange();
}
