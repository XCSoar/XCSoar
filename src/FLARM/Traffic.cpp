/*
Copyright_License {

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

#include "FLARM/Traffic.hpp"

const TCHAR* acTypes[16] = { _T("Unknown"), _T("Glider"), _T("TowPlane"),
    _T("Helicopter"), _T("Parachute"), _T("DropPlane"), _T("HangGlider"),
    _T("ParaGlider"), _T("PoweredAircraft"), _T("JetAircraft"),
    _T("FlyingSaucer"), _T("Balloon"), _T("Airship"), _T("UAV"),
    _T("Unknown"), _T("StaticObject") };

const TCHAR *
FLARM_TRAFFIC::GetTypeString(AircraftType Type)
{
  if (Type < 16)
    return acTypes[Type];

  return NULL;
}

void
FLARM_TRAFFIC::Update(const FLARM_TRAFFIC &other)
{
  AlarmLevel = other.AlarmLevel;
  RelativeNorth = other.RelativeNorth;
  RelativeEast = other.RelativeEast;
  RelativeAltitude = other.RelativeAltitude;
  IDType = other.IDType;
  TrackBearing = other.TrackBearing;
  track_received = other.track_received;
  TurnRate = other.TurnRate;
  turn_rate_received = other.turn_rate_received;
  Speed = other.Speed;
  speed_received = other.speed_received;
  ClimbRate = other.ClimbRate;
  climb_rate_received = other.climb_rate_received;
  Stealth = other.Stealth;
  Type = other.Type;
}
