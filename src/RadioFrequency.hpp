// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cassert>
#include <cstdint>
#include <compare>
#include <cstddef>
#include <string_view>

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

  static constexpr bool ChannelKhzValid(unsigned khz) noexcept {
    return khz >= MIN_KHZ && khz < MAX_KHZ && (khz % 5 == 0) &&
           (khz % 25) != 15 && (khz % 25) != 20;
  }

  constexpr RadioFrequency(unsigned _value) noexcept:value(_value) {}

public:
  /**
   * Uninitialized.  Use Null() for a properly initialized undefined instance.
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

  friend constexpr auto operator<=>(RadioFrequency,
                                    RadioFrequency) noexcept = default;

  constexpr bool IsDefined() const noexcept {
    if (value == 0)
      return false;

    const unsigned khz = BASE_KHZ + value;
    return ChannelKhzValid(khz);
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
   * VHF voice band is 118000 kHz up to (not including) 137000 kHz.
   * Channels are on a 5 kHz grid; in each 25 kHz block only offsets 0, 5 and
   * 10 kHz are real 8.33 kHz channels (offsets 15 and 20 are unused).
   */
  constexpr void SetKiloHertz(unsigned khz) noexcept {
    value = ChannelKhzValid(khz) ? (khz - BASE_KHZ) : 0;
  }

  constexpr void OffsetKiloHertz(int khz_offset) noexcept {
    int new_khz = static_cast<int>(GetKiloHertz()) + khz_offset;
    const int dir = khz_offset > 0 ? 5 : (khz_offset < 0 ? -5 : 0);

    if (dir != 0) {
      for (unsigned n = 0; n < 5u; ++n) {
        if (new_khz < static_cast<int>(MIN_KHZ) ||
            new_khz >= static_cast<int>(MAX_KHZ))
          break;
        const unsigned u = static_cast<unsigned>(new_khz);
        if (u % 5 != 0)
          break;
        const unsigned r = u % 25;
        if (r != 15 && r != 20)
          break;
        new_khz += dir;
      }
    }

    if (new_khz < static_cast<int>(MIN_KHZ) ||
        new_khz >= static_cast<int>(MAX_KHZ))
      Clear();
    else
      SetKiloHertz(static_cast<unsigned>(new_khz));
  }

  char *Format(char *buffer, size_t max_size) const noexcept;

  [[gnu::pure]]
  static RadioFrequency Parse(std::string_view src) noexcept;
};
