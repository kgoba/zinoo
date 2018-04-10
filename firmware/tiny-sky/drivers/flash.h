//#include "sx1276.h"

#include <stdint.h>

class SPIFlash_Base { // : public SPIDeviceBase 
public:
    bool initialize();

    uint32_t chipSize();
    uint32_t sectorSize();

    bool busy();
    void cmdReadStatusRegister(uint8_t &value);

    void read(uint32_t address, uint8_t *rd_buf, int rd_len);
    void programPage(uint32_t address, const uint8_t *wr_buf, int wr_len);

    void eraseSector(uint32_t address);
    void eraseBlock(uint32_t address);
    void eraseChip();

protected:
    void sendCommand(uint8_t command, const uint8_t *wr_buf = 0, int wr_len = 0, uint8_t *rd_buf = 0, int rd_len = 0);
    void waitTCPH();

    void cmdReset();
    void cmdWake();
    void cmdGlobalUnlock();
    void cmdReadID(uint8_t *rd_buf, int rd_len = 3);
    void cmdSFDP(uint32_t address, uint8_t *rd_buf, int rd_len);
    void cmdReadConfigRegister(uint8_t &value);
    void cmdWriteStatusRegister(uint8_t value1, uint8_t value2 = 0);

protected:
    virtual void select() = 0;
    virtual void release() = 0;
    virtual void transfer(const uint8_t *wr_buf, int wr_len, uint8_t *rd_buf, int rd_len) = 0;
};
