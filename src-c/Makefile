F_CPU 	= 16000000
MCU 	= atmega328p
LFUSE 	= 0xE2
HFUSE 	= 0xDD
XFUSE 	= 0xFF

PROG_TYPE = usbasp

CC = avr-gcc
LD = avr-gcc
OBJDUMP = avr-objdump
OBJCOPY = avr-objcopy
OBJSIZE = avr-size

TARGET 	= zinoo5

INCLUDES = 
LIBS 	 = 

CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
CFLAGS += -ffunction-sections -fdata-sections
LFLAGS += -Wl,--relax,--gc-sections

CFLAGS += -g -mmcu=$(MCU) -Os -DF_CPU=$(F_CPU)
LFLAGS += -g -mmcu=$(MCU)

OBJDIR 	= obj
SRC 	= $(wildcard *.c)
OBJ 	= $(addprefix $(OBJDIR)/,$(SRC:.c=.o))


.PHONY: all fuse program clean

all: $(TARGET).hex $(TARGET).lst


$(TARGET).hex: $(TARGET).elf
	@$(OBJCOPY) -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex


$(TARGET).lst: $(TARGET).elf
	@$(OBJDUMP) -h -S $(TARGET).elf >$(TARGET).lst


$(TARGET).elf: $(OBJ)
	@echo "Linking..."
	@$(LD) -o $@ $^ $(LFLAGS) $(LIBS)
	$(OBJSIZE) $@


$(OBJDIR)/%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


fuse:
	avrdude -p $(MCU) -c $(PROG_TYPE) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(XFUSE):m


program: $(TARGET).hex
	avrdude -p $(MCU) -c $(PROG_TYPE) -U flash:w:$(TARGET).hex
	

clean:
	@rm -r $(OBJDIR) $(TARGET).hex $(TARGET).lst $(TARGET).elf
