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

#include "Device/Simulator.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "../Simulator.hpp"
#include "OS/Clock.hpp"
#include "Asset.hpp"
#include "Engine/Math/Earth.hpp"

#include <stdio.h>

/**
 * This function creates some simulated traffic for FLARM debugging
 * @param GPS_INFO Pointer to the NMEA_INFO struct
 */
void
Simulator::GenerateFLARMTraffic(NMEA_INFO &basic)
{
  static int i = 90;

  i++;
  if (i > 255)
    i = 0;

  if (i > 80)
    return;

  const Angle angle = Angle::degrees(fixed((i * 360) / 255)).as_bearing();

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  int h1;
  int n1;
  int e1;
  int t1;
  unsigned l;
  h1 = (angle.ifastsine()) / 7;
  n1 = (angle.ifastsine()) / 2 - 200;
  e1 = (angle.ifastcosine()) / 1.5;
  t1 = -angle.as_bearing().value_degrees();

  l = (i % 30 > 13 ? 0 : (i % 30 > 5 ? 2 : 1));
  int h2;
  int n2;
  int e2;
  int t2;
  Angle dangle = (angle + Angle::degrees(fixed(120))).as_bearing();
  Angle hangle = dangle.flipped().as_bearing();

  h2 = (angle.ifastcosine()) / 10;
  n2 = (dangle.ifastsine()) / 1.20 + 300;
  e2 = (dangle.ifastcosine()) + 500;
  t2 = hangle.value_degrees();

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  char t_laa1[50];
  sprintf(t_laa1, "$PFLAA,%d,%d,%d,%d,2,DDA85C,%d,0,35,0,1", l, n1, e1, h1, t1);
  char t_laa2[50];
  sprintf(t_laa2, "$PFLAA,0,%d,%d,%d,2,AA9146,%d,0,27,0,1", n2, e2, h2, t2);

  char t_lau[50];
  sprintf(t_lau, "$PFLAU,2,1,2,1,%d", l);

  AppendNMEAChecksum(t_laa1);
  AppendNMEAChecksum(t_laa2);
  AppendNMEAChecksum(t_lau);

  parser.ParseNMEAString_Internal(t_lau, &basic);
  parser.ParseNMEAString_Internal(t_laa1, &basic);
  parser.ParseNMEAString_Internal(t_laa2, &basic);
}

void
Simulator::Process(NMEA_INFO &basic)
{
  if (!is_simulator())
    return;

  basic.Connected.update(fixed(MonotonicClockMS()) / 1000);
  basic.gps.SatellitesUsed = 6;
  basic.gps.Simulator = true;
  basic.gps.real = false;
  basic.gps.MovementDetected = false;

#ifdef ANDROID
  basic.gps.AndroidInternalGPS = false;
#endif

  basic.Location = FindLatitudeLongitude(basic.Location, basic.TrackBearing,
                                         basic.GroundSpeed);
  basic.LocationAvailable.update(basic.Time);
  basic.GPSAltitudeAvailable.update(basic.Time);
  basic.TrackBearingAvailable.update(basic.Time);
  basic.GroundSpeedAvailable.update(basic.Time);

  basic.Time += fixed_one;
  long tsec = (long)basic.Time;
  basic.DateTime.hour = tsec / 3600;
  basic.DateTime.minute = (tsec - basic.DateTime.hour * 3600) / 60;
  basic.DateTime.second = tsec - basic.DateTime.hour * 3600
    - basic.DateTime.minute * 60;

  // use this to test FLARM parsing/display
  if (is_debug() && !is_altair())
    GenerateFLARMTraffic(basic);

  // clear Airspeed as it is not available in simulation mode
  basic.AirspeedAvailable.clear();
}
