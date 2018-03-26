##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2014 Frantisek Burian <BuFran@seznam.cz>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

###############################################################################
# The support makefile for GCC compiler toolchain, the rules part.
#
# please read mk/README for specification how to use this file in your project
#

%.bin: %.elf
	@printf "  OBJCOPY $@\n"
	$(Q)$(OBJCOPY) -Obinary $< $@

%.hex: %.elf
	@printf "  OBJCOPY $@\n"
	$(Q)$(OBJCOPY) -Oihex $< $@

%.srec: %.elf
	@printf "  OBJCOPY $@\n"
	$(Q)$(OBJCOPY) -Osrec $< $@

%.list: %.elf
	@printf "  OBJDUMP $@\n"
	$(Q)$(OBJDUMP) -S $< > $@

$(OBJDIR)/%.elf: $(OBJS2) $(LDSCRIPT) $(LIBDEPS)
	@printf "  LD      $(*).elf\n"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(LD) $(OBJS2) $(LDLIBS) $(LDFLAGS) -T$(LDSCRIPT) $(ARCH_FLAGS)  -o $@

$(OBJDIR)/%.o: %.c
	@printf "  CC      $<\n"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) $(ARCH_FLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cxx
	@printf "  CXX     $<\n"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(ARCH_FLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cpp
	@printf "  CXX     $(*).cpp\n"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(ARCH_FLAGS) -o $@ -c $<
