PROGRAMMER = arduino
PROG_OPTS = -P /dev/ttyUSB0
F_CPU   = 16000000UL
MCU     = atmega328p

CXX = avr-g++
LD  = avr-g++
OBJDUMP = avr-objdump
OBJCOPY = avr-objcopy
SIZE    = avr-size
AR	= avr-ar

