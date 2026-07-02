// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TraceComputer.hpp"
#include "Settings.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Geo/GeoBounds.hpp"

#include <cmath>

static constexpr unsigned contest_trace_size = 256;
static constexpr unsigned sprint_trace_size = 128;

static constexpr auto full_trace_no_thin_time = std::chrono::minutes{2};

/** Minimum |Δvario| to store another merge-vario sample (m/s). */
static constexpr float MERGE_VARIO_DEDUPE_EPS = 0.05f;

/** Near-zero band: always keep samples for Nullschieber colouring. */
static constexpr float MERGE_VARIO_NULL_BAND = 0.05f;

[[gnu::const]]
static bool
MergeVarioSampleWorthKeeping(float prev, float next) noexcept
{
  if (std::abs(next - prev) >= MERGE_VARIO_DEDUPE_EPS)
    return true;

  if ((prev > 0.f) != (next > 0.f))
    return true;

  if (std::abs(next) < MERGE_VARIO_NULL_BAND)
    return true;

  return false;
}

static void
PushMergeVarioDeduped(std::vector<TrailVarioSample> &dest,
                      const TrailVarioSample &sample) noexcept
{
  if (!dest.empty() &&
      !MergeVarioSampleWorthKeeping(dest.back().vario, sample.vario))
    return;

  dest.push_back(sample);
}

TraceComputer::TraceComputer()
 :full(full_trace_no_thin_time, Trace::null_time, FULL_TRACE_MAX_POINTS),
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
    merge_vario_archive.clear();
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
TraceComputer::ArchiveMergeVarioForLegUnlocked(TracePoint::Time t0,
                                               TracePoint::Time t1) noexcept
{
  if (!(t1 > t0))
    return;

  for (const auto &s : merge_vario_samples) {
    if (s.time < t0 || s.time >= t1)
      continue;

    PushMergeVarioDeduped(merge_vario_archive, s);
  }

  const size_t max_archive_size = FULL_TRACE_MAX_POINTS;
  if (merge_vario_archive.size() > max_archive_size) {
    const size_t excess = merge_vario_archive.size() - max_archive_size;
    merge_vario_archive.erase(merge_vario_archive.begin(),
                              merge_vario_archive.begin() + excess);
  }
}

void
TraceComputer::CopyMergeVarioSamplesUnlocked(
    std::vector<TrailVarioSample> &vario_samples,
    TracePoint::Time min_time) const
{
  vario_samples.clear();

  const auto min_count = merge_vario_archive.size() + MERGE_VARIO_SAMPLES_CAPACITY;
  vario_samples.reserve(min_count);

  for (const auto &s : merge_vario_archive) {
    if (s.time >= min_time)
      vario_samples.push_back(s);
  }

  const TracePoint::Time after_archive =
    merge_vario_archive.empty()
      ? min_time
      : std::max(min_time, merge_vario_archive.back().time);

  for (const auto &s : merge_vario_samples) {
    if (s.time < after_archive)
      continue;

    bool duplicate = false;
    for (auto it = vario_samples.rbegin();
         it != vario_samples.rend() && it->time == s.time; ++it) {
      if (it->vario == s.vario) {
        duplicate = true;
        break;
      }
    }

    if (!duplicate)
      vario_samples.push_back(s);
  }
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
  CopyMergeVarioSamplesUnlocked(vario_samples, min_time);
}

void
TraceComputer::LockedTrailQuery(const TrailQuery &query,
                                TracePointVector &v,
                                std::vector<TrailVarioSample> &vario_samples,
                                Serial *append_serial,
                                Serial *modify_serial) const
{
  const std::lock_guard lock{mutex};

  if (query.bounds.IsValid())
    full.GetPoints(v, query.min_time, query.bounds,
                   query.project_location, query.min_distance_m);
  else
    full.GetPoints(v, query.min_time, query.project_location,
                   query.min_distance_m);

  CopyMergeVarioSamplesUnlocked(vario_samples, query.min_time);

  if (append_serial != nullptr)
    *append_serial = full.GetAppendSerial();
  if (modify_serial != nullptr)
    *modify_serial = full.GetModifySerial();
}

void
TraceComputer::PushMergeVarioSample(TracePoint::Time time, float vario) noexcept
{
  const std::lock_guard lock{mutex};
  /* Drop stale samples after replay / backward clock jumps; keep samples at
     equal timestamps (several merge ticks per GPS second). */
  if (!merge_vario_samples.empty()) {
    const TracePoint::Time newest = merge_vario_samples.last().time;
    if (time < newest) {
      merge_vario_samples.clear();
      merge_vario_archive.clear();
    } else {
      const float last_vario = merge_vario_samples.last().vario;
      if (!MergeVarioSampleWorthKeeping(last_vario, vario))
        return;
    }
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
    const bool had_previous = !full.empty();
    const TracePoint::Time leg_start =
      had_previous ? full.back().GetTime() : TracePoint::Time{};

    full.push_back(point);

    if (had_previous)
      ArchiveMergeVarioForLegUnlocked(leg_start, point.GetTime());
  }

  // only contest requires trace_sprint
  if (settings_computer.contest.enable) {
    sprint.push_back(point);
    contest.push_back(point);
  }
}
