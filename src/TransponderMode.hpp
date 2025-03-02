// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>
#include <cstdint>
#include <tchar.h>
#include <cassert>

/**
 * This struct holds valid transponder modes.
 */
struct TransponderMode {
  enum Mode : uint_least8_t {
    UNDEFINED,
    OFF,
    SBY,
    GND,
    ON,
    ALT,
    IDENT,
  };

  Mode mode;

  /**
   * Uninitialized.
   */
  TransponderMode() = default;

  /**
   * Construct an empty instance.  Its IsDefined() method will return
   * false.
   */
  static constexpr TransponderMode Null() noexcept {
    return TransponderMode(Mode::UNDEFINED);
  }

  explicit constexpr TransponderMode(Mode m) noexcept 
    :mode(m) {}

  constexpr bool IsDefined() const noexcept {
    return mode != Mode::UNDEFINED;
  }

  /**
   * Set this object to "undefined".
   */
  constexpr void Clear() noexcept {
    *this = Null();
  }

  /**
   * Returns a human-readable string for the given value.
   * The caller is responsible for calling gettext() on the return
   * value.
   */
  [[gnu::const]]
  static const TCHAR *ToString(Mode mode) noexcept;

  [[gnu::pure]]
  const TCHAR *GetModeString() const noexcept {
    assert(IsDefined());

    return ToString(mode);
  }
};

static_assert(std::is_trivial<TransponderMode>::value, "type is not trivial");
