// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "util/TrivialArray.hxx"

#include <cstdint>
#include <type_traits>

/** A calculated thermal marker contributed by physical FLARM traffic. */
struct TrafficThermalSource {
  /** Ground source and reported lift shown in the map-item details. */
  ThermalSource thermal;

  /**
   * Stable detected core at an explicit navigation-altitude (MSL) datum.
   * Marker adjustment starts here instead of reinterpreting #thermal with a
   * later reporting average or selected wind.
   */
  GeoPoint reference_location;
  double reference_altitude;

  /** Geometry parameters captured from contributors during qualification. */
  double geometry_lift_rate;
  SpeedVector geometry_wind;

  /** Horizontal drift per metre of altitude, averaged per contributor. */
  SpeedVector drift_per_meter;

  /**
   * Opaque key used to update this bounded published marker.  This is a
   * cluster generation, not a contributor or aircraft identity.
   */
  std::uint32_t cluster_serial;

  /** Number of unique aircraft contributing to this encounter. */
  unsigned aircraft_count;

  /** Number of contributors that are currently qualified and active. */
  unsigned active_aircraft_count;

  /** Complete observed navigation-altitude (MSL) range of the encounter. */
  double min_observed_altitude;
  double max_observed_altitude;

  /** First and most recent observation in the cluster encounter. */
  TimeStamp first_seen;
  TimeStamp last_seen;

  /** True while at least one contributor is current. */
  bool active;

  void Clear() noexcept;

  /** Project the retained core to another navigation-altitude (MSL) level. */
  [[nodiscard]] [[gnu::pure]]
  GeoPoint CalculateAdjustedLocation(double altitude) const noexcept;
};

static_assert(std::is_trivially_copyable_v<TrafficThermalSource>,
              "type is not trivially copyable");

/**
 * Bounded, identity-free blackboard output for aggregated FLARM thermal
 * markers.  Per-aircraft state remains private to FlarmThermalComputer.
 */
struct TrafficThermalInfo {
  static constexpr unsigned MAX_SOURCES = 20;

  TrivialArray<TrafficThermalSource, MAX_SOURCES> sources;

  void Clear() noexcept;

  /** Find a published source by its stable calculation serial. */
  [[nodiscard]]
  TrafficThermalSource *FindBySerial(std::uint32_t serial) noexcept;

  [[nodiscard]]
  const TrafficThermalSource *FindBySerial(std::uint32_t serial) const noexcept;

  /** Remove a published source, preserving the order of the others. */
  bool RemoveBySerial(std::uint32_t serial) noexcept;

  /**
   * Find an existing source or allocate a slot, replacing the oldest source
   * when the bounded history is full.
   */
  [[nodiscard]]
  TrafficThermalSource &AllocateSource(std::uint32_t serial) noexcept;
};

static_assert(std::is_trivially_copyable_v<TrafficThermalInfo>,
              "type is not trivially copyable");
