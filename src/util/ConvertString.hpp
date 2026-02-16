
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "UTF8.hpp"
#include "StringPointer.hxx"

#include <cassert>

/**
 * Convert a UTF-8 string to a char string.  The source buffer passed
 * to the constructor must be valid as long as this object is being
 * used.
 */

/**
 * Convert a char string to ACP (Windows ANSI code page).  The source
 * buffer passed to the constructor must be valid as long as this
 * object is being used.
 */
class WideToACPConverter {
  typedef StringPointer<> Value;
  typedef typename Value::const_pointer const_pointer;

  Value value;

public:
  WideToACPConverter(const_pointer _value) noexcept
    :value(_value)
  {
    assert(_value != nullptr);
  }

  WideToACPConverter(const WideToACPConverter &other) = delete;
  WideToACPConverter &operator=(const WideToACPConverter &other) = delete;

  [[gnu::pure]]
  bool IsValid() const noexcept {
    assert(value != nullptr);

    return true;
  }

  const_pointer c_str() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }

  operator const_pointer() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }
};
