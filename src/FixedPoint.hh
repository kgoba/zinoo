#pragma once
#include <stdint.h>

template<typename T, typename T2, int shift>
class FixedPoint {
public:
  FixedPoint(T value = 0) : _value(value) {}
  //FixedPoint(float value) : _value((value + 0.5f) * (1 << shift)) {}

  FixedPoint<T, T2, shift> & operator *= (word rhs) {
    _value *= rhs;
    return *this;
  }

  template<int shift2>
  FixedPoint<T, T2, shift> & operator *= (FixedPoint<T, T2, shift2> const& rhs) {
    _value = (_value * rhs.getExtendedValue()) >> shift2;
    return *this;
  }

  template<int shift2>
  FixedPoint<T, T2, shift> & operator /= (FixedPoint<T, T2, shift2> const& rhs) {
    _value = (getExtendedValue() << shift2) / rhs.getValue();
    return *this;
  }

  FixedPoint<T, T2, shift> & operator += (FixedPoint<T, T2, shift> const& rhs) {
    _value += rhs.getValue();
    return *this;
  }

  T getValue() const { return _value; }
  T2 getExtendedValue() const { return _value; }

private:
  T _value;
};

template<typename T, typename T2, int shift, typename B>
FixedPoint<T, T2, shift> operator * (FixedPoint<T, T2, shift> a, const B &b)
{
  return a *= b;
}

template<typename T, typename T2, int shift, typename B>
FixedPoint<T, T2, shift> operator / (FixedPoint<T, T2, shift> a, const B &b)
{
  return a /= b;
}

template<typename T, typename T2, int shift>
FixedPoint<T, T2, shift> operator + (FixedPoint<T, T2, shift> a, const FixedPoint<T, T2, shift> &b)
{
  return a += b;
}


typedef FixedPoint<uint16_t, uint32_t, 0> U16F0;      // 0 .. 65536
typedef FixedPoint<int16_t, int32_t, 0> S15F0;        // -32768 .. 32767
typedef FixedPoint<int8_t, int16_t, 0> S7F0;          // -128 .. 127

typedef FixedPoint<uint16_t, uint32_t, 4> U12F4;      // 0 .. 4096.0
typedef FixedPoint<uint16_t, uint32_t, 16> U0F16;     // 0 .. 0.9999

typedef FixedPoint<int16_t, int32_t, 4> S11F4;        // -2048.0 .. 2047.9
typedef FixedPoint<int16_t, int32_t, 10> S5F10;        // -32.000 .. 31.999
typedef FixedPoint<int16_t, int32_t, 15> S0F15;       // -0.9999 .. 0.9999

typedef FixedPoint<int8_t, int16_t, 7> S0F7;          // -0.50 .. 0.49



template<uint8_t fractionBits>
void fixedPointToDecimal(int16_t fp, bool skipTrailingZeros = true) {
  // Determine maximum decimal digit value
  int16_t digitValue = (1000 << fractionBits);

  // Process integer part
  while (digitValue > (1 << fractionBits)) {
    uint8_t nextDigitValue = digitValue / 10;
    if (nextDigitValue < (1 << fractionBits)) {
      skipTrailingZeros = false;
    }
    uint8_t d = fp / digitValue;
    if (!skipTrailingZeros || d != 0) {
      // OUTPUT d + '0'
    }
    if (d != 0) {
      fp -= d * digitValue;
      skipTrailingZeros = false;
    }
    digitValue = nextDigitValue;
  }
  // Process fractional part
  // OUTPUT '.'
  while (digitValue > 0) {
    fp *= 10;
    uint8_t d = (fp >> fractionBits);
    // OUTPUT d + '0'
    fp -= (int16_t)d << fractionBits;
    digitValue /= 10;
  }
}
