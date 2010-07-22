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

#ifndef XCSOAR_REUSABLE_ARRAY_HPP
#define XCSOAR_REUSABLE_ARRAY_HPP

#include "Util/NonCopyable.hpp"

#include <algorithm>
#include <stddef.h>

template<class T>
class ReusableArray : private NonCopyable {
protected:
  T *data;
  unsigned length;

public:
  ReusableArray():data(NULL), length(0) {}
  ReusableArray(unsigned _length):data(new T[_length]), length(_length) {}

  ~ReusableArray() {
    if (data != NULL)
      delete[] data;
  }

  /**
   * Obtains an array.  Its values are undefined.
   */
  T *get(unsigned _length) {
    if (_length > length) {
      delete[] data;
      length = _length;
      data = new T[length];
    }

    return data;
  }

  /**
   * Grows an existing array, preserving data.
   */
  T *grow(unsigned old_length, unsigned new_length) {
    if (new_length >= length) {
      T *new_data = new T[new_length];
      if (new_data == NULL)
        return NULL;

      std::copy(data, data + old_length, new_data);
      delete[] data;
      data = new_data;
      length = new_length;
    }

    return data;
  }
};

#endif
