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
typedef FixedPoint<int16_t, int32_t, 15> S0F15;       // -0.9999 .. 0.9999

typedef FixedPoint<int8_t, int16_t, 7> S0F7;          // -0.50 .. 0.49
