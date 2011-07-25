/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "AircraftSim.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

AircraftSim::AircraftSim()
{
}

void
AircraftSim::Stop()
{
  // nothing to do
}


void
AircraftSim::Start(const GeoPoint& location_start,
                   const GeoPoint& location_last,
                   const fixed& altitude)
{
  state.Reset();
  state.Location = location_start;
  state.NavAltitude = altitude;
  state.Time = fixed_zero;
  state.wind.norm = fixed_zero;
  state.wind.bearing = Angle();
  state.Speed = fixed(16);
  state_last = state;
  state_last.Location = location_last;

  // start with aircraft moving since this isn't a real replay (no time on ground)
  for (unsigned i=0; i<10; i++) {
    state.Moving(state.Time);
  }
}


GeoPoint
AircraftSim::endpoint(const Angle &heading, const fixed timestep) const
{
  GeoPoint ref = GeoVector(state.TrueAirspeed*timestep, heading).end_point(state.Location);
  return GeoVector(state.wind.norm*timestep,
                   state.wind.bearing+ Angle::degrees(fixed_180)).end_point(ref);
}

void
AircraftSim::integrate(const Angle& heading, const fixed timestep)
{
  GeoPoint v = endpoint(heading, timestep);
  state.track = state.Location.bearing(v);
  state.Speed = v.distance(state.Location)/timestep;
  state.Location = v;
  state.NavAltitude += state.vario*timestep;
  state.Time += timestep;
  state.Moving(state.Time);
}


void
AircraftSim::set_wind(const fixed speed, const Angle direction)
{
  state.wind.norm = speed;
  state.wind.bearing = direction;
}


bool
AircraftSim::Update(const Angle& heading, const fixed timestep)
{
  state_last = state;
  integrate(heading, timestep);
  return true;
}
