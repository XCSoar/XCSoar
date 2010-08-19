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

#ifndef XCSOAR_ACTIVE_LIST_HPP
#define XCSOAR_ACTIVE_LIST_HPP

#include <algorithm>
#include <assert.h>

/**
 * A list of active objects.  This is used by RasterTileCache to
 * search the list of active tiles quickly.
 */
template<class T, unsigned size>
class ActiveList {
protected:
  T *data[size];
  unsigned tail;

public:
  ActiveList():tail(0) {}

protected:
  static unsigned next(unsigned i) {
    return (i + 1) % size;
  }

public:
  bool empty() const {
    return tail == 0;
  }

  bool full() const {
    return tail == size;
  }

  unsigned length() const {
    return tail;
  }

  T &operator[](unsigned i) const {
    assert(i < tail);

    return *data[i];
  }

  void append(T &value) {
    assert(tail < size);

    data[tail++] = &value;
  }

  void clear() {
    tail = 0;
  }

  /**
   * Move an element to the front, to improve its locality in the next
   * linear scan.
   */
  void move_to_front(unsigned i) {
    assert(i < tail);

    if (i > 0)
      std::swap(data[0], data[i]);
  }
};

#endif
