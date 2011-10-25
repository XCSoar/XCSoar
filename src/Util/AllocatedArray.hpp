/*
 * Copyright (C) 2010 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_ALLOCATED_ARRAY_HPP
#define XCSOAR_ALLOCATED_ARRAY_HPP

#include "Compiler.h"

#include <assert.h>
#include <algorithm>

/**
 * An array allocated on the heap with a length determined at runtime.
 */
template<class T>
class AllocatedArray {
protected:
  unsigned size;
  T *gcc_restrict data;

public:
  typedef T *iterator;
  typedef const T *const_iterator;

public:
  gcc_constexpr_ctor AllocatedArray(): size(0), data(NULL) {}

  explicit AllocatedArray(unsigned _size): size(_size), data(new T[size]) {
    assert(Size() == 0 || data != NULL);
  }

  explicit AllocatedArray(const AllocatedArray &other)
    :size(other.Size()), data(new T[size]) {
    assert(Size() == 0 || data != NULL);
    assert(other.Size() == 0 || other.data != NULL);

    std::copy(other.data, other.data + size, data);
  }

  explicit AllocatedArray(AllocatedArray &&other)
    :size(other.size), data(other.data) {
    other.size = 0;
    other.data = NULL;
  }

  ~AllocatedArray() {
    delete[] data;
  }

  AllocatedArray &operator=(const AllocatedArray &other) {
    assert(Size() == 0 || data != NULL);
    assert(other.Size() == 0 || other.data != NULL);

    if (&other == this)
      return *this;

    ResizeDiscard(other.Size());
    std::copy(other.begin(), other.end(), data);
    return *this;
  }

  AllocatedArray &operator=(AllocatedArray &&other) {
    delete[] data;
    size = other.size;
    data = other.data;
    other.size = 0;
    other.data = NULL;
  }

  /**
   * Returns the number of allocated elements.
   */
  unsigned Size() const {
    return size;
  }

  /**
   * Returns one element.  No bounds checking.
   */
  T &operator[](unsigned i) {
    assert(i < Size());

    return data[i];
  }

  /**
   * Returns one constant element.  No bounds checking.
   */
  const T &operator[](unsigned i) const {
    assert(i < Size());

    return data[i];
  }

  iterator begin() {
    return data;
  }

  const_iterator begin() const {
    return data;
  }

  iterator end() {
    return data + size;
  }

  const_iterator end() const {
    return data + size;
  }

  /**
   * Resizes the array, discarding old data.
   */
  void ResizeDiscard(unsigned _size) {
    if (_size == size)
      return;

    delete[] data;
    size = _size;
    data = new T[size];

    assert(Size() == 0 || data != NULL);
  }

  /**
   * Grows the array to the specified size, discarding old data.
   * Similar to ResizeDiscard(), but will never shrink the array to
   * avoid expensive heap operations.
   */
  void GrowDiscard(unsigned _size) {
    if (_size > size)
      ResizeDiscard(_size);
  }

  /**
   * Grows the array to the specified size, preserving the value of a
   * range of elements, starting from the beginning.
   */
  void GrowPreserve(unsigned _size, unsigned preserve) {
    if (_size <= size)
      return;

    T *new_data = new T[_size];
    assert(_size == 0 || new_data != NULL);

    std::copy(data, data + preserve, new_data);
    delete[] data;
    data = new_data;
    size = _size;
  }
};

#endif
