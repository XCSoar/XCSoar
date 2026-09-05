// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlarmThermalSettings.hpp"
#include "FLARM/Traffic.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/SpeedVector.hpp"
#include "NMEA/TrafficThermal.hpp"

#include <cstdint>
#include <span>

class RasterTerrain;

namespace FlarmThermal {

struct Sample {
  TimeStamp time;
  GeoPoint location;
  double altitude;
  Angle track;
  double climb_rate;
};

struct Candidate {
  FlarmId id;
  GeoPoint centre;
  ThermalSource source;
  TimeStamp first_seen;
  double altitude;
  double min_altitude;
  double max_altitude;
  double climb_rate;
  double geometry_lift_rate;
  SpeedVector geometry_wind;
  SpeedVector drift_per_meter;
};

enum class CandidateResult : std::uint8_t {
  QUALIFIED,
  INCOMPLETE_WINDOW,
  WEAK_LIFT,
  INSUFFICIENT_TURN,
  LEFT_CIRCLE,
  EXCESSIVE_RADIUS,
  INVALID_SOURCE,
};

[[gnu::pure]]
bool IsEligibleTraffic(const FlarmTraffic &traffic) noexcept;

CandidateResult BuildCandidate(std::span<const Sample> samples,
                               FlarmId id, bool previously_qualified,
                               const FlarmTraffic &traffic,
                               double geometry_lift_rate,
                               const SpeedVector &geometry_wind,
                               const RasterTerrain *terrain,
                               Candidate &candidate) noexcept;

[[gnu::const]]
const char *GetCandidateResultName(CandidateResult result) noexcept;

} // namespace FlarmThermal
