// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmThermalCluster.hpp"

#include "Geo/Flat/FlatPoint.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "NMEA/ThermalProjection.hpp"

#include <algorithm>
#include <cassert>

using namespace FlarmThermalConstants;

namespace FlarmThermal {

std::optional<double>
GetClusterCompatibilityDistance(const Candidate &candidate,
                                const ClusterView &cluster,
                                TimeStamp now) noexcept
{
  if (cluster.closed || now < cluster.last_seen ||
      now > cluster.last_seen + GROUPING_TIME_GAP)
    return std::nullopt;

  const auto &geometry = cluster.geometry;
  if (!geometry.reference_location.IsValid() ||
      !(geometry.lift_rate > 0))
    return std::nullopt;

  const double comparison_altitude = std::max({
    candidate.altitude,
    candidate.source.ground_height,
    geometry.ground_height,
    geometry.max_observed_altitude,
    geometry.reference_altitude,
  });
  const auto candidate_location =
    ProjectThermalCore(candidate.centre,
                       comparison_altitude - candidate.altitude,
                       candidate.drift_per_meter);
  const auto cluster_location =
    ProjectThermalCore(geometry.reference_location,
                       comparison_altitude - geometry.reference_altitude,
                       geometry.drift_per_meter);
  const double distance = candidate_location.DistanceS(cluster_location);
  return distance <= GROUPING_RADIUS
    ? std::optional<double>{distance}
    : std::nullopt;
}

static bool
ClustersOverlapInTime(TimeStamp a_first, TimeStamp a_last,
                      TimeStamp b_first, TimeStamp b_last) noexcept
{
  if (a_last < b_first)
    return b_first - a_last <= GROUPING_TIME_GAP;

  if (b_last < a_first)
    return a_first - b_last <= GROUPING_TIME_GAP;

  return true;
}

bool
AreClustersCompatible(const ClusterView &a,
                      const ClusterView &b) noexcept
{
  if (a.closed || b.closed ||
      !ClustersOverlapInTime(a.first_seen, a.last_seen,
                             b.first_seen, b.last_seen))
    return false;

  if (!a.geometry.reference_location.IsValid() ||
      !(a.geometry.lift_rate > 0) ||
      !b.geometry.reference_location.IsValid() ||
      !(b.geometry.lift_rate > 0))
    return false;

  const double comparison_altitude = std::max({
    a.geometry.ground_height,
    b.geometry.ground_height,
    a.geometry.max_observed_altitude,
    b.geometry.max_observed_altitude,
    a.geometry.reference_altitude,
    b.geometry.reference_altitude,
  });
  const auto a_location = ProjectThermalCore(
    a.geometry.reference_location,
    comparison_altitude - a.geometry.reference_altitude,
    a.geometry.drift_per_meter);
  const auto b_location = ProjectThermalCore(
    b.geometry.reference_location,
    comparison_altitude - b.geometry.reference_altitude,
    b.geometry.drift_per_meter);
  return a_location.DistanceS(b_location) <= GROUPING_RADIUS;
}

bool
IsFirstClusterPreferred(TimeStamp a_first, std::uint32_t a_serial,
                        TimeStamp b_first, std::uint32_t b_serial) noexcept
{
  return a_first < b_first ||
    (a_first == b_first && a_serial < b_serial);
}

Contributor
MergeContributors(Contributor existing,
                  const Contributor &incoming) noexcept
{
  const double combined_climb_duration =
    existing.encounter_duration + incoming.encounter_duration;
  if (combined_climb_duration > 0) {
    existing.climb_integral += incoming.climb_integral;
    existing.encounter_duration = combined_climb_duration;
    existing.encounter_average =
      existing.climb_integral / combined_climb_duration;
  } else {
    existing.encounter_average =
      (existing.encounter_average + incoming.encounter_average) * 0.5;
  }

  existing.min_altitude = std::min(existing.min_altitude,
                                   incoming.min_altitude);
  existing.max_altitude = std::max(existing.max_altitude,
                                   incoming.max_altitude);

  existing.first_seen = std::min(existing.first_seen,
                                 incoming.first_seen);
  if (incoming.last_seen > existing.last_seen) {
    existing.last_seen = incoming.last_seen;
    existing.last_value_time = incoming.last_value_time;
    existing.latest_climb_rate = incoming.latest_climb_rate;
    existing.last_climb_rate = incoming.last_climb_rate;
    existing.centre = incoming.centre;
    existing.source = incoming.source;
    existing.reference_altitude = incoming.reference_altitude;
    existing.geometry_lift_rate = incoming.geometry_lift_rate;
    existing.geometry_wind = incoming.geometry_wind;
    existing.drift_per_meter = incoming.drift_per_meter;
  }
  existing.active = existing.active || incoming.active;
  return existing;
}

static void
AddVector(const SpeedVector &vector, double &east, double &north) noexcept
{
  east += vector.norm * vector.bearing.sin();
  north += vector.norm * vector.bearing.cos();
}

static SpeedVector
AverageVector(double east, double north, double count) noexcept
{
  return count > 0
    ? SpeedVector(east / count, north / count)
    : SpeedVector::Zero();
}

ClusterAggregate
CalculateClusterAggregate(std::span<const Contributor> contributors,
                          double reference_altitude) noexcept
{
  assert(!contributors.empty());

  ClusterAggregate result{};
  result.geometry.reference_altitude = reference_altitude;
  result.min_altitude = contributors.front().min_altitude;
  result.max_altitude = contributors.front().max_altitude;

  for (const auto &contributor : contributors)
    result.geometry.reference_altitude =
      std::max(result.geometry.reference_altitude,
               contributor.source.ground_height);

  const auto &first = contributors.front();
  const GeoPoint projection_centre =
    ProjectThermalCore(first.centre,
                       result.geometry.reference_altitude -
                         first.reference_altitude,
                       first.drift_per_meter);
  const FlatProjection projection(projection_centre);
  FlatPoint reference_location(0, 0);

  double active_lift = 0;
  double historical_lift = 0;
  double geometry_lift = 0;
  double wind_east = 0;
  double wind_north = 0;
  double drift_east = 0;
  double drift_north = 0;
  double ground_height = 0;

  for (const auto &contributor : contributors) {
    if (contributor.active) {
      ++result.active_count;
      active_lift += contributor.latest_climb_rate;
    }

    historical_lift += contributor.encounter_average;
    result.min_altitude = std::min(result.min_altitude,
                                   contributor.min_altitude);
    result.max_altitude = std::max(result.max_altitude,
                                   contributor.max_altitude);
    geometry_lift += contributor.geometry_lift_rate;
    AddVector(contributor.geometry_wind, wind_east, wind_north);
    AddVector(contributor.drift_per_meter, drift_east, drift_north);
    reference_location += projection.ProjectFloat(
      ProjectThermalCore(
        contributor.centre,
        result.geometry.reference_altitude -
          contributor.reference_altitude,
        contributor.drift_per_meter));
    ground_height += contributor.source.ground_height;
  }

  const double count = contributors.size();
  result.geometry.reference_location =
    projection.Unproject(reference_location * (1. / count));
  result.geometry.lift_rate = geometry_lift / count;
  result.geometry.wind = AverageVector(wind_east, wind_north, count);
  result.geometry.drift_per_meter =
    AverageVector(drift_east, drift_north, count);
  result.geometry.ground_height = ground_height / count;
  result.geometry.max_observed_altitude = result.max_altitude;
  result.reporting_lift_rate = result.active_count > 0
    ? active_lift / result.active_count
    : historical_lift / count;
  return result;
}

} // namespace FlarmThermal
