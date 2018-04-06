#include "flash.h"

bool SPIFlash_Base::initialize() {
    //uint8_t id[3];
    //readID(id);

    

    // buf[0] = spi.write(0); // manufacturer ID
    // buf[1] = spi.write(0); // memory type
    // buf[2] = spi.write(0); // capacity
    // if (id[0] == ID0_SPANSION) {
    //     buf[3] = spi.write(0); // ID-CFI
    //     buf[4] = spi.write(0); // sector size
    // }
}

void SPIFlash_Base::readID(uint8_t *buf) {
    readBuf(0x9F, buf, 3);

    // uint32_t capacity = 1048576; // unknown chips, default to 1 MByte
    // if (id[2] >= 16 && id[2] <= 31) {
    //     capacity = 1ul << id[2];
    // }
    // else if (id[2] >= 32 && id[2] <= 37) {
    //     capacity = 1ul << (id[2] - 6);
    // }
}

void SPIFlash_Base::eraseAll() {
    // All other chips support the bulk erase command
    //writeBuf(0x06, 0, 0);   // write enable command
    //wait_us(1);
    //writeBuf(0xC7, 0, 0);   // bulk erase command    
}