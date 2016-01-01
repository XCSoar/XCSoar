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

#include "TraceComputer.hpp"
#include "Settings.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Asset.hpp"

static constexpr unsigned full_trace_size =
  HasLittleMemory() ? 512 : 1024;

static constexpr unsigned contest_trace_size =
  HasLittleMemory() ? 128 : 256;

static constexpr unsigned sprint_trace_size =
  IsAncientHardware() ? 96 : 128;

static constexpr unsigned full_trace_no_thin_time =
  HasLittleMemory() ? 60 : 120;

TraceComputer::TraceComputer()
 :full(full_trace_no_thin_time, Trace::null_time, full_trace_size),
  contest(0, Trace::null_time, contest_trace_size),
  sprint(0, 9000, sprint_trace_size)
{
}

void
TraceComputer::Reset()
{
  {
    const ScopeLock lock(mutex);
    full.clear();
  }

  contest.clear();
  sprint.clear();
}

void
TraceComputer::LockedCopyTo(TracePointVector &v) const
{
  const ScopeLock lock(mutex);
  full.GetPoints(v);
}

void
TraceComputer::LockedCopyTo(TracePointVector &v, unsigned min_time,
                            const GeoPoint &location,
                            double resolution) const
{
  const ScopeLock lock(mutex);
  full.GetPoints(v, min_time, location, resolution);
}

void
TraceComputer::Update(const ComputerSettings &settings_computer,
                      const MoreData &basic, const DerivedInfo &calculated)
{
  /* time warps are handled by the Trace class */

  if (!basic.time_available || !basic.location_available ||
      !basic.NavAltitudeAvailable() ||
      !calculated.flight.flying)
    return;

  const TracePoint point(basic);

  {
    const ScopeLock lock(mutex);
    full.push_back(point);
  }

  // only olc requires trace_sprint
  if (settings_computer.contest.enable) {
    sprint.push_back(point);
    contest.push_back(point);
  }
}
