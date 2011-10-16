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

#ifndef XCSOAR_UTIL_TINY_ENUM_HPP
#define XCSOAR_UTIL_TINY_ENUM_HPP

#include "Compiler.h"

#include <stdint.h>

/**
 * A template that stores an enum value inside one byte.  It tries to
 * behave just like the original enum type, but it is smaller than the
 * real one (1 byte instead of 4 bytes).  This reduces memory usage by
 * packing it more tightly in structs.  The allowed integer range for
 * a TinyEnum is 0..255.
 */
template<typename E>
class TinyEnum {
  uint8_t value;

public:
  /**
   * Non-initialising constructor.
   */
  TinyEnum() {}

  gcc_constexpr_ctor
  TinyEnum(E _value):value((uint8_t)_value) {}

  TinyEnum<E> &operator =(E _value) {
    value = (uint8_t)_value;
    return *this;
  }

  gcc_constexpr_method
  operator E() const {
    return (E)value;
  }

  gcc_constexpr_method
  bool operator ==(E other) const {
    return value == (uint8_t)other;
  }

  gcc_constexpr_method
  bool operator !=(E other) const {
    return value != (uint8_t)other;
  }
};

#endif
