//#include ""
#include <PTLib.h>

template<int baudrate, class Pin>
class BitTransmitter {
public:
  void sendBit(byte bit) {
    if (bit) Pin::on();
    Pin::off();
  }
};

template<class BitStream>
class RTTYEncoder {
public:
  void sendChar(char c) {
    // Send start bit
    BitStream::sendBit(1);
    // Send data bits starting from LSB
    for (byte idx = 0; idx < 7; idx++) {
      BitStream::sendBit(c & 0x01);
      c >>= 1;
    }
    // Send stop bit(s)
    BitStream::sendBit(0);
    BitStream::sendBit(0);
  }

private:
  Queue<char, 32> txQueue;
};
