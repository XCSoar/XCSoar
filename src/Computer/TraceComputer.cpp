/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "TraceComputer.hpp"
#include "SettingsComputer.hpp"
#include "Engine/Navigation/Aircraft.hpp"

TraceComputer::TraceComputer()
 :full(60),
  sprint(0, 9000, 300)
{
}

void
TraceComputer::Reset()
{
  mutex.Lock();
  full.clear();
  mutex.Unlock();

  sprint.clear();
  last_time = fixed_zero;
}

void
TraceComputer::LockedCopyTo(TracePointVector &v) const
{
  mutex.Lock();
  full.get_trace_points(v);
  mutex.Unlock();
}

void
TraceComputer::LockedCopyTo(TracePointVector &v, unsigned min_time,
                            const GeoPoint &location,
                            fixed resolution) const
{
  mutex.Lock();
  full.GetTracePoints(v, min_time, location, resolution);
  mutex.Unlock();
}

void
TraceComputer::Update(const SETTINGS_COMPUTER &settings_computer,
                      const AircraftState &state)
{
  if (state.time < last_time) {
    Reset();
  } else if (state.time <= last_time)
    return;

  last_time = state.time;

  if (!state.flying)
    return;

  // either olc or basic trace requires trace_full
  if (settings_computer.task.enable_olc ||
      settings_computer.task.enable_trace) {
    mutex.Lock();
    full.append(state);
    mutex.Unlock();
  }

  // only olc requires trace_sprint
  if (settings_computer.task.enable_olc)
    sprint.append(state);
}

void
TraceComputer::Idle(const SETTINGS_COMPUTER &settings_computer,
                    const AircraftState &state)
{
  if (!state.flying)
    return;

  if (settings_computer.task.enable_olc ||
      settings_computer.task.enable_trace) {
    mutex.Lock();
    full.optimise_if_old();
    mutex.Unlock();
  }

  if (settings_computer.task.enable_olc)
    sprint.optimise_if_old();
}
