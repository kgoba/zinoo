#pragma once

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

typedef uint8_t byte;
typedef uint16_t word;


#define bit_set(byte, bit)          ((byte) |= (1<<(bit)))
#define bit_clear(byte, bit)        ((byte) &= ~(1<<(bit)))
#define bit_toggle(byte, bit)       ((byte) ^= (1<<(bit)))
#define bit_check(byte, bit)        ((byte) & (1<<(bit)))


#define ARRAY_SIZE(array)     (sizeof(array) / sizeof(array[0]))


#define ENTER_CRITICAL   byte tmpSREG = SREG; cli();
#define EXIT_CRITICAL    SREG = tmpSREG;


#define FIFO_DEFINE(Name, Size, Type) \
  struct {          \
    byte count;     \
    byte head;      \
    Type data[Size];\
  } Name;
  
#define FIFO_SIZE(Name)       ARRAY_SIZE(Name.data)

#define FIFO_PUT(Name, Value) \
  if (Name.count < FIFO_SIZE(Name)) {           \
    byte tail = Name.head + Name.count;         \
    if (tail > FIFO_SIZE(Name)) tail -= FIFO_SIZE(Name);      \
    Name.data[tail] = Value;      \
    Name.count++;                 \
  }

