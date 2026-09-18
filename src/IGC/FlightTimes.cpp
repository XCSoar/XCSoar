// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightTimes.hpp"

#include "IGCParser.hpp"
#include "IGCExtensions.hpp"
#include "IGCFix.hpp"
#include "ApplyFixToNMEA.hpp"
#include "Computer/FlyingComputer.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/Info.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/Operation.hpp"
#include "io/FileLineReader.hpp"
#include "system/Path.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>

using std::chrono::seconds;

namespace {

static constexpr double IMPORT_TAKEOFF_SPEED = 10;
static constexpr int64_t DAY_SECONDS = 24 * 60 * 60;

struct AbsoluteFlightTimes {
  std::optional<int64_t> first_fix;
  std::optional<int64_t> last_fix;
  std::optional<int64_t> takeoff;
  std::optional<int64_t> landing;
};

static int64_t
ToIntegerSeconds(TimeStamp time) noexcept
{
  return std::chrono::duration_cast<seconds>(time.ToDuration()).count();
}

static BrokenTime
ToBrokenTime(int64_t value) noexcept
{
  value %= DAY_SECONDS;
  if (value < 0)
    value += DAY_SECONDS;

  return BrokenTime::FromSecondOfDay(static_cast<unsigned>(value));
}

static AbsoluteFlightTimes
Scan(Path path, OperationEnvironment *operation)
{
  FileLineReaderA reader(path);
  IGCExtensions extensions;
  extensions.clear();

  FlyingComputer flying_computer;
  flying_computer.Reset();

  FlyingState flying;
  flying.Reset();

  NMEAInfo basic;
  basic.Reset();

  DerivedInfo calculated;
  calculated.Reset();

  AbsoluteFlightTimes result;
  GeoPoint previous_location = GeoPoint::Invalid();
  std::optional<int64_t> previous_time;
  int64_t day_offset = 0;
  unsigned line_count = 0;

  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (operation != nullptr && (++line_count % 256) == 0 &&
        operation->IsCancelled())
      throw OperationCancelled{};

    if (line[0] == 'I') {
      IGCParseExtensions(line, extensions);
      continue;
    }

    if (line[0] != 'B')
      continue;

    IGCFix fix;
    if (!IGCParseFix(line, extensions, fix) || !fix.gps_valid)
      continue;

    const int64_t second_of_day = fix.time.GetSecondOfDay();
    int64_t absolute_time = day_offset + second_of_day;
    if (previous_time && absolute_time + DAY_SECONDS / 2 < *previous_time) {
      day_offset += DAY_SECONDS;
      absolute_time += DAY_SECONDS;
    }

    if (!result.first_fix)
      result.first_fix = absolute_time;

    if (previous_time) {
      const int64_t delta = absolute_time - *previous_time;
      if (delta <= 0 || delta >= DAY_SECONDS / 2)
        continue;
    }

    result.last_fix = absolute_time;

    std::optional<double> derived_ground_speed;
    if (fix.gsp < 0 && previous_time && previous_location.IsValid())
      derived_ground_speed = previous_location.DistanceS(fix.location) /
        static_cast<double>(absolute_time - *previous_time);

    const TimeStamp time{FloatDuration{static_cast<double>(absolute_time)}};
    ApplyIGCFixToNMEA(fix, time, derived_ground_speed, basic);

    const bool was_flying = flying.flying;
    flying_computer.Compute(IMPORT_TAKEOFF_SPEED, basic, calculated, flying);

    if (!was_flying && flying.flying) {
      if (!result.takeoff)
        result.takeoff = ToIntegerSeconds(flying.takeoff_time);

      /* A later takeoff invalidates the previous intermediate landing. */
      result.landing.reset();
    } else if (was_flying && !flying.flying) {
      result.landing = ToIntegerSeconds(flying.landing_time);
    }

    previous_time = absolute_time;
    previous_location = fix.location;
  }

  if (operation != nullptr && operation->IsCancelled())
    throw OperationCancelled{};

  if (previous_time) {
    const bool was_flying = flying.flying;
    flying_computer.Finish(
      flying, TimeStamp{FloatDuration{static_cast<double>(*previous_time)}});
    if (was_flying && !flying.flying)
      result.landing = ToIntegerSeconds(flying.landing_time);
  }

  return result;
}

} // namespace

IGCFlightTimes
DetectIGCFlightTimes(Path path, OperationEnvironment *operation)
{
  const auto detected = Scan(path, operation);

  IGCFlightTimes result;
  if (!detected.first_fix || !detected.last_fix)
    return result;

  auto takeoff = detected.takeoff;
  auto landing = detected.landing;

  result.takeoff_detected = takeoff.has_value();
  result.landing_detected = landing.has_value();

  if (!takeoff)
    takeoff = detected.first_fix;
  if (!landing)
    landing = detected.last_fix;

  result.has_valid_fixes = true;
  result.takeoff = ToBrokenTime(*takeoff);
  result.landing = ToBrokenTime(*landing);
  result.duration = seconds{std::max<int64_t>(0, *landing - *takeoff)};
  return result;
}
