// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceWarningConfig.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_HTTP
#include "NOTAM/Settings.hpp"
#endif

/**
 * Settings for airspace options
 */
struct AirspaceComputerSettings {
  /** Airspace warnings enabled (true/false) */
  bool enable_warnings;

  AirspaceWarningConfig warnings;

#ifdef HAVE_HTTP
  /**
   * NOTAM settings.
   *
   * This member makes AirspaceComputerSettings non-trivial because
   * NOTAMSettings contains StaticString members with default initializers.
   */
  NOTAMSettings notam;
#endif

  void SetDefaults();
};
