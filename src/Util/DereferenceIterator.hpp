/*
 * Copyright (C) 2012 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef DEREFERENCE_ITERATOR_HPP
#define DEREFERENCE_ITERATOR_HPP

#include <iterator>
#include <type_traits>

/**
 * An iterator wrapper that dereferences the values returned by the
 * original iterator.
 */
template<typename IT, typename VT=std::remove_pointer<typename IT::value_type>>
class DereferenceIterator {
  typedef std::iterator_traits<IT> Traits;

  IT original;

public:
  typedef typename Traits::iterator_category iterator_category;
  typedef typename Traits::difference_type difference_type;
  typedef VT value_type;
  typedef VT *pointer;
  typedef VT &reference;

  DereferenceIterator() = default;

  constexpr
  DereferenceIterator(const IT _original):original(_original) {}

  reference operator*() const {
    return static_cast<reference>(**original);
  }

  pointer operator->() const {
    return static_cast<reference>(*original.IT::operator->());
  }

  DereferenceIterator<IT,VT> &operator++() {
    original = ++original;
    return *this;
  }

  DereferenceIterator<IT,VT> operator++(int) {
    DereferenceIterator<IT,VT> old = *this;
    original++;
    return old;
  }

  DereferenceIterator<IT,VT> &operator--() {
    original = --original;
    return *this;
  }

  DereferenceIterator<IT,VT> operator--(int) {
    DereferenceIterator<IT,VT> old = *this;
    original--;
    return old;
  }

  bool operator==(const DereferenceIterator<IT,VT> &other) const {
    return original == other.original;
  }

  bool operator!=(const DereferenceIterator<IT,VT> &other) const {
    return original != other.original;
  }
};

/**
 * A container wrapper that wraps the iterators in a
 * DereferenceIterator.
 */
template<typename CT, typename VT=std::remove_pointer<typename CT::value_type>>
class DereferenceContainerAdapter {
  CT &original;

public:
  typedef VT value_type;
  typedef VT *pointer;
  typedef VT &reference;

  typedef DereferenceIterator<typename CT::iterator, VT> iterator;
  typedef DereferenceIterator<typename CT::const_iterator, const VT> const_iterator;

  DereferenceContainerAdapter(CT &_original):original(_original) {}

#if 0
  iterator begin() {
    return original.begin();
  }
#endif

  const_iterator begin() const {
    return original.begin();
  }

  const_iterator cbegin() const {
    return original.begin();
  }

#if 0
  iterator end() {
    return original.end();
  }
#endif

  const_iterator end() const {
    return original.end();
  }

  const_iterator cend() const {
    return original.end();
  }
};

#endif
