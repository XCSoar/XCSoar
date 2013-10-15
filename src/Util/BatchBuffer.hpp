/*
 * Copyright (C) 2010-2012 Max Kellermann <max@duempel.org>
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

#ifndef BATCH_BUFFER_HPP
#define BATCH_BUFFER_HPP

#include "StaticArray.hpp"

#include <algorithm>
#include <stddef.h>

/**
 * A fixed-size buffer which gets flushed by the caller after it
 * becomes full.
 *
 * Not thread safe.
 */
template<class T, size_t size>
class BatchBuffer {
  typedef class StaticArray<T, size> Array;

public:
  typedef typename Array::size_type size_type;
  typedef typename Array::const_iterator const_iterator;

protected:
  Array data;

public:
  BatchBuffer() = default;

  BatchBuffer(const BatchBuffer<T,size> &other) {
    data.resize(other.Length());
    std::copy(other.data.begin(), other.data.end(), data.begin());
  }

  bool IsEmpty() const {
    return data.empty();
  }

  bool IsFull() const {
    return data.full();
  }

  size_type Length() const {
    return data.size();
  }

  const T &operator[](size_type i) const {
    return data[i];
  }

  T &Append() {
    return data.append();
  }

  void Clear() {
    data.clear();
  }

  const_iterator begin() const {
    return data.begin();
  }

  const_iterator end() const {
    return data.end();
  }
};

#endif
