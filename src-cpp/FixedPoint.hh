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


typedef FixedPoint<uint16_t, uint32_t, 0> Q16_0;
typedef FixedPoint<uint16_t, uint32_t, 4> Q12_4;
typedef FixedPoint<uint16_t, uint32_t, 16> Q0_16;

typedef FixedPoint<int16_t, int32_t, 0> Q15_0;
typedef FixedPoint<int16_t, int32_t, 4> Q11_4;
typedef FixedPoint<int16_t, int32_t, 15> Q0_15;

typedef FixedPoint<int8_t, int16_t, 0> Q7_0;
typedef FixedPoint<int8_t, int16_t, 7> Q0_7;
