// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/OptionalPercent.hxx"

#include <cstdint>

namespace Power {

struct BatteryInfo {
  OptionalPercent remaining_percent = std::nullopt;
};

struct ExternalInfo {
  enum class Status : uint_least8_t {
    UNKNOWN,
    OFF,
    ON,
  };

  Status status = Status::UNKNOWN;
};

struct Info {
  BatteryInfo battery;
  ExternalInfo external;
};

} // namespace Power
