// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplay.hpp"
#include "DebugReplayIGC.hpp"
#include "DebugReplayNMEA.hpp"
#include "system/Args.hpp"
#include "system/PathName.hpp"
#include "Computer/Settings.hpp"
#include "util/StringCompare.hxx"

DebugReplay::DebugReplay()
  :glide_polar(1)
{
  raw_basic.Reset();
  computed_basic.Reset();
  calculated.Reset();

  flying_computer.Reset();

  wrap_clock.Reset();

  qnh = AtmosphericPressure::Standard();
}

DebugReplay::~DebugReplay()
{
}

void
DebugReplay::Compute()
{
  computed_basic.Reset();
  (NMEAInfo &)computed_basic = raw_basic;
  wrap_clock.Normalise(computed_basic);

  FeaturesSettings features;
  features.nav_baro_altitude_enabled = true;
  computer.Fill(computed_basic, qnh, features);

  computer.Compute(computed_basic, last_basic, last_basic, calculated);
  flying_computer.Compute(glide_polar.GetVTakeoff(),
                          computed_basic, calculated,
                          calculated.flight);
}

DebugReplay *
CreateDebugReplay(Args &args)
{
  DebugReplay *replay;

  if (!args.IsEmpty() && StringEndsWithIgnoreCase(args.PeekNext(), ".igc")) {
    replay = DebugReplayIGC::Create(args.ExpectNextPath());
  } else {
    const auto driver_name = args.ExpectNextT();
    const auto input_file = args.ExpectNextPath();
    replay = DebugReplayNMEA::Create(input_file, driver_name);
  }

  return replay;
}
