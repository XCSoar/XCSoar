/*
 * Copyright (C) 2011 Max Kellermann <max@duempel.org>
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

#ifndef XCSOAR_CONST_ARRAY_HPP
#define XCSOAR_CONST_ARRAY_HPP

#include "Compiler.h"

#include <assert.h>
#include <stddef.h>

/**
 * A reference to an array that is initialised at compile time.  It
 * provides a STL-like API.
 */
template<class T>
class ConstArray {
public:
  typedef size_t size_type;
  typedef T value_type;
  typedef const T *const_pointer;
  typedef const T &const_reference;
  typedef const T *const_iterator;

protected:
  const_pointer data;
  size_type the_size;

public:
  gcc_constexpr_ctor
  ConstArray(const_pointer _data, size_type _size)
    :data(_data), the_size(_size) {}

  gcc_constexpr_method
  size_type size() const {
    return the_size;
  }

  gcc_constexpr_method
  bool empty() const {
    return size() == 0;
  }

  /**
   * Returns one element.  No bounds checking.
   */
  const_reference operator[](size_type i) const {
    assert(i < size());

    return data[i];
  }

  const_iterator begin() const {
    return data;
  }

  const_iterator end() const {
    return begin() + size();
  }

  const_reference front() const {
    assert(!empty());

    return data[0];
  }

  const_reference last() const {
    assert(!empty());

    return data[size() - 1];
  }
};

#endif
