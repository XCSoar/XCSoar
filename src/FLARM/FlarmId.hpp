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

  constexpr
  FlarmId(uint32_t _value):value(_value) {}

public:
  FlarmId() = default;

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
