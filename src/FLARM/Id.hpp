// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <compare> // for the defaulted spaceship operator

#ifdef _UNICODE
#include <tchar.h>
#endif

/**
 * The identification number of a FLARM traffic.
 */
class FlarmId {
  static constexpr uint32_t UNDEFINED_VALUE = 0;

  uint32_t value;

  constexpr FlarmId(uint32_t _value) noexcept
    :value(_value) {}

public:
  constexpr FlarmId() noexcept = default;

  static constexpr FlarmId Undefined() noexcept {
    return FlarmId(UNDEFINED_VALUE);
  }

  constexpr bool IsDefined() const noexcept {
    return value != UNDEFINED_VALUE;
  }

  constexpr void Clear() noexcept {
    value = UNDEFINED_VALUE;
  }

  friend constexpr auto operator<=>(const FlarmId &,
                                    const FlarmId &) noexcept = default;

  static FlarmId Parse(const char *input, char **endptr_r) noexcept;
#ifdef _UNICODE
  static FlarmId Parse(const TCHAR *input, TCHAR **endptr_r) noexcept;
#endif

  const char *Format(char *buffer) const noexcept;
#ifdef _UNICODE
  const TCHAR *Format(TCHAR *buffer) const noexcept;
#endif
};
