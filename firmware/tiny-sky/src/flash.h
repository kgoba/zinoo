#include "sx1276.h"

class SPIFlash_Base : public SPIDeviceBase {
public:
    bool initialize();

    void eraseAll();

protected:
    void readID(uint8_t *buf);
};