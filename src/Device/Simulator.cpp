/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Device/Simulator.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "../Simulator.hpp"
#include "Asset.hpp"
#include "Geo/Math.hpp"

#include <stdio.h>

void
Simulator::Init(NMEAInfo &basic)
{
  /* just in case DeviceBlackboard::SetStartupLocation never gets
     called, set some dummy values that are better than uninitialised
     values */

  basic.location = GeoPoint::Zero();
  basic.track = Angle::Zero();
  basic.ground_speed = fixed(0);
  basic.gps_altitude = fixed(0);
}

/**
 * This function creates some simulated traffic for FLARM debugging
 * @param GPS_INFO Pointer to the NMEA_INFO struct
 */
void
Simulator::GenerateFLARMTraffic(NMEAInfo &basic)
{
  static int i = 90;

  i++;
  if (i > 255)
    i = 0;

  if (i > 80)
    return;

  const Angle angle = Angle::FullCircle() * i / 255;
  Angle dangle = (angle + Angle::Degrees(120)).AsBearing();

  int alt = (angle.ifastsine()) / 7;
  int north = (angle.ifastsine()) / 2 - 200;
  int east = (angle.ifastcosine()) / 1.5;
  int track = -(int)angle.AsBearing().Degrees();
  unsigned alarm_level = (i % 30 > 13 ? 0 : (i % 30 > 5 ? 2 : 1));

  NMEAParser parser(true);
  char buffer[50];

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  sprintf(buffer, "$PFLAA,%d,%d,%d,%d,2,DDA85C,%d,0,35,0,1",
          alarm_level, north, east, alt, track);
  parser.ParseLine(buffer, basic);

  alt = (angle.ifastcosine()) / 10;
  north = (dangle.ifastsine()) / 1.20 + 300;
  east = (dangle.ifastcosine()) + 500;

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  sprintf(buffer, "$PFLAA,0,%d,%d,%d,2,AA9146,,,,,1",
          north, east, alt);
  parser.ParseLine(buffer, basic);

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  sprintf(buffer, "$PFLAU,2,1,2,1,%d", alarm_level);
  parser.ParseLine(buffer, basic);
}

void
Simulator::Touch(NMEAInfo &basic)
{
  assert(is_simulator());

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.gps.simulator = true;
  basic.gps.real = false;

  basic.location_available.Update(basic.clock);
  basic.track_available.Update(basic.clock);
  basic.ground_speed_available.Update(basic.clock);
  basic.gps_altitude_available.Update(basic.clock);

  basic.time_available.Update(basic.clock);
  basic.time += fixed(1);
  basic.date_time_utc = basic.date_time_utc + 1;
}

void
Simulator::Process(NMEAInfo &basic)
{
  assert(is_simulator());

  Touch(basic);

  basic.location = FindLatitudeLongitude(basic.location, basic.track,
                                         basic.ground_speed);

  // use this to test FLARM parsing/display
  if (IsDebug() && !IsAltair())
    GenerateFLARMTraffic(basic);
}
