/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "DebugReplay.hpp"
#include "DebugReplayIGC.hpp"
#include "DebugReplayNMEA.hpp"
#include "OS/Args.hpp"
#include "OS/PathName.hpp"
#include "Computer/Settings.hpp"

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

  if (!args.IsEmpty() && MatchesExtension(args.PeekNext(), ".igc")) {
    replay = DebugReplayIGC::Create(args.ExpectNextPath());
  } else {
    const auto driver_name = args.ExpectNextT();
    const auto input_file = args.ExpectNextPath();
    replay = DebugReplayNMEA::Create(input_file, driver_name);
  }

  return replay;
}
