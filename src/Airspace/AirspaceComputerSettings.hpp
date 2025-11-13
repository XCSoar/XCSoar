// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceWarningConfig.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_HTTP
#include "NOTAM/Settings.hpp"
#endif

#include <type_traits>

/**
 * Settings for airspace options
 */
struct AirspaceComputerSettings {
  /** Airspace warnings enabled (true/false) */
  bool enable_warnings;

  AirspaceWarningConfig warnings;

#ifdef HAVE_HTTP
  /** NOTAM settings */
  NOTAMSettings notam;
#endif

  void SetDefaults();
};

// Note: AirspaceComputerSettings is no longer trivial due to NOTAMSettings containing std::chrono and std::string
