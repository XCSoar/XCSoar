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

#include "NMEA/Validity.hpp"

#include <type_traits>
#include <cstdint>
#include <tchar.h>

#ifdef _WIN32
#undef NO_ERROR
#endif

/**
 * The FLARM self-test results read from the PFLAE sentence.
 */
struct FlarmError {
  enum Severity : uint8_t {
    NO_ERROR = 0x00,
    INFORMATION_ONLY = 0x01,
    REDUCED_FUNCTIONALITY = 0x02,
    FATAL_PROBLEM = 0x03,
  };

  enum Code : uint8_t {
    FIRMWARE_TIMEOUT = 0x11,
    POWER = 0x21,
    GPS_COMMUNICATION = 0x31,
    GPS_CONFIGURATION = 0x32,
    RF_COMMUNICATION = 0x41,
    COMMUNICATION = 0x51,
    FLASH_MEMORY = 0x61,
    PRESSURE_SENSOR = 0x71,
    OBSTACLE_DATABASE = 0x81,
    FLIGHT_RECORDER = 0x91,
    TRANSPONDER_RECEIVER = 0xa1,
    OTHER = 0xf1,
  };

  Validity available;

  Severity severity;
  Code code;

  constexpr bool IsWarning() const noexcept {
    return severity >= REDUCED_FUNCTIONALITY;
  }

  constexpr bool IsError() const noexcept {
    return severity >= FATAL_PROBLEM;
  }

  constexpr void Clear() noexcept {
    available.Clear();
  }

  constexpr void Complement(const FlarmError &add) noexcept {
    if (available.Complement(add.available)) {
      severity = add.severity;
      code = add.code;
    }
  }

  constexpr void Expire([[maybe_unused]] TimeStamp clock) noexcept {
    /* no expiry; this object will be cleared only when the device
       connection is lost */
  }

  /**
   * Returns a human-readable translatable string for the given value.
   * The caller is responsible for calling gettext() on the return
   * value.
   */
  [[gnu::const]]
  static const TCHAR *ToString(Severity severity) noexcept;

  /**
   * Returns a human-readable translatable string for the given value.
   * The caller is responsible for calling gettext() on the return
   * value.
   */
  [[gnu::const]]
  static const TCHAR *ToString(Code code) noexcept;

  [[gnu::pure]]
  const TCHAR *GetSeverityString() const noexcept {
    return ToString(severity);
  }

  [[gnu::pure]]
  const TCHAR *GetCodeString() const noexcept {
    return ToString(code);
  }
};

static_assert(std::is_trivial<FlarmError>::value, "type is not trivial");
