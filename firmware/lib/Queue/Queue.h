#pragma once

template <typename T, int capacity, typename Size=int> class Queue {
public:
  Queue() : _count(0), _head(0) {}

  void clear() {
    _count = 0;
    _head = 0;
  }

  bool push(T item) {
    if (_count < capacity) {
      Size tail = _head + _count;
      if (tail >= capacity) {
        tail -= capacity;
      }
      _buffer[tail] = item;
      _count++;
      return true;
    }
    return false;
  }

  T pop() {
    if (_count > 0) {
      T &item = _buffer[_head];
      _head++;
      if (_head == capacity) {
        _head = 0;
      }
      _count--;
      return item;
    }
    return T();
  }

  T at(Size rank) {
      if (rank > _count) return T();

      Size index = _head + rank;
      if (index >= capacity) {
        index -= capacity;
      }
      return _buffer[index];
  }

  Size count() { return _count; }

  bool empty() { return _count == 0; }
  bool full() { return _count == capacity; }

private:
  T     _buffer[capacity];
  Size   _count;
  Size   _head;
};