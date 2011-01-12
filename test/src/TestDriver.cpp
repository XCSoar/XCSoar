/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Device/Driver/Generic.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/ILEC.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Device/Geoid.h"
#include "Device/device.hpp"
#include "NMEA/Info.hpp"
#include "TestUtil.hpp"
#include "Protection.hpp"
#include "InputEvents.hpp"

#include <string.h>

static bool gps_updated, vario_updated;

FLYING_STATE::FLYING_STATE() {}

void TriggerGPSUpdate()
{
  gps_updated = true;
}

void TriggerVarioUpdate()
{
  vario_updated = true;
}

bool
HaveCondorDevice()
{
  return false;
}

static bool has_baro_source;

bool
devHasBaroSource()
{
  return has_baro_source;
}

size_t
Declaration::size() const
{
  return 0;
}

/*
 * Fake Device/Geoid.cpp
 */

fixed
LookupGeoidSeparation(const GeoPoint pt)
{
  return fixed_zero;
}

/*
 * Fake InputEvents.cpp
 */

bool
InputEvents::processGlideComputer(unsigned gce_id)
{
  return true;
}

/*
 * Unit tests
 */

static void
TestGeneric()
{
  NMEAParser parser;

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* no GPS reception */
  parser.ParseNMEAString_Internal("$GPRMC,082310,V,,,,,230610*3f",
                                  &nmea_info);
  ok1(nmea_info.gps.Connected == 2);
  ok1(nmea_info.gps.NAVWarning);
  ok1(nmea_info.DateTime.year == 2010);
  ok1(nmea_info.DateTime.month == 6);
  ok1(nmea_info.DateTime.day == 23);
  ok1(nmea_info.DateTime.hour == 8);
  ok1(nmea_info.DateTime.minute == 23);
  ok1(nmea_info.DateTime.second == 10);

  /* got a GPS fix */
  parser.ParseNMEAString_Internal("$GPRMC,082311,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*6C",
                                  &nmea_info);
  ok1(nmea_info.gps.Connected == 2);
  ok1(!nmea_info.gps.NAVWarning);
  ok1(nmea_info.DateTime.hour == 8);
  ok1(nmea_info.DateTime.minute == 23);
  ok1(nmea_info.DateTime.second == 11);
  ok1(equals(nmea_info.Location.Longitude, 7.693));
  ok1(equals(nmea_info.Location.Latitude, 51.059));
  ok1(!nmea_info.BaroAltitudeAvailable);

  /* baro altitude (proprietary Garmin sentence) */
  parser.ParseNMEAString_Internal("$PGRMZ,100,m,3*11", &nmea_info);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 100));
}

static void
TestCAI302()
{
  NMEAParser parser;
  has_baro_source = true;

  Device *device = cai302Device.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* baro altitude disabled */
  device->ParseNMEA("!w,000,000,0000,500,01287,01020,-0668,191,199,191,000,000,100*44",
                    &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(nmea_info.AirspeedAvailable);
  ok1(nmea_info.TotalEnergyVarioAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("!w,000,000,0000,500,01287,01020,-0668,191,199,191,000,000,100*44",
                    &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 287));
  ok1(nmea_info.AirspeedAvailable);
  ok1(equals(nmea_info.TrueAirspeed, -6.68));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -0.463));

  /* check if RMZ overrides CAI's baro altitude */
  parser.ParseNMEAString_Internal("$PGRMZ,100,m,3*11", &nmea_info);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 287));

  delete device;

  has_baro_source = false;
}

static void
TestLX()
{
  Device *device = lxDevice.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* baro altitude disabled */
  device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(!nmea_info.AirspeedAvailable);
  ok1(!nmea_info.TotalEnergyVarioAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 1266.5));
  ok1(!nmea_info.AirspeedAvailable);
  ok1(!nmea_info.TotalEnergyVarioAvailable);

  /* airspeed and vario available */
  device->ParseNMEA("$LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1",
                    &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 1665.5));
  ok1(nmea_info.AirspeedAvailable);
  ok1(equals(nmea_info.TrueAirspeed, 222.3/3.6));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, 1.71));

  delete device;
}

static void
TestILEC()
{
  Device *device = ilec_device_driver.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* baro altitude disabled */
  device->ParseNMEA("$PILC,PDA1,1489,-3.21*7D", &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(!nmea_info.AirspeedAvailable);
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -3.21));
  ok1(!nmea_info.ExternalWindAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("$PILC,PDA1,1489,-3.21,274,15,58*7D", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 1489));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -3.21));
  ok1(nmea_info.ExternalWindAvailable);
  ok1(equals(nmea_info.ExternalWind.norm, 15 / 3.6));
  ok1(equals(nmea_info.ExternalWind.bearing, 274));

  delete device;
}

int main(int argc, char **argv)
{
  plan_tests(57);

  TestGeneric();
  TestCAI302();
  TestLX();
  TestILEC();

  return exit_status();
}
