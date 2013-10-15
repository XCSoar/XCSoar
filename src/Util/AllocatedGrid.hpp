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

#ifndef ALLOCATED_GRID_HPP
#define ALLOCATED_GRID_HPP

#include "AllocatedArray.hpp"

#include <assert.h>

/**
 * A two dimensional array allocated on the heap with a length
 * determined at runtime.
 */
template<class T>
class AllocatedGrid {
protected:
  AllocatedArray<T> array;
  unsigned width, height;

public:
  typedef typename AllocatedArray<T>::iterator iterator;
  typedef typename AllocatedArray<T>::const_iterator const_iterator;

  constexpr AllocatedGrid():width(0), height(0) {}
  AllocatedGrid(unsigned _width, unsigned _height)
    :array(_width * _height), width(_width), height(_height) {}

  constexpr bool IsDefined() const {
    return array.size() > 0;
  }

  constexpr unsigned GetWidth() const {
    return width;
  }

  constexpr unsigned GetHeight() const {
    return height;
  }

  constexpr unsigned GetSize() const {
    return width * height;
  }

  const T &Get(unsigned x, unsigned y) const {
    assert(x < width);
    assert(y < height);

    return array[y * width + x];
  }

  T &Get(unsigned x, unsigned y) {
    assert(x < width);
    assert(y < height);

    return array[y * width + x];
  }

  const T &GetLinear(unsigned i) const {
    assert(i < GetSize());

    return array[i];
  }

  T &GetLinear(unsigned i) {
    assert(i < GetSize());

    return array[i];
  }

  iterator begin() {
    return array.begin();
  }

  constexpr const_iterator begin() const {
    return array.begin();
  }

  iterator end() {
    return begin() + GetSize();
  }

  constexpr const_iterator end() const {
    return begin() + GetSize();
  }

  iterator GetPointerAt(unsigned x, unsigned y) {
    assert(x < width);
    assert(y < height);

    return begin() + y * width + x;
  }

  const_iterator GetPointerAt(unsigned x, unsigned y) const {
    assert(x < width);
    assert(y < height);

    return begin() + y * width + x;
  }

  void Reset() {
    width = height = 0;
    array.ResizeDiscard(0);
  }

  void GrowDiscard(unsigned _width, unsigned _height) {
    array.GrowDiscard(_width * _height);
    width = _width;
    height = _height;
  }

  /**
   * Resize the grid, preserving as many old values as fit into the
   * new dimensions, and fill newly allocated array slots.
   */
  void GrowPreserveFill(unsigned _width, unsigned _height, const T &fill=T()) {
    if (_width < width) {
      const unsigned h = std::min(height, _height);
      const auto end = array.begin() + h * width;

      for (auto in = array.begin() + width, out = array.begin() + _width;
           in < end; in += width) {
        out = std::move(in, in + _width, out);
      }

      width = _width;
    }

    array.GrowPreserve(_width * _height, width * height);

    if (_width > width) {
      const unsigned h = std::min(height, _height);
      const auto end = array.begin();

      for (auto in = array.begin() + (h - 1) * width,
             out = array.begin() + (h - 1) * _width + width;
           in < end; in -= width, out -= _width) {
        std::move_backward(in, in + width, out);
        std::fill(out, out + _width - width, fill);
      }

      width = _width;
    }

    std::fill(array.begin() + width * height, array.end(), fill);

    height = _height;
  }
};

#endif
