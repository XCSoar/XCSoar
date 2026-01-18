// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <string>
#include <string_view>

/**
 * This class stores a 4-digit octal transponder code.
 */
class TransponderCode {
  uint_least16_t value;

public:
  /**
   * Uninitialized.
   */
  TransponderCode() = default;

  /**
   * Construct an empty instance.  Its IsDefined() method will return
   * false.
   */
  static constexpr TransponderCode Null() noexcept {
    return TransponderCode(010000);
  }

  /**
   * Construct an instance from an octal code.
   */
  explicit constexpr TransponderCode(uint_least16_t code) noexcept
    :value(code) {}

  /**
   * @return an octal representation of the transponder code
   */
  constexpr uint_least16_t GetCode() const noexcept {
    assert(IsDefined());

    return value;
  }

  constexpr bool IsDefined() const noexcept {
    return value <= 07777;
  }

  /**
   * Set this object to "undefined".
   */
  constexpr void Clear() noexcept {
    *this = Null();
  }

  /**
   * Format the transponder code as a string (e.g., "7000").
   * Returns an empty string if the code is not defined.
   */
  [[nodiscard]]
  std::string Format() const noexcept;

  /**
   * Parse a 4-digit octal transponder code (e.g., "7000").
   * Returns Null() if the input is invalid.
   */
  [[gnu::pure]]
  static TransponderCode Parse(std::string_view s) noexcept;
};
