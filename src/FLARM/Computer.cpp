// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer.hpp"
#include "Details.hpp"
#include "NMEA/Info.hpp"
#include "Geo/GeoVector.hpp"
#include "time/Cast.hxx"

static constexpr FlarmTraffic::Average30sUpdate
ToAverage30sUpdate(ClimbSampleAction action) noexcept
{
  switch (action) {
  case ClimbSampleAction::IGNORED:
    return FlarmTraffic::Average30sUpdate::IGNORED;

  case ClimbSampleAction::APPENDED:
    return FlarmTraffic::Average30sUpdate::APPENDED;

  case ClimbSampleAction::REPLACED:
    return FlarmTraffic::Average30sUpdate::REPLACED;
  }

  return FlarmTraffic::Average30sUpdate::NONE;
}

void
FlarmComputer::Process(FlarmData &flarm, const FlarmData &last_flarm,
                       const NMEAInfo &basic) noexcept
{
  const TimeStamp now = basic.time_available ? basic.time : basic.clock;

  // Cleanup old calculation instances
  flarm_calculations.CleanUp(now);

  // if (FLARM data is available)
  if (!flarm.IsDetected()) {
    flarm_calculations.Clear();
    return;
  }

  flarm_calculations.ResetMissing(flarm.traffic);

  double north_to_latitude(0);
  double east_to_longitude(0);

  if (basic.location_available) {
    // Precalculate relative east and north projection to lat/lon
    // for Location calculations of each target
    constexpr Angle delta_lat = Angle::Degrees(0.01);
    constexpr Angle delta_lon = Angle::Degrees(0.01);

    GeoPoint plat = basic.location;
    plat.latitude += delta_lat;
    GeoPoint plon = basic.location;
    plon.longitude += delta_lon;

    double dlat = basic.location.DistanceS(plat);
    double dlon = basic.location.DistanceS(plon);

    if (fabs(dlat) > 0 && fabs(dlon) > 0) {
      north_to_latitude = delta_lat.Degrees() / dlat;
      east_to_longitude = delta_lon.Degrees() / dlon;
    }
  }

  // for each item in traffic
  for (auto &traffic : flarm.traffic.list) {
    const auto ownship_altitude = basic.GetAnyAltitude();
    const FlarmTraffic *last_traffic =
      last_flarm.traffic.FindTraffic(traffic.id);

    // Keep the cached display name (callsign) in sync with current sources.
    // Skip for no_track targets and random IDs: they must not be resolved
    // against databases (FTD-012 NoTrack / random ID semantics).
    if (!traffic.no_track &&
        traffic.id.IsDefined() &&
        traffic.id_type != FlarmTraffic::IdType::RANDOM) {
      const char *fname = FlarmDetails::LookupCallsign(traffic.id);
      if (fname != nullptr &&
          (!traffic.HasName() || !traffic.name.equals(fname)))
        traffic.name = fname;
    }

    if (traffic.absolute_location && traffic.location.IsValid() &&
        basic.location_available) {
      const GeoVector vec{basic.location, traffic.location};
      traffic.relative_north = vec.distance * vec.bearing.cos();
      traffic.relative_east = vec.distance * vec.bearing.sin();
    }

    // Calculate distance
    traffic.distance = hypot(traffic.relative_north, traffic.relative_east);

    // Calculate Location
    traffic.location_available = traffic.absolute_location
      ? traffic.location.IsValid()
      : basic.location_available;

    if (!traffic.absolute_location && traffic.location_available) {
      traffic.location.latitude =
          Angle::Degrees(traffic.relative_north * north_to_latitude) +
          basic.location.latitude;

      traffic.location.longitude =
          Angle::Degrees(traffic.relative_east * east_to_longitude) +
          basic.location.longitude;
    }

    // Calculate absolute altitude (same ownship preference as
    // NMEAInfo::GetAnyAltitude / online FillRelative)
    if (!traffic.absolute_altitude) {
      traffic.altitude_available = ownship_altitude.has_value();
      if (traffic.altitude_available)
        traffic.altitude = traffic.relative_altitude +
          RoughAltitude(*ownship_altitude);
    } else if (ownship_altitude && traffic.altitude_available) {
      traffic.relative_altitude =
        traffic.altitude - RoughAltitude(*ownship_altitude);
    }

    // Calculate average climb rate.  Do not advertise a 30-second average
    // until the retained history actually spans the complete window.
    traffic.climb_rate_avg30s_available = false;
    traffic.climb_rate_avg30s_update =
      FlarmTraffic::Average30sUpdate::NONE;
    traffic.climb_rate_avg30s_reset = false;
    traffic.climb_rate_avg30s_time_span = FloatDuration::zero();
    if (traffic.altitude_available) {
      if (last_traffic != nullptr &&
          traffic.valid == last_traffic->valid) {
        /* Keep the sampling event level-triggered until a newer target
           update replaces it.  The CalculationThread consumes a copied
           snapshot asynchronously and may not have seen the first merge
           pass which published this event. */
        traffic.climb_rate_avg30s_available =
          last_traffic->climb_rate_avg30s_available;
        traffic.climb_rate_avg30s = last_traffic->climb_rate_avg30s;
        traffic.climb_rate_avg30s_time_span =
          last_traffic->climb_rate_avg30s_time_span;
        traffic.climb_rate_avg30s_update =
          last_traffic->climb_rate_avg30s_update;
        traffic.climb_rate_avg30s_reset =
          last_traffic->climb_rate_avg30s_reset;
      } else {
        const auto average =
          flarm_calculations.Average30sWithSpan(traffic.id, now,
                                                traffic.altitude);
        traffic.climb_rate_avg30s = average.average;
        traffic.climb_rate_avg30s_time_span = average.time_span;
        traffic.climb_rate_avg30s_available =
          average.IsComplete(FlarmCalculations::AVERAGE_TIME);
        traffic.climb_rate_avg30s_update =
          ToAverage30sUpdate(average.sample_action);
        traffic.climb_rate_avg30s_reset = average.reset;
      }
    } else {
      flarm_calculations.Reset(traffic.id);
      traffic.climb_rate_avg30s = 0;
      traffic.climb_rate_avg30s_reset = true;
    }

    // The following calculations are only relevant for targets
    // where information is missing
    if (traffic.track_received && traffic.turn_rate_received &&
        traffic.speed_received && traffic.climb_rate_received)
      continue;

    // Check if the target has been seen before in the last seconds
    if (last_traffic == nullptr || !last_traffic->valid)
      continue;

    // Calculate the time difference between now and the last contact
    const auto dt = traffic.valid.GetTimeDifference(last_traffic->valid);
    if (dt.count() > 0) {
      // Calculate the immediate climb rate
      if (!traffic.climb_rate_received)
        traffic.climb_rate =
          (traffic.relative_altitude - last_traffic->relative_altitude) / ToFloatSeconds(dt);
    } else {
      // Since the time difference is zero (or negative)
      // we can just copy the old values
      if (!traffic.climb_rate_received)
        traffic.climb_rate = last_traffic->climb_rate;
    }

    if (dt.count() > 0 &&
        traffic.location_available &&
        last_traffic->location_available) {
      // Calculate the GeoVector between now and the last contact
      GeoVector vec = last_traffic->location.DistanceBearing(traffic.location);

      if (!traffic.track_received)
        traffic.track = vec.bearing;

      // Calculate the turn rate
      if (!traffic.turn_rate_received) {
        Angle turn_rate = traffic.track - last_traffic->track;
        traffic.turn_rate =
          turn_rate.AsDelta().Degrees() / ToFloatSeconds(dt);
      }

      // Calculate the speed [m/s]
      if (!traffic.speed_received)
        traffic.speed = vec.distance / ToFloatSeconds(dt);
    } else {
      // Since the time difference is zero (or negative)
      // we can just copy the old values
      if (!traffic.track_received)
        traffic.track = last_traffic->track;

      if (!traffic.turn_rate_received)
        traffic.turn_rate = last_traffic->turn_rate;

      if (!traffic.speed_received)
        traffic.speed = last_traffic->speed;
    }
  }
}
