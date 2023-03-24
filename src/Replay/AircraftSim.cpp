// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AircraftSim.hpp"
#include "Geo/GeoVector.hpp"

void
AircraftSim::Start(const GeoPoint& location_start,
                   const GeoPoint& location_last,
                   double altitude)
{
  state.Reset();
  state.location = location_start;
  state.altitude = altitude;
  state.time = {};
  state.wind.norm = 0;
  state.wind.bearing = Angle();
  state.ground_speed = 16;
  state_last = state;
  state_last.location = location_last;

  // start with aircraft moving since this isn't a real replay (no time on ground)
  state.flying = true;
}


GeoPoint
AircraftSim::GetEndPoint(const Angle heading,
                         const FloatDuration timestep) const noexcept
{
  GeoPoint ref = GeoVector(state.true_airspeed * timestep.count(), heading)
    .EndPoint(state.location);
  return GeoVector(state.wind.norm * timestep.count(),
                   state.wind.bearing + Angle::HalfCircle()).EndPoint(ref);
}

void
AircraftSim::Integrate(const Angle heading,
                       const FloatDuration timestep) noexcept
{
  GeoPoint v = GetEndPoint(heading, timestep);
  state.track = state.location.Bearing(v);
  state.ground_speed = v.DistanceS(state.location) / timestep.count();
  state.location = v;
  state.altitude += state.vario * timestep.count();
  state.time += timestep;
}


void
AircraftSim::SetWind(const double speed, const Angle direction)
{
  state.wind.norm = speed;
  state.wind.bearing = direction;
}


bool
AircraftSim::Update(const Angle heading,
                    const FloatDuration timestep) noexcept
{
  state_last = state;
  Integrate(heading, timestep);
  return true;
}
