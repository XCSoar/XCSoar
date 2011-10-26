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

#ifndef XCSOAR_BATCH_BUFFER_HPP
#define XCSOAR_BATCH_BUFFER_HPP

#include <algorithm>

/**
 * A fixed-size buffer which gets flushed by the caller after it
 * becomes full.
 *
 * Not thread safe.
 */
template<class T, unsigned size>
class BatchBuffer {
protected:
  T data[size];
  unsigned tail;

public:
  BatchBuffer()
    :tail(0) {}

  BatchBuffer(const BatchBuffer<T,size> &other)
    :tail(other.tail) {
    std::copy(other.data, other.data + tail, data);
  }

protected:
  static unsigned Next(unsigned i) {
    return (i + 1) % size;
  }

public:
  bool IsEmpty() const {
    return tail == 0;
  }

  bool IsFull() const {
    return tail == size;
  }

  unsigned Length() const {
    return tail;
  }

  const T &operator[](unsigned i) const {
    return data[i];
  }

  T &Append() {
    return data[tail++];
  }

  void Clear() {
    tail = 0;
  }
};

#endif
