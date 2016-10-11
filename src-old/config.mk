PROGRAMMER = arduino
PROG_OPTS = -P /dev/tty.usbserial-A4004R3X
F_CPU   = 16000000UL
#MCU     = atmega328p
MCU     = atmega32u4

CXX = avr-g++
LD  = avr-g++
OBJDUMP = avr-objdump
OBJCOPY = avr-objcopy
SIZE    = avr-size
AR	= avr-ar

