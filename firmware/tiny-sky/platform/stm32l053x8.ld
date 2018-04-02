MEMORY
{
  rom    (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  ram    (rwx) : ORIGIN = 0x20000000, LENGTH =  8K
  eeprom (rx)  : ORIGIN = 0x08080000, LENGTH =  2K
}
 
INCLUDE libopencm3_stm32l0.ld
