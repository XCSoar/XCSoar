// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "IGC/IGCFix.hpp"
#include "time/BrokenDate.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

struct IGCFixEnhanced : public IGCFix
{
  BrokenDate date;

  TimeStamp clock;

  /* The detail level of this fix. -1 is not visible at all, 0 is always visible. */
  int level;

  /* Terrian elevation */
  int elevation;

  bool Apply(const NMEAInfo &basic, const DerivedInfo &calculated) {
    if (IGCFix::Apply(basic)) {
      date = basic.date_time_utc;
      clock = basic.time;

      if (calculated.terrain_valid)
        elevation = calculated.terrain_altitude;
      else
        elevation = -1000;

      return true;
    } else {
      return false;
    }
  };

  void Clear() {
    time = BrokenTime::Invalid();
    elevation = -1000;
    level = 0;
    ClearExtensions();
  };
};
