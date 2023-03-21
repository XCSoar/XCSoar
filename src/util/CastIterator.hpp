// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

template<typename VT, typename IT>
class CastIterator {
  IT iterator;

public:
  typedef typename IT::iterator_category iterator_category;
  typedef typename IT::difference_type difference_type;
  typedef VT value_type;
  typedef VT *pointer;
  typedef VT &reference;

  CastIterator() = default;

  constexpr
  CastIterator(const IT _iterator):iterator(_iterator) {}

  reference operator*() const {
    return static_cast<reference>(*iterator);
  }

  pointer operator->() const {
    return static_cast<pointer>(iterator.IT::operator->());
  }

  CastIterator<VT,IT> &operator++() {
    iterator = ++iterator;
    return *this;
  }

  CastIterator<VT,IT> operator++(int) {
    CastIterator<VT,IT> old = *this;
    iterator++;
    return old;
  }

  CastIterator<VT,IT> &operator--() {
    iterator = --iterator;
    return *this;
  }

  CastIterator<VT,IT> operator--(int) {
    CastIterator<VT,IT> old = *this;
    iterator--;
    return old;
  }

  bool operator==(const CastIterator<VT,IT> &other) const {
    return iterator == other.iterator;
  }

  bool operator!=(const CastIterator<VT,IT> &other) const {
    return iterator != other.iterator;
  }
};
