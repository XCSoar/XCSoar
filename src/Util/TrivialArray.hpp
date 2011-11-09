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

#ifndef XCSOAR_TRIVIAL_ARRAY_HPP
#define XCSOAR_TRIVIAL_ARRAY_HPP

#include "Compiler.h"

#include <assert.h>
#include <algorithm>

/**
 * An array with a maximum size known at compile time.  It keeps track
 * of the actual length at runtime. The clear() function needs to be called
 * to initialize the class properly.
 */
template<class T, unsigned max>
class TrivialArray {
public:
  typedef unsigned size_type;
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;

protected:
  size_type the_size;
  T data[max];

  gcc_constexpr_ctor
  TrivialArray(size_type _size):the_size(_size) {}

public:
  /**
   * Non-initialising constructor.
   */
  TrivialArray() = default;

  gcc_constexpr_method
  size_type capacity() const { return max; }

  gcc_constexpr_method
  size_type max_size() const {
    return max;
  }

  /**
   * Returns the number of allocated elements.
   */
  gcc_constexpr_method
  size_type size() const {
    return the_size;
  }

  void shrink(size_type _size) {
    assert(_size <= the_size);

    the_size = _size;
  }

  gcc_constexpr_method
  bool empty() const {
    return the_size == 0;
  }

  gcc_constexpr_method
  bool full() const {
    return the_size == max;
  }

  /**
   * Empties this array, but does not destruct its elements.
   */
  void clear() {
    the_size = 0;
  }

  /**
   * Returns one element.  No bounds checking.
   */
  T &operator[](size_type i) {
    assert(i < size());

    return data[i];
  }

  /**
   * Returns one constant element.  No bounds checking.
   */
  const T &operator[](size_type i) const {
    assert(i < size());

    return data[i];
  }

  iterator begin() {
    return data;
  }

  const_iterator begin() const {
    return data;
  }

  iterator end() {
    return data + the_size;
  }

  const_iterator end() const {
    return data + the_size;
  }

  T &last() {
    assert(the_size > 0);

    return data[the_size - 1];
  }

  const T &last() const {
    assert(the_size > 0);

    return data[the_size - 1];
  }

  bool contains(const T &value) const {
    return std::find(begin(), end(), value) != end();
  }

  /**
   * Return address of start of data segment.
   */
  const T* raw() const {
    return data;
  }

  /**
   * Append an element at the end of the array, increasing the length
   * by one.  No bounds checking.
   */
  void append(const T &value) {
    assert(!full());

    data[the_size++] = value;
  }

  /**
   * Increase the length by one and return a pointer to the new
   * element, to be modified by the caller.  No bounds checking.
   */
  T &append() {
    assert(!full());

    return data[the_size++];
  }

  /**
   * Like append(), but checks if the array is already full (returns
   * false in this case).
   */
  bool checked_append(const T &value) {
    if (full())
      return false;

    append(value);
    return true;
  }

  /**
   * Remove an item by copying the last item over it.
   */
  void quick_remove(size_type i) {
    assert(i < size());

    if (i < size() - 1)
      data[i] = data[size() - 1];

    --the_size;
  }

  /* STL API emulation */

  void push_back(const T &value) {
    append(value);
  }

  T &front() {
    assert(the_size > 0);

    return data[0];
  }

  const T &front() const {
    assert(the_size > 0);

    return data[0];
  }

  T &back() {
    return last();
  }

  const T &back() const {
    return last();
  }
};

#endif
