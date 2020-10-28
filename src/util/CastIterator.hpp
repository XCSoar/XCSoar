/*
 * Copyright (C) 2010-2011 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef CAST_ITERATOR_HPP
#define CAST_ITERATOR_HPP

#include <iterator>

template<typename VT, typename IT>
class CastIterator {
  IT iterator;

public:
  typedef typename IT::iterator_category iterator_category;
  typedef typename IT::difference_type difference_type;
  typedef VT value_type;
  typedef VT *pointer;
  typedef VT &reference;

  CastIterator() = default;

  constexpr
  CastIterator(const IT _iterator):iterator(_iterator) {}

  reference operator*() const {
    return static_cast<reference>(*iterator);
  }

  pointer operator->() const {
    return static_cast<pointer>(iterator.IT::operator->());
  }

  CastIterator<VT,IT> &operator++() {
    iterator = ++iterator;
    return *this;
  }

  CastIterator<VT,IT> operator++(int) {
    CastIterator<VT,IT> old = *this;
    iterator++;
    return old;
  }

  CastIterator<VT,IT> &operator--() {
    iterator = --iterator;
    return *this;
  }

  CastIterator<VT,IT> operator--(int) {
    CastIterator<VT,IT> old = *this;
    iterator--;
    return old;
  }

  bool operator==(const CastIterator<VT,IT> &other) const {
    return iterator == other.iterator;
  }

  bool operator!=(const CastIterator<VT,IT> &other) const {
    return iterator != other.iterator;
  }
};

#endif
