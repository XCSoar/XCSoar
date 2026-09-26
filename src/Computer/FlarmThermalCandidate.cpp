// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmThermalCandidate.hpp"

#include "Computer/ThermalBase.hpp"
#include "FLARM/Calculations.hpp"
#include "Geo/Flat/FlatPoint.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "NMEA/ThermalProjection.hpp"

#include <algorithm>
#include <cmath>

using namespace FlarmThermalConstants;

namespace FlarmThermal {

bool
IsEligibleTraffic(const FlarmTraffic &traffic) noexcept
{
  // IsPassive() classifies unknown types as powered; retain its low-speed
  // exclusion while permitting physical FLARM traffic with an unknown type.
  if (!traffic.valid ||
      traffic.source != FlarmTraffic::SourceType::FLARM ||
      !traffic.id.IsDefined() ||
      traffic.id_type == FlarmTraffic::IdType::RANDOM ||
      traffic.no_track ||
      !traffic.location_available || !traffic.location.Check() ||
      !traffic.altitude_available ||
      !std::isfinite(double(traffic.altitude)) ||
      !std::isfinite(static_cast<Angle>(traffic.track).Native()) ||
      traffic.speed < 4)
    return false;

  return traffic.type == FlarmTraffic::AircraftType::UNKNOWN ||
    traffic.type == FlarmTraffic::AircraftType::GLIDER ||
    traffic.type == FlarmTraffic::AircraftType::HANG_GLIDER ||
    traffic.type == FlarmTraffic::AircraftType::PARA_GLIDER;
}

CandidateResult
BuildCandidate(std::span<const Sample> samples, FlarmId id,
               bool previously_qualified, const FlarmTraffic &traffic,
               double geometry_lift_rate,
               const SpeedVector &geometry_wind,
               const RasterTerrain *terrain,
               Candidate &candidate) noexcept
{
  if (samples.size() < 2 ||
      !traffic.climb_rate_avg30s_available ||
      traffic.climb_rate_avg30s_time_span <
        FlarmCalculations::AVERAGE_TIME ||
      !std::isfinite(traffic.climb_rate_avg30s))
    return CandidateResult::INCOMPLETE_WINDOW;

  const auto &oldest = samples.front();
  const auto &newest = samples.back();
  if (newest.time - oldest.time < FlarmCalculations::AVERAGE_TIME)
    return CandidateResult::INCOMPLETE_WINDOW;

  const double climb_threshold = previously_qualified
    ? EXIT_CLIMB_THRESHOLD
    : ENTER_CLIMB_THRESHOLD;
  if (traffic.climb_rate_avg30s < climb_threshold)
    return CandidateResult::WEAK_LIFT;

  double accumulated_turn = 0;
  for (unsigned i = 1; i < samples.size(); ++i)
    accumulated_turn +=
      (samples[i].track - samples[i - 1].track)
      .AsDelta().Absolute().Degrees();

  if (accumulated_turn < MIN_ACCUMULATED_TURN)
    return CandidateResult::INSUFFICIENT_TURN;

  if (previously_qualified) {
    unsigned tail_begin = samples.size() - 1;
    while (tail_begin > 0 &&
           samples[tail_begin - 1].time >=
             newest.time - EXIT_TURN_WINDOW)
      --tail_begin;

    double recent_turn = 0;
    for (unsigned i = tail_begin + 1; i < samples.size(); ++i)
      recent_turn +=
        (samples[i].track - samples[i - 1].track)
        .AsDelta().Absolute().Degrees();

    const double recent_duration =
      (newest.time - samples[tail_begin].time).count();
    const auto &previous = samples[samples.size() - 2];
    const double current_duration = (newest.time - previous.time).count();
    const double current_turn =
      (newest.track - previous.track).AsDelta().Absolute().Degrees();

    if (!(recent_duration > 0) ||
        recent_turn / recent_duration < MIN_RECENT_TURN_RATE ||
        !(current_duration > 0) ||
        current_turn / current_duration < MIN_CURRENT_TURN_RATE)
      return CandidateResult::LEFT_CIRCLE;
  }

  const auto drift_per_meter =
    CalculateThermalDriftPerMeter(geometry_wind, geometry_lift_rate);

  const FlatProjection projection(newest.location);
  FlatPoint mean(0, 0);
  for (const auto &sample : samples)
    mean += projection.ProjectFloat(
      ProjectThermalCore(sample.location,
                         newest.altitude - sample.altitude,
                         drift_per_meter));

  mean = mean * (1. / samples.size());

  const double flat_scale = projection.GetApproximateScale();
  for (const auto &sample : samples) {
    const auto point = projection.ProjectFloat(
      ProjectThermalCore(sample.location,
                         newest.altitude - sample.altitude,
                         drift_per_meter));
    if (point.Distance(mean) * flat_scale > MAX_DRIFT_CORRECTED_RADIUS)
      return CandidateResult::EXCESSIVE_RADIUS;
  }

  candidate.id = id;
  candidate.centre = projection.Unproject(mean);
  candidate.first_seen = oldest.time;
  candidate.altitude = newest.altitude;
  candidate.min_altitude = oldest.altitude;
  candidate.max_altitude = oldest.altitude;
  for (const auto &sample : samples) {
    candidate.min_altitude = std::min(candidate.min_altitude,
                                      sample.altitude);
    candidate.max_altitude = std::max(candidate.max_altitude,
                                      sample.altitude);
  }
  candidate.climb_rate = traffic.climb_rate_avg30s;
  candidate.geometry_lift_rate = geometry_lift_rate;
  candidate.geometry_wind = geometry_wind;
  candidate.drift_per_meter = drift_per_meter;
  candidate.source.lift_rate = geometry_lift_rate;
  candidate.source.time = newest.time;
  EstimateThermalBase(terrain, candidate.centre, candidate.altitude,
                      geometry_lift_rate, geometry_wind,
                      candidate.source.location,
                      candidate.source.ground_height);
  return candidate.source.location.IsValid()
    ? CandidateResult::QUALIFIED
    : CandidateResult::INVALID_SOURCE;
}

const char *
GetCandidateResultName(CandidateResult result) noexcept
{
  switch (result) {
  case CandidateResult::QUALIFIED:
    return "qualified";
  case CandidateResult::INCOMPLETE_WINDOW:
    return "incomplete-window";
  case CandidateResult::WEAK_LIFT:
    return "weak-lift";
  case CandidateResult::INSUFFICIENT_TURN:
    return "insufficient-turn";
  case CandidateResult::LEFT_CIRCLE:
    return "left-circle";
  case CandidateResult::EXCESSIVE_RADIUS:
    return "excessive-radius";
  case CandidateResult::INVALID_SOURCE:
    return "invalid-source";
  }

  return "unknown";
}

} // namespace FlarmThermal
