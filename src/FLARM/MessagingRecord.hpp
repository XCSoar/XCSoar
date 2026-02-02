// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RadioFrequency.hpp"
#include "Id.hpp"
#include "util/StaticString.hxx"
#include "util/Macros.hpp"

#include <string>
#include <cstdint>

struct MessagingRecord {
  FlarmId id;
  std::string pilot;
  std::string plane_type;
  std::string registration;
  std::string callsign;
  RadioFrequency frequency = RadioFrequency::Null();

  /**
   * Epoch-based cycle tracking. `epoch` starts at 1 so that the first
   * received field does not prematurely trigger a cycle boundary.
   */
  uint8_t epoch = 1;
  uint8_t reg_epoch = 0;
  uint8_t pilot_epoch = 0;
  uint8_t plane_epoch = 0;
  uint8_t callsign_epoch = 0;

  enum class Field : uint8_t { 
    Registration,
    Pilot,
    PlaneType,
    Callsign,
    Count // Helper to report the number of tracked fields.
  };

  struct FieldMapping {
    std::string MessagingRecord::*value;
    uint8_t MessagingRecord::*epoch;
  };

  /**
   * Total number of tracked messaging fields.
   */
  static inline constexpr uint8_t kFieldCount = static_cast<uint8_t>(Field::Count);

  /**
   * Maps each messaging field to the member storing its last-seen epoch.
   */
  static inline constexpr FieldMapping kFields[kFieldCount] = {
    { &MessagingRecord::registration, &MessagingRecord::reg_epoch },
    { &MessagingRecord::pilot,        &MessagingRecord::pilot_epoch },
    { &MessagingRecord::plane_type,   &MessagingRecord::plane_epoch },
    { &MessagingRecord::callsign,     &MessagingRecord::callsign_epoch },
  };

  static_assert(ARRAY_SIZE(kFields) == kFieldCount, "kFields array size must match Field::Count");

  /**
   * Returns the value string for the requested messaging field.
   */
  inline std::string &GetFieldValue(Field f) noexcept {
    return this->*kFields[static_cast<uint8_t>(f)].value;
  }

  /**
   * Const overload for retrieving a field value.
   */
  inline const std::string &GetFieldValueConst(Field f) const noexcept {
    return this->*kFields[static_cast<uint8_t>(f)].value;
  }

  /**
   * Accesses the epoch counter tied to the requested messaging field.
   */
  inline uint8_t &GetFieldEpoch(Field f) noexcept {
    return this->*kFields[static_cast<uint8_t>(f)].epoch;
  }

  /**
   * Number of fields tracked in `kFields`.
   */
  static inline constexpr uint8_t GetFieldCount() noexcept { return kFieldCount; }

  /**
   * Format a UTF-8 value into the provided buffer; returns nullptr if empty/invalid. 
   */
  const TCHAR *Format(StaticString<256> &buffer, const std::string &value) const noexcept;
};
