// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RangeEstimate.hpp"
#include "NMEA/Info.hpp"
#include "Math/Angle.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/LineReader.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>

void
FlarmRangeEstimate::Sector::Add(unsigned distance) noexcept
{
  ++count;
  maximum = std::max<uint32_t>(maximum, distance);
  ++histogram[std::min(distance / BIN_WIDTH, BINS - 1)];
}

std::optional<unsigned>
FlarmRangeEstimate::Sector::Percentile(double p) const noexcept
{
  if (count == 0)
    return std::nullopt;

  const double wanted = p * count;
  uint32_t sum = 0;
  for (unsigned i = 0; i < BINS; ++i) {
    sum += histogram[i];
    if (sum >= wanted)
      /* the upper edge of the bin, but never beyond what was
         actually received */
      return std::min<unsigned>((i + 1) * BIN_WIDTH, maximum);
  }

  return maximum;
}

unsigned
FlarmRangeEstimate::SectorIndex(Angle relative_bearing) noexcept
{
  const double fraction =
    relative_bearing.AsBearing().Degrees() / 360.;
  return std::min(unsigned(fraction * SECTORS), SECTORS - 1);
}

uint32_t
FlarmRangeEstimate::GetCount() const noexcept
{
  uint32_t count = 0;
  for (const auto &sector : sectors)
    count += sector.count;
  return count;
}

void
FlarmRangeEstimate::Add(Angle relative_bearing, unsigned distance,
                        std::optional<TimePoint> time) noexcept
{
  sectors[SectorIndex(relative_bearing)].Add(distance);

  if (time) {
    if (!first || *time < *first)
      first = time;
    if (!last || *time > *last)
      last = time;
  }
}

void
FlarmRangeEstimator::Process(FlarmRangeEstimate &estimate,
                             const NMEAInfo &basic, bool flying) noexcept
{
  const auto &list = basic.flarm.traffic.list;

  /* forget the targets that have left the list */
  for (unsigned i = 0; i < seen.size();) {
    const FlarmId id = seen[i].id;
    if (std::none_of(list.begin(), list.end(),
                     [id](const FlarmTraffic &t){ return t.id == id; }))
      seen.quick_remove(i);
    else
      ++i;
  }

  const bool counting = flying && basic.track_available;

  for (const FlarmTraffic &traffic : list) {
    /* only what the FLARM's own radio received: not ADS-B and not
       traffic XCSoar adds from the internet; and only targets that
       may be tracked */
    if (!traffic.IsDefined() ||
        traffic.source != FlarmTraffic::SourceType::FLARM ||
        traffic.stealth || traffic.no_track)
      continue;

    /* without the PFLAA Source field (before protocol version 9), an
       ADS-B target looks like a FLARM one; but it always has an ICAO
       address, while a FLARM radio usually sends its own ID */
    if (!traffic.source_received &&
        traffic.id_type == FlarmTraffic::IdType::ICAO)
      continue;

    /* a target beyond the reach of the relative position fields;
       PFLAA clamps them to +/-32767 m */
    constexpr double CLAMPED = 32767;
    if (std::fabs(traffic.relative_north) >= CLAMPED ||
        std::fabs(traffic.relative_east) >= CLAMPED)
      continue;

    auto i = std::find_if(seen.begin(), seen.end(),
                          [&traffic](const Seen &s){
                            return s.id == traffic.id;
                          });
    if (i == seen.end()) {
      if (seen.full())
        continue;

      seen.append({traffic.id, traffic.valid});
    } else if (traffic.valid.Modified(i->valid))
      i->valid = traffic.valid;
    else
      /* this report has been counted already */
      continue;

    if (!counting)
      continue;

    const double distance = std::hypot(traffic.relative_north,
                                       traffic.relative_east);
    if (distance <= 0)
      continue;

    const Angle bearing = Angle::FromXY(traffic.relative_north,
                                        traffic.relative_east);

    std::optional<FlarmRangeEstimate::TimePoint> time;
    if (basic.time_available && basic.date_time_utc.IsPlausible())
      time = basic.date_time_utc.ToTimePoint();

    estimate.Add(bearing - basic.track, unsigned(std::lround(distance)),
                 time);
  }
}

static std::chrono::seconds::rep
ToUnix(std::chrono::system_clock::time_point t) noexcept
{
  return std::chrono::duration_cast<std::chrono::seconds>(
    t.time_since_epoch()).count();
}

void
SaveFlarmRangeEstimate(const FlarmRangeEstimate &estimate,
                       BufferedOutputStream &os)
{
  os.Write("# XCSoar FLARM range estimate\n");

  if (estimate.first && estimate.last)
    os.Write(fmt::format("period {} {}\n",
                         ToUnix(*estimate.first), ToUnix(*estimate.last)));

  for (unsigned i = 0; i < FlarmRangeEstimate::SECTORS; ++i) {
    const auto &sector = estimate.sectors[i];
    if (sector.count == 0)
      continue;

    os.Write(fmt::format("sector {} {} {}", i, sector.count,
                         sector.maximum));
    for (const auto n : sector.histogram)
      os.Write(fmt::format(" {}", n));
    os.Write('\n');
  }
}

/**
 * Parse the next unsigned number, moving @p p past it.
 */
static std::optional<uint64_t>
NextNumber(const char *&p) noexcept
{
  char *end;
  const auto value = ParseUint64(p, &end);
  if (end == p)
    return std::nullopt;

  p = end;
  return value;
}

static void
LoadSector(FlarmRangeEstimate &estimate, const char *p) noexcept
{
  constexpr uint64_t MAX = UINT32_MAX;

  const auto i = NextNumber(p);
  const auto count = NextNumber(p);
  const auto maximum = NextNumber(p);
  if (!i || *i >= FlarmRangeEstimate::SECTORS || !count ||
      !maximum || *maximum > MAX)
    return;

  FlarmRangeEstimate::Sector sector;
  sector.maximum = *maximum;

  /* the count is the sum of the histogram; take that rather than a
     stored number that may not match it */
  uint64_t sum = 0;
  for (auto &n : sector.histogram) {
    const auto value = NextNumber(p);
    if (!value || *value > MAX)
      return;
    n = *value;
    sum += *value;
  }

  if (sum > MAX)
    return;

  sector.count = sum;
  estimate.sectors[*i] = sector;
}

void
LoadFlarmRangeEstimate(FlarmRangeEstimate &estimate, NLineReader &reader)
{
  estimate.Clear();

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (const char *p = StringAfterPrefix(line, "sector ")) {
      LoadSector(estimate, p);
    } else if (const char *q = StringAfterPrefix(line, "period ")) {
      const auto first = NextNumber(q);
      const auto last = NextNumber(q);
      if (first && last) {
        estimate.first = std::chrono::system_clock::time_point{
          std::chrono::seconds(*first)};
        estimate.last = std::chrono::system_clock::time_point{
          std::chrono::seconds(*last)};
      }
    }
  }
}
