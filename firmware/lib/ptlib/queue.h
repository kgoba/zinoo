#pragma once

#include <stdint.h>

template<typename T, uint32_t capacity>
class Queue {
public:
  Queue() : _size(0), _head(0) {} 
  
  void clear() {
    _size = 0;
    _head = 0;
  }
  
  bool push(T item) {
    if (_size < capacity) {
      uint32_t tail = _head + _size;
      if (tail >= capacity) tail -= capacity;
      _buffer[tail] = item;
      _size++;      
      return true;
    }
    return false;
  }
  
  bool pop(T &item) {
    if (_size > 0) {
      item = _buffer[_head];
      _head++;
      if (_head == capacity) _head = 0;
      _size--;
      return true;
    }
    return false;
  }
  
  uint32_t size() {
    return _size;
  }

  bool empty() {
    return (_size == 0);
  }

  bool full() {
    return (_size == capacity);
  }
  
private:
  T         _buffer[capacity];
  uint32_t  _size;
  uint32_t  _head;
};

