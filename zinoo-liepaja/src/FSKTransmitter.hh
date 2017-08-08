#pragma once
#include "Arduino.h"

#include "pins.hh"

template<class PinTXMod, class PinTXEnable>
class FSKTransmitter {
public:
  FSKTransmitter();

  void enable();
  void disable();
  void transmit(const uint8_t *buffer, uint16_t length);
  void tick();

  void mark();
  void space();

  bool isBusy();

private:
  PinTXMod    pinTXMod;
  PinTXEnable pinTXEnable;

  const uint8_t * txBuffer;
  uint16_t  txLength;
  uint8_t   bitIndex;
  uint8_t   txShift;
  bool      autoShutdown;
  bool      active;

  void shiftNew();
};

template<class PinTXMod, class PinTXEnable>
FSKTransmitter<PinTXMod, PinTXEnable>::FSKTransmitter() {
  pinTXEnable.clear();
  pinTXMod.clear();

  autoShutdown = false;
  txLength = 0;
  bitIndex = 0;
  active = false;
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::mark() {
  pinTXMod.set();
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::space() {
  pinTXMod.clear();
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::enable() {
  pinTXEnable.set();
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::disable() {
  pinTXEnable.clear();
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::transmit(const uint8_t *buffer, uint16_t length) {
  if (active) return;
  txBuffer = buffer;
  txLength = length;
  if (txLength > 0) {
    active = true;
    enable();
    shiftNew();
  }
}

template<class PinTXMod, class PinTXEnable>
bool FSKTransmitter<PinTXMod, PinTXEnable>::isBusy() {
  return active;
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::tick() {
  if (!active) return;

  if (bitIndex == 10) {
    if (txLength > 0)
      shiftNew();
    else {
      if (autoShutdown) {
        disable();
      }
      else {
        mark();
      }
      active = false;
      return;
    }
  }
  if (bitIndex == 0) {
    space();
  }
  else if (bitIndex >= 8) {
    mark();
  }
  else {
    if (txShift & 1) mark();
    else space();

    txShift >>= 1;
  }
  bitIndex++;
}

template<class PinTXMod, class PinTXEnable>
void FSKTransmitter<PinTXMod, PinTXEnable>::shiftNew() {
  txShift = *txBuffer++;
  txLength--;
  bitIndex = 0;
}
