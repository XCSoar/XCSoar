// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  enum Code : uint16_t {
    FIRMWARE_TIMEOUT = 0x11,
    FIRMWARE_UPDATE_ERROR = 0x12,
    POWER = 0x21,
    UI = 0x22,
    AUDIO = 0x23,
    ADC = 0x24,
    SDCARD = 0x25,
    USB = 0x26,
    LED = 0x27,
    EEPROM = 0x28,
    GENERAL = 0x29,
    TRANSPONDER_ADSB = 0x2a,
    GPIO = 0x2c,
    GPS_COMMUNICATION = 0x31,
    GPS_CONFIGURATION = 0x32,
    GPS_ANTENNA = 0x33,
    RF_COMMUNICATION = 0x41,
    ID_SAME = 0x42,
    ID_WRONG = 0x43,
    COMMUNICATION = 0x51,
    FLASH_MEMORY = 0x61,
    PRESSURE_SENSOR = 0x71,
    OBSTACLE_DATABASE = 0x81,
    OBSTACLE_DATABASE_EXPIRED = 0x82,
    FLIGHT_RECORDER = 0x91,
    ENL = 0x93,
    RANGE_ANALYZER = 0x94,
    CONFIGURATION_ERROR = 0xa1,
    INVALID_OBSTACLE_LICENSE = 0xb1,
    INVALID_IGC_LICENSE = 0xb2,
    INVALID_AUD_LICENSE = 0xb3,
    INVALID_ENL_LICENSE = 0xb4,
    INVALID_RFB_LICENSE = 0xb5,
    INVALID_TIS_LICENSE = 0xb6,
    GENERIC = 0x100,
    FLASH_FS = 0x101,
    FAILURE_UPDATING_DISPLAY = 0x110,
    DEVICE_OUTSIDE_REGION = 0x120,
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
