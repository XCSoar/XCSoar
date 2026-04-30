// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TraceComputer.hpp"
#include "Settings.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

static constexpr unsigned full_trace_size = 1024;
static constexpr unsigned contest_trace_size = 256;
static constexpr unsigned sprint_trace_size = 128;

static constexpr auto full_trace_no_thin_time = std::chrono::minutes{2};

TraceComputer::TraceComputer()
 :full(full_trace_no_thin_time, Trace::null_time, full_trace_size),
  contest({}, Trace::null_time, contest_trace_size),
  sprint({}, std::chrono::minutes{120}, sprint_trace_size)
{
}

void
TraceComputer::Reset()
{
  {
    const std::lock_guard lock{mutex};
    full.clear();
    merge_vario_samples.clear();
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
TraceComputer::CopyMergeVarioSamplesUnlocked(
    std::vector<TrailVarioSample> &vario_samples) const
{
  vario_samples.clear();
  constexpr unsigned max_items = MERGE_VARIO_SAMPLES_CAPACITY - 1;
  vario_samples.reserve(max_items);
  for (const auto &s : merge_vario_samples)
    vario_samples.push_back(s);
}

void
TraceComputer::LockedCopySnapshot(TracePointVector &v,
                                  std::vector<TrailVarioSample> &vario_samples) const
{
  const std::lock_guard lock{mutex};
  full.GetPoints(v);
  CopyMergeVarioSamplesUnlocked(vario_samples);
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
TraceComputer::LockedCopySnapshot(TracePointVector &v,
                                   std::vector<TrailVarioSample> &vario_samples,
                                   std::chrono::duration<unsigned> min_time,
                                   const GeoPoint &location,
                                   double resolution) const
{
  const std::lock_guard lock{mutex};
  full.GetPoints(v, min_time, location, resolution);
  CopyMergeVarioSamplesUnlocked(vario_samples);
}

void
TraceComputer::PushMergeVarioSample(TracePoint::Time time, float vario) noexcept
{
  const std::lock_guard lock{mutex};
  /* Drop stale samples after replay / backward clock jumps; keep samples at
     equal timestamps (several merge ticks per GPS second). */
  if (!merge_vario_samples.empty()) {
    const TracePoint::Time newest = merge_vario_samples.last().time;
    if (time < newest)
      merge_vario_samples.clear();
  }
  merge_vario_samples.push({time, vario});
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
