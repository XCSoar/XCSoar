// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringAPI.hxx"
#include "util/StringParser.hxx"

#include <cassert>
#include <cstdint>
#include <compare>
#include <cstddef>
#include <string_view>

#include <tchar.h>

/**
 * This class tores a VHF radio frequency.
 */
class RadioFrequency {
  static constexpr unsigned BASE_KHZ = 100000;
  static constexpr unsigned MIN_KHZ = BASE_KHZ + 18000;
  static constexpr unsigned MAX_KHZ = BASE_KHZ + 37000;

  /**
   * The radio frequency in kilohertz minus 100 MHz.
   */
  uint_least16_t value;

  constexpr RadioFrequency(unsigned _value) noexcept:value(_value) {}

public:
  /**
   * Uninitialized.
   */
  RadioFrequency() = default;

  /**
   * Construct an empty instance.  Its IsDefined() method will return
   * false.
   */
  static constexpr RadioFrequency Null() noexcept {
    return { 0 };
  }

  static constexpr RadioFrequency FromKiloHertz(unsigned khz) noexcept {
    RadioFrequency f;
    f.SetKiloHertz(khz);
    return f;
  }

  static constexpr RadioFrequency FromMegaKiloHertz(unsigned mhz,
                                                    unsigned khz) noexcept {
    RadioFrequency f;
    f.SetKiloHertz(mhz * 1000 + khz);
    return f;
  }

  static constexpr RadioFrequency Convert(double mhz) noexcept {
    RadioFrequency f;
    f.SetKiloHertz(mhz * 1000.0 + 0.49);  // for a correct truncation
    return f;
  }

  friend constexpr auto operator<=>(RadioFrequency,
                                    RadioFrequency) noexcept = default;

  constexpr bool IsDefined() const noexcept {
    return value != 0;
  }

  /**
   * Set this object to "undefined".
   */
  constexpr void Clear() noexcept {
    *this = Null();
  }

  constexpr unsigned GetKiloHertz() const noexcept {
    assert(IsDefined());

    return BASE_KHZ + value;
  }

  /**
   * VHF Voice channels range from 118000 kHz up to not including 137000 kHz
   * Valid 8.33 kHz channels must be a multiple of 5 kHz
   * Due to rounding from 8.33 kHz to multiples of 5 (for displaying), some
   * channels are invalid. These are matched by (value % 25) == 20.
   */
  constexpr void SetKiloHertz(unsigned khz) noexcept {
    value = (khz >= MIN_KHZ && khz < MAX_KHZ) &&
            (khz % 5 == 0) &&
            (khz % 25 != 20)
      ? (khz - BASE_KHZ)
      : 0;
  }

  constexpr void OffsetKiloHertz(int khz_offset) noexcept {
    auto new_khz = GetKiloHertz() + khz_offset;
    if ((new_khz % 25) == 20) {
      new_khz += khz_offset > 0 ? 5 : -5;
    }
    SetKiloHertz(new_khz);
  }

  TCHAR *Format(TCHAR *buffer, size_t max_size) const noexcept;

  [[gnu::pure]]
  static RadioFrequency Parse(const TCHAR *p) noexcept;

  [[gnu::pure]]
  static RadioFrequency Parse(std::string_view src) noexcept;

  [[gnu::pure]] 
  static RadioFrequency Parse(StringParser<> &src) noexcept;
};
