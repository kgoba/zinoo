#include <ptlib/ptlib.h>
#include <ptlib/queue.h>

template<typename Base, int rx_queue_size = 128, int tx_queue_size = 32>
class BufferedSerial : public Base {
public:
    static void begin(int baudrate = 9600) {
        Base::begin(baudrate);
        Base::enableIRQ();
        Base::enableRXInterrupt();
    }

    static void onReceived() {
        char ch = Base::recv();
        rxQueue.push(ch);
    }

    static void onTransmitEmpty() {
        uint8_t ch;
        if (txQueue.pop(ch)) {
            Base::send(ch);
        }
        else {
            Base::disableTXInterrupt();
        }
    }

    static bool putc(uint8_t c) {
        if (txQueue.push(c)) {
            Base::enableTXInterrupt();
            return true;
        }
        return false;
    }

    static bool getc(uint8_t &c) {
        return rxQueue.pop(c);
    }

    static Queue<uint8_t, rx_queue_size> rxQueue;
    static Queue<uint8_t, tx_queue_size> txQueue;
};

template<typename Base, int rx_queue_size, int tx_queue_size>
Queue<uint8_t,  rx_queue_size> BufferedSerial<Base, rx_queue_size, tx_queue_size>::rxQueue;

template<typename Base, int rx_queue_size, int tx_queue_size>
Queue<uint8_t,  tx_queue_size> BufferedSerial<Base, rx_queue_size, tx_queue_size>::txQueue;

typedef BufferedSerial<USART<USART1, PB_6, PB_7>, 200, 64> SerialGPS;
// USART<USART1, PB_6, PB_7> usart_gps;

struct OutRawStreamBase {
    virtual int putc(int c) = 0;
};

struct InRawStreamBase {
    virtual int getc() = 0;
};

// struct OutTextStreamBase : public OutRawStreamBase {
//     virtual void puts(const char *str) {
//         while (*str) {
//             putc(*str++);
//         }
//     }
// };

// template<class Periph>
// class SimpleSerial : public OutRawStreamBase {
// public:
//     int putc(int c) override {
//         return Periph::send(c);
//     }
// };

// class BufferedSerial : public OutRawStreamBase {
// public:
//     int putc(int c) override {
//         if (txQueue.push(c)) 
//             return c;
//         else 
//             return -1;
//     }

//     void handleISR() {

//     }

// private:
//     Queue<char, 16> txQueue;
// };

// class BufferedOutStream {
//     int putc(int c) {
//     }
//     Queue<char, 16> outBuffer;
// };
