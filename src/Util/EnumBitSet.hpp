/*
 * Copyright (C) 2013 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef ENUM_BIT_SET_HPP
#define ENUM_BIT_SET_HPP

#include "Compiler.h"

#include <stdint.h>

/**
 * A container that stores each enum value as a bit.
 *
 * @param E an enum type that has one element called "COUNT"
 */
template<typename E>
class EnumBitSet {
  typedef uint32_t I;

  static constexpr I ToMask(E e) {
    return I(1) << unsigned(e);
  }

  template<typename... Args>
  static constexpr I ToMask(E e, Args&&... args) {
    return ToMask(e) | ToMask(args...);
  }

  I mask;

  constexpr EnumBitSet(I _mask):mask(_mask) {}

public:
  static constexpr unsigned N = unsigned(E::COUNT);
  static_assert(N <= 32, "enum is too large");

  constexpr EnumBitSet():mask(0) {}

  template<typename... Args>
  constexpr EnumBitSet(E e, Args&&... args)
    :mask(ToMask(e, args...)) {}

  constexpr EnumBitSet operator|(const EnumBitSet other) const {
    return EnumBitSet(mask | other.mask);
  }

  EnumBitSet &operator|=(const EnumBitSet &other) {
    mask |= other.mask;
    return *this;
  }

  constexpr bool IsEmpty() const {
    return mask == 0;
  }

  gcc_pure
  E UncheckedFirst() const {
    I i = 1;

    for (unsigned j = 0;; ++j, i <<= 1)
      if (mask & i)
        return E(j);
  }

  void Add(const E e) {
    mask |= ToMask(e);
  }

  constexpr bool Contains(E e) const {
    return (mask & ToMask(e)) != 0;
  }

  template<typename O>
  void CopyTo(O o) const {
    for (unsigned i = 0; i < N; ++i) {
      const E e = E(i);
      if (Contains(e))
        *o++ = e;
    }
  }
};

#endif
