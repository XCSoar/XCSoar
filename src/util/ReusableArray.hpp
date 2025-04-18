// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "util/AllocatedArray.hxx"

template<class T>
class ReusableArray {
public:
  typedef typename AllocatedArray<T>::size_type size_type;

protected:
  AllocatedArray<T> array;

public:
  ReusableArray() = default;

  constexpr
  ReusableArray(size_type _length):array(_length) {}

  ReusableArray(ReusableArray<T> &&other)
    :array(std::move(other.array)) {}

  ReusableArray<T> &operator=(ReusableArray<T> &&other) {
    array = std::move(other.array);
    return *this;
  }

  /**
   * Obtains an array.  Its values are undefined.
   */
  T *get(size_type _length) {
    array.GrowDiscard(_length);
    return array.data();
  }

  /**
   * Grows an existing array, preserving data.
   */
  T *grow(size_type old_length, size_type new_length) {
    array.grow_preserve(new_length, old_length);
    return array.data();
  }
};
