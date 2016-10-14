#include "debug.hh"

//#include "i2c.hh"

MinimumSerial dbg;
//SerialStream dbg;

/*
void I2CDetect() {
  dbg.println("Detecting I2C devices");
    for (byte addr = 8; addr < 126; addr++) {
        byte data = 0;
        TWIMaster::write(addr, &data, 1);

        bool success = false;

        for (byte nTry = 0; nTry < 10; nTry++) {
          _delay_ms(5);
          if (TWIMaster::isReady()) {
              success = !TWIMaster::isError();
              break;
          }
        }

        if (success) {
          dbg.print(addr, HEX);
          dbg.println();
        }

    _delay_ms(1);
    }
}
*/
