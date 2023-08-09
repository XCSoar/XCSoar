// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Units/Settings.hpp"
#include "Geo/CoordinateFormat.hpp"

#include <type_traits>

/**
 * Settings specifying the format in which values are displayed or
 * entered.
 */
struct FormatSettings {
  CoordinateFormat coordinate_format;

  UnitSetting units;

  void SetDefaults() noexcept {
      coordinate_format = CoordinateFormat::DDMMSS;
      units.SetDefaults();
  }
};

static_assert(std::is_trivial<FormatSettings>::value, "type is not trivial");
