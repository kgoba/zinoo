#include <i2c.hh>

#include <util/delay.h>

template<class I2CPeriph>
class BusBuzzer {
public:
  static bool setMode(uint8_t mode) {
    uint8_t cmd[2];
    cmd[0] = 0x00;
    cmd[1] = mode;
    return Device::send(cmd, 2);
  }

private:
  typedef I2CDevice<I2CPeriph, 0x23> Device;
};
