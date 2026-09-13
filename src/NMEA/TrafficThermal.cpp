// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficThermal.hpp"
#include "ThermalProjection.hpp"
#include "util/BoundedArray.hxx"

#include <tuple>

void
TrafficThermalSource::Clear() noexcept
{
  thermal.location = GeoPoint::Invalid();
  thermal.ground_height = 0;
  thermal.lift_rate = 0;
  thermal.time = TimeStamp::Undefined();

  reference_location = GeoPoint::Invalid();
  reference_altitude = 0;
  geometry_lift_rate = 0;
  geometry_wind = SpeedVector::Zero();
  drift_per_meter = SpeedVector::Zero();

  cluster_serial = 0;
  aircraft_count = 0;
  active_aircraft_count = 0;
  min_observed_altitude = 0;
  max_observed_altitude = 0;
  first_seen = TimeStamp::Undefined();
  last_seen = TimeStamp::Undefined();
  active = false;
}

GeoPoint
TrafficThermalSource::CalculateAdjustedLocation(double altitude) const noexcept
{
  if (!reference_location.IsValid())
    return GeoPoint::Invalid();

  return ProjectThermalCore(reference_location,
                            altitude - reference_altitude,
                            drift_per_meter);
}

void
TrafficThermalInfo::Clear() noexcept
{
  sources.clear();
}

TrafficThermalSource *
TrafficThermalInfo::FindBySerial(std::uint32_t serial) noexcept
{
  return BoundedArray::FindByKey(
    sources, serial,
    [](const TrafficThermalSource &source) noexcept {
      return source.cluster_serial;
    });
}

const TrafficThermalSource *
TrafficThermalInfo::FindBySerial(std::uint32_t serial) const noexcept
{
  return BoundedArray::FindByKey(
    sources, serial,
    [](const TrafficThermalSource &source) noexcept {
      return source.cluster_serial;
    });
}

bool
TrafficThermalInfo::RemoveBySerial(std::uint32_t serial) noexcept
{
  for (unsigned i = 0; i < sources.size(); ++i)
    if (sources[i].cluster_serial == serial) {
      sources.remove(i);
      return true;
    }

  return false;
}

TrafficThermalSource &
TrafficThermalInfo::AllocateSource(std::uint32_t serial) noexcept
{
  if (auto *source = FindBySerial(serial))
    return *source;

  auto allocation = BoundedArray::AppendOrReplaceOldest(
    sources,
    [](const TrafficThermalSource &source) noexcept {
      return std::tuple{source.first_seen, source.cluster_serial};
    });

  allocation.value.Clear();
  allocation.value.cluster_serial = serial;
  return allocation.value;
}
