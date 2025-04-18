// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AirspaceClass.hpp"

#include <cassert>
#include <chrono>

struct AirspaceWarningConfig
{
  using Duration = std::chrono::duration<unsigned>;

  /** Warning time before airspace entry */
  Duration warning_time;

  /** Enable repetitive sound */
  bool repetitive_sound;

  /** Time an acknowledgement will persist before a warning is reissued */
  Duration acknowledgement_time;

  /** Altitude margin (m) outside of which to not display airspace for auto mode */
  unsigned altitude_warning_margin;

  /** Class-specific warning flags */
  bool class_warnings[AIRSPACECLASSCOUNT];

  void SetDefaults();

  bool IsClassEnabled(AirspaceClass cls) const {
    assert((unsigned)cls < AIRSPACECLASSCOUNT);

    return class_warnings[(unsigned)cls];
  }
};
