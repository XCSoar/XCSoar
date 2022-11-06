/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#pragma once

#include <cassert>
#include <cstdint>
#include <compare>
#include <cstddef>

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
  uint16_t value;

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

  friend constexpr auto operator<=>(RadioFrequency,
                                    RadioFrequency) noexcept = default;

  constexpr bool IsDefined() const noexcept {
    return value != 0;
  }

  /**
   * Set this object to "undefined".
   */
  constexpr void Clear() noexcept {
    value = 0;
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
};
