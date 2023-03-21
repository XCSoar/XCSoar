// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

static constexpr auto full_trace_no_thin_time =
  HasLittleMemory() ? std::chrono::minutes{1} : std::chrono::minutes{2};

TraceComputer::TraceComputer()
 :full(full_trace_no_thin_time, Trace::null_time, full_trace_size),
  contest({}, Trace::null_time, contest_trace_size),
  sprint({}, std::chrono::minutes{150}, sprint_trace_size)
{
}

void
TraceComputer::Reset()
{
  {
    const std::lock_guard lock{mutex};
    full.clear();
  }

  contest.clear();
  sprint.clear();
}

void
TraceComputer::LockedCopyTo(TracePointVector &v) const
{
  const std::lock_guard lock{mutex};
  full.GetPoints(v);
}

void
TraceComputer::LockedCopyTo(TracePointVector &v,
                            std::chrono::duration<unsigned> min_time,
                            const GeoPoint &location,
                            double resolution) const
{
  const std::lock_guard lock{mutex};
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
    const std::lock_guard lock{mutex};
    full.push_back(point);
  }

  // only contest requires trace_sprint
  if (settings_computer.contest.enable) {
    sprint.push_back(point);
    contest.push_back(point);
  }
}
