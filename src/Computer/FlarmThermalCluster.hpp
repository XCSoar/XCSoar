// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlarmThermalCandidate.hpp"

#include <optional>
#include <span>

namespace FlarmThermal {

struct Contributor {
  FlarmId id;
  GeoPoint centre;
  ThermalSource source;
  TimeStamp first_seen;
  TimeStamp last_seen;
  TimeStamp last_value_time;
  double latest_climb_rate;
  double last_climb_rate;
  double climb_integral;
  double encounter_duration;
  double encounter_average;
  double min_altitude;
  double max_altitude;
  double reference_altitude;
  double geometry_lift_rate;
  SpeedVector geometry_wind;
  SpeedVector drift_per_meter;
  bool active;
};

/**
 * Identity-free geometry used for detector-side grouping and merging.
 *
 * This deliberately mirrors only the published geometry needed by the
 * detector. TrafficThermalInfo remains an output snapshot.
 */
struct ClusterGeometry {
  GeoPoint reference_location;
  double reference_altitude;
  double lift_rate;
  SpeedVector wind;
  SpeedVector drift_per_meter;
  double ground_height;
  double max_observed_altitude;
};

struct ClusterView {
  const ClusterGeometry &geometry;
  TimeStamp first_seen;
  TimeStamp last_seen;
  bool closed;
};

struct ClusterAggregate {
  ClusterGeometry geometry;
  unsigned active_count;
  double reporting_lift_rate;
  double min_altitude;
  double max_altitude;
};

/**
 * Return the projected distance when the candidate may join the cluster.
 */
[[gnu::pure]]
std::optional<double>
GetClusterCompatibilityDistance(const Candidate &candidate,
                                const ClusterView &cluster,
                                TimeStamp now) noexcept;

[[gnu::pure]]
bool AreClustersCompatible(const ClusterView &a,
                           const ClusterView &b) noexcept;

[[gnu::const]]
bool IsFirstClusterPreferred(TimeStamp a_first, std::uint32_t a_serial,
                             TimeStamp b_first,
                             std::uint32_t b_serial) noexcept;

[[gnu::pure]]
Contributor MergeContributors(Contributor existing,
                              const Contributor &incoming) noexcept;

[[gnu::pure]]
ClusterAggregate
CalculateClusterAggregate(std::span<const Contributor> contributors,
                          double reference_altitude) noexcept;

} // namespace FlarmThermal
