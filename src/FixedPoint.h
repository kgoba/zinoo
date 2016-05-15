#include <stdint.h>

template<typename T, typename T2, int shift>
class FixedPoint {
public:
  FixedPoint(T value) : _value(value) {}

  FixedPoint<T, T2, shift> & operator *= (word rhs) {
    _value *= rhs;
    return *this;
  }

  FixedPoint<T, T2, shift> & operator *= (FixedPoint<T, T2, shift> const& rhs) {
    _value = (_value * (T2)rhs._value) >> shift;
    return *this;
  }

  FixedPoint<T, T2, shift> & operator += (FixedPoint<T, T2, shift> const& rhs) {
    _value += rhs._value;
    return *this;
  }
  
private:
  T _value;
};

template<typename T, typename T2, int shift, typename B>
FixedPoint<T, T2, shift> operator * (FixedPoint<T, T2, shift> a, const B &b) 
{
  return a *= b;
}

template<typename T, typename T2, int shift>
FixedPoint<T, T2, shift> operator + (FixedPoint<T, T2, shift> a, const FixedPoint<T, T2, shift> &b) 
{
  return a += b;
}


typedef FixedPoint<uint16_t, uint32_t, 0> Q16_0;
typedef FixedPoint<uint16_t, uint32_t, 4> Q12_4;
typedef FixedPoint<uint16_t, uint32_t, 0> Q0_16;

typedef FixedPoint<int16_t, int32_t, 0> Q15_0;
typedef FixedPoint<int16_t, int32_t, 4> Q11_4;
typedef FixedPoint<int16_t, int32_t, 0> Q0_15;

typedef FixedPoint<int8_t, int16_t, 7> Q0_7;
