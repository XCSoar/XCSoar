// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceWarningConfig.hpp"

#include <type_traits>

/**
 * Settings for airspace options
 */
struct AirspaceComputerSettings {
  /** Airspace warnings enabled (true/false) */
  bool enable_warnings;

  AirspaceWarningConfig warnings;

  void SetDefaults();
};

static_assert(std::is_trivial<AirspaceComputerSettings>::value, "type is not trivial");
