TARGET ?= firmware
OBJDIR ?= .build

DEFINES         += -D$(FAMILY)
INCLUDE         += -I$(OPENCM3_DIR)/include/

CFLAGS          += -Os -ggdb3 $(ARCH) -ffunction-sections -fdata-sections
CXXFLAGS        += -Os -ggdb3 $(ARCH) -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections
CPPFLAGS        += -MD -MP $(DEFINES) $(INCLUDE)

LDFLAGS         += -static -nostartfiles $(ARCH)
LDFLAGS         += -Wl,--start-group -Wl,--end-group -Wl,--gc-sections
LDFLAGS         += -Wl,-Map=$(OBJDIR)/$(TARGET).map,--cref
LDFLAGS         += -lgcc -lc --specs=nosys.specs --specs=nano.specs

LDLIBS          += -L$(OPENCM3_DIR)/lib
LDLIBS          += -l$(OPENCM3_LIB)

OBJS2 = $(addprefix $(OBJDIR)/, $(OBJS))

#include $(OPENCM3_DIR)/mk/genlink-config.mk
include platform/gcc-config.mk

.PHONY: directories clean all
.PRECIOUS: $(OBJS2)

all: directories $(OBJDIR)/$(TARGET).elf $(OBJDIR)/$(TARGET).bin $(OBJDIR)/$(TARGET).list
	@printf "  SIZE     $(TARGET).elf\n"
	$(Q)$(SIZE) $(OBJDIR)/$(TARGET).elf 

directories:
	$(Q)mkdir -p $(OBJDIR)

clean:
	$(Q)$(RM) -rf $(OBJDIR)

flash: $(OBJDIR)/$(TARGET).bin
	st-flash --reset write $< 0x8000000

#include $(OPENCM3_DIR)/mk/genlink-rules.mk
include platform/gcc-rules.mk

-include $(OBJS2:%.o=%.d)
