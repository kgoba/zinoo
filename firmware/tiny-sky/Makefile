OPENCM3_DIR     = ../lib/libopencm3

SRC         = $(wildcard src/*.c) $(wildcard drivers/*.c)
SRC_PP		= $(wildcard src/*.cpp) $(wildcard drivers/*.cpp)

OBJS		= $(patsubst %.c,%.o,$(SRC)) $(patsubst %.cpp,%.o,$(SRC_PP))

DEFINES     = -DLFS_NO_MALLOC -DLFS_NO_ASSERT -DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR

INCLUDE         += -I../lib -Idrivers -Isrc
Q = @

#include platform/stm32f103x8.mk
#include platform/stm32f030x8.mk
include platform/stm32l053x8.mk
include platform/libopencm3.mk
