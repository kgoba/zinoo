#include "flash.h"

#include <cstring>

#include "systick.h"

#define ADDRESS_LENGTH      3
#define ADDRESS_SHIFT       (4 - ADDRESS_LENGTH)

enum cmd_t {
    CMD_RDSR    = 0x05, // Read Status Register
    CMD_WRSR    = 0x01, // Write Status Register
    CMD_RDCR    = 0x35, // Read Configuration Register
    CMD_RSTEN   = 0x66, // Reset Enable
    CMD_RST     = 0x99, // Reset Memory
    CMD_READ    = 0x03, // Read Memory
    CMD_READF   = 0x0B, // Read Memory Fast
    CMD_SFDP    = 0x5A, // Serial Flash Discoverable Parameters
    CMD_WREN    = 0x06, // Write Enable
    CMD_WRDI    = 0x04, // Write Disable
    CMD_SE      = 0x20, // Sector Erase (4 kB)
    CMD_BE      = 0xD8, // Block Erase  (8/32/64 kB)
    CMD_CE      = 0xC7, // Chip Erase
    CMD_PP      = 0x02, // Page Program
    CMD_ID      = 0x9F, // Read JEDEC-ID
    CMD_WAKE    = 0xAB,
    CMD_ULBPR   = 0x98
};

bool SPIFlash_Base::initialize() {
    cmdWake();

    cmdReset();

    uint8_t tmp32[4];        // hex: 53 46 44 50 = 'SFDP'

    cmdSFDP(0x0000, tmp32, 4);
    if (memcmp(tmp32, "SFDP", 4) != 0) 
        return false;

    //cmdWriteStatusRegister(0, 0);

    cmdGlobalUnlock();
    return true;
}

uint32_t SPIFlash_Base::chipSize() {
    uint8_t tmp32[4];

    cmdSFDP(0x000C, tmp32, 4);
    uint32_t table_addr = tmp32[0] | (tmp32[1] << 8) | (tmp32[2] << 16);

    cmdSFDP(0x0004 + table_addr, tmp32, 4);
    uint32_t flash_top = (tmp32[0] | (tmp32[1] << 8) | (tmp32[2] << 16) | (tmp32[3] << 24));

    if (flash_top == 0) return 0;

    // Convert number of bits to bytes
    uint32_t flash_size = (flash_top + 1) / 8;

    // Check for overflow (512 MBytes = 2^32 bits)
    return (flash_size > 0) ? flash_size : (1 << 29);
}

uint32_t SPIFlash_Base::sectorSize() {
    uint8_t tmp32[4];

    cmdSFDP(0x000C, tmp32, 4);
    uint32_t table_addr = tmp32[0] | (tmp32[1] << 8) | (tmp32[2] << 16);

    cmdSFDP(0x0000 + table_addr, tmp32, 4);
    uint8_t sectorSizeType = (tmp32[0] & 0x03);

    // We only recognize type 1, 4096 bytes
    return (sectorSizeType == 1) ? 4096 : 0;
}

bool SPIFlash_Base::busy()
{
    uint8_t status;
    cmdReadStatusRegister(status);
    return (status & 1);
}

void SPIFlash_Base::sendCommand(uint8_t command, const uint8_t *wr_buf, int wr_len, uint8_t *rd_buf, int rd_len) {
    select();
    transfer(&command, 1, 0, 0);
    transfer(wr_buf, wr_len, rd_buf, rd_len);
    waitTCPH();
    release();
}

void SPIFlash_Base::cmdReset() {
    sendCommand(CMD_RSTEN);
    waitTCPH();
    sendCommand(CMD_RST);
}

void SPIFlash_Base::cmdWake() {
    sendCommand(CMD_WAKE);
}

void SPIFlash_Base::cmdGlobalUnlock() {
    sendCommand(CMD_WREN);
    waitTCPH();
    sendCommand(CMD_ULBPR);
    waitTCPH();
    sendCommand(CMD_WRDI);
}

void SPIFlash_Base::cmdSFDP(uint32_t address, uint8_t *rd_buf, int rd_len) {
    uint8_t wr_buf[] = {
        (uint8_t)(address >> 16),
        (uint8_t)(address >>  8),
        (uint8_t)(address >>  0),
        0
    };
    sendCommand(CMD_SFDP, wr_buf, 4, rd_buf, rd_len);
}

void SPIFlash_Base::read(uint32_t address, uint8_t *rd_buf, int rd_len) {
    uint8_t wr_buf[] = {
        (uint8_t)(address >> 24),
        (uint8_t)(address >> 16),
        (uint8_t)(address >>  8),
        (uint8_t)(address >>  0)
    };
    sendCommand(CMD_READ, wr_buf + ADDRESS_SHIFT, ADDRESS_LENGTH, rd_buf, rd_len);
}

void SPIFlash_Base::programPage(uint32_t address, const uint8_t *wr_buf, int wr_len) {
    uint8_t cmd_u8 = (uint8_t)CMD_PP;
    uint8_t addr_buf[] = {
        (uint8_t)(address >> 24),
        (uint8_t)(address >> 16),
        (uint8_t)(address >>  8),
        (uint8_t)(address >>  0)
    };

    sendCommand(CMD_WREN);
    waitTCPH();

    select();
    transfer(&cmd_u8, 1, 0, 0);
    transfer(addr_buf + ADDRESS_SHIFT, ADDRESS_LENGTH, 0, 0);
    transfer(wr_buf, wr_len, 0, 0);
    waitTCPH();
    release();
}

void SPIFlash_Base::eraseSector(uint32_t address) {
    uint8_t wr_buf[] = {
        (uint8_t)(address >> 24),
        (uint8_t)(address >> 16),
        (uint8_t)(address >>  8),
        (uint8_t)(address >>  0)
    };    
    sendCommand(CMD_WREN);
    waitTCPH();
    sendCommand(CMD_SE, wr_buf + ADDRESS_SHIFT, ADDRESS_LENGTH);
}

void SPIFlash_Base::eraseBlock(uint32_t address) {
    uint8_t wr_buf[] = {
        (uint8_t)(address >> 24),
        (uint8_t)(address >> 16),
        (uint8_t)(address >>  8),
        (uint8_t)(address >>  0)
    };

    sendCommand(CMD_WREN);
    waitTCPH();
    sendCommand(CMD_BE, wr_buf + ADDRESS_SHIFT, ADDRESS_LENGTH);
}

void SPIFlash_Base::eraseChip() {
    sendCommand(CMD_WREN);
    waitTCPH();
    sendCommand(CMD_CE);
}

void SPIFlash_Base::cmdReadConfigRegister(uint8_t &value) {
    sendCommand(CMD_RDCR, 0, 0, &value, 1);
}

void SPIFlash_Base::cmdReadStatusRegister(uint8_t &value) {
    sendCommand(CMD_RDSR, 0, 0, &value, 1);
}

void SPIFlash_Base::cmdWriteStatusRegister(uint8_t value1, uint8_t value2) {
    uint8_t wr_buf[] = {
        value1, 
        value2
    };

    sendCommand(CMD_WREN);
    waitTCPH();
    sendCommand(CMD_WRSR, wr_buf, 2);
}

void SPIFlash_Base::cmdReadID(uint8_t *rd_buf, int rd_len) {
    sendCommand(CMD_ID, 0, 0, rd_buf, rd_len);
}

void SPIFlash_Base::waitTCPH() {
    for (int i = 0; i < 10; i++) {
    }
}
