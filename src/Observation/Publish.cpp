// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Publish.hpp"
#include "Database.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/ThermalLocator.hpp"

#include <chrono>

void
PublishOwnObservations(ObservationDatabase &db,
                       const DerivedInfo &calculated) noexcept
{
  const auto now = std::chrono::steady_clock::now();

  ThermalRows existing;
  db.CopyThermals(existing);

  ThermalRows next;
  next.clear();
  for (const auto &source : calculated.thermal_locator.sources) {
    if (next.full())
      break;
    if (!source.location.IsValid())
      continue;

    ObservedThermal row;
    row.origin = ObservationOrigin::OWN;
    row.location = source.location;
    row.ground_height = source.ground_height;
    row.lift_rate = source.lift_rate;
    row.received = now;

    for (const auto &old : existing) {
      if (old.origin == ObservationOrigin::OWN &&
          old.location.IsValid() &&
          old.location.DistanceS(source.location) <=
            OWN_THERMAL_MATCH_RANGE_M) {
        row.received = old.received;
        break;
      }
    }

    next.append(row);
  }
  db.ReplaceThermals(ObservationOrigin::OWN, next);

  WaveRows waves;
  waves.clear();
  for (const auto &wave : calculated.wave.waves) {
    if (!wave.IsDefined() || waves.full())
      continue;

    ObservedWave row;
    row.origin = ObservationOrigin::OWN;
    row.a = wave.a;
    row.b = wave.b;
    row.received = now;
    waves.append(row);
  }
  db.ReplaceWaves(ObservationOrigin::OWN, waves);
}

void
CollectVisibleThermals(std::span<const ObservedThermal> in,
                       ThermalRows &out,
                       double aircraft_altitude,
                       const SpeedVector &wind,
                       bool show_cloud, bool show_tim) noexcept
{
  out.clear();

  for (const auto &row : in) {
    if (!OriginShown(row.origin, show_cloud, show_tim))
      continue;

    if (row.origin == ObservationOrigin::OWN &&
        aircraft_altitude < row.ground_height)
      continue;

    if (!row.location.IsValid() || out.full())
      continue;

    ObservedThermal drawn = row;
    if (row.origin == ObservationOrigin::OWN &&
        wind.IsNonZero() && row.lift_rate > 0) {
      ThermalSource source;
      source.location = row.location;
      source.ground_height = row.ground_height;
      source.lift_rate = row.lift_rate;
      source.time = TimeStamp::Undefined();
      drawn.location = source.CalculateAdjustedLocation(aircraft_altitude,
                                                        wind);
    }

    if (drawn.location.IsValid())
      out.append(drawn);
  }
}
