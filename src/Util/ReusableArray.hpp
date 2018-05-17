/*
 * Copyright (C) 2010 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef REUSABLE_ARRAY_HPP
#define REUSABLE_ARRAY_HPP

#include "Util/AllocatedArray.hxx"

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
    return array.begin();
  }

  /**
   * Grows an existing array, preserving data.
   */
  T *grow(size_type old_length, size_type new_length) {
    array.grow_preserve(new_length, old_length);
    return array.begin();
  }
};

#endif
