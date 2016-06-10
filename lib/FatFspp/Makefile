F_CPU   = 16000000UL
MCU     = atmega328p

CC  = avr-gcc
CXX = avr-g++
LD  = avr-g++
OBJDUMP = avr-objdump
OBJCOPY = avr-objcopy
SIZE    = avr-size
AR	= avr-ar

TARGET 	 = libfatfspp
#SRCPP 	 = src/FatFsCard.cpp src/FatFs.cpp
SRC	 = src/diskio.c src/ff.c
# SRC = $(wildcard *.cc) $(EXTSRC)


CXXFLAGS += -g -mmcu=$(MCU) -Os 
CXXFLAGS += -DF_CPU=$(F_CPU)
CXXFLAGS += -fdata-sections -ffunction-sections 
CXXFLAGS += -funsigned-char -funsigned-bitfields -fshort-enums -fno-inline-small-functions 
CXXFLAGS += -fpack-struct
CFLAGS = $(CXXFLAGS)
INCLUDES += -I../../include
LFLAGS   += -g -mmcu=$(MCU) -Wl,--relax,--gc-sections
LIBS 	 += 

OBJPP = $(SRCPP:.cpp=.o)
OBJ = $(SRC:.c=.o)

.PHONY: all clean fuse program

all: $(TARGET).a

clean:
	@echo "  Cleaning all"
	@rm -f $(TARGET).hex $(TARGET).lst $(TARGET).elf $(OBJ)

$(TARGET).a: $(OBJ) $(LIBS)
	@echo "  Linking..."
	@$(AR) rcs $@ $(OBJ) $(LIBS)
	@$(SIZE) $@

.cc.o:
	@echo "  Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

.c.o:
	@echo "  Compiling $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

fuse:
	avrdude -p $(MCU) -c $(PROGRAMMER) $(PROG_OPTS) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(XFUSE):m

program: $(TARGET).hex
	avrdude -p $(MCU) -c $(PROGRAMMER) $(PROG_OPTS) -U flash:w:$(TARGET).hex

