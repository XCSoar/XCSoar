/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Device/NullPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Descriptor.hpp"
#include "Device/device.hpp"
#include "Device/Geoid.h"
#include "Engine/Navigation/GeoPoint.hpp"
#include "InputEvents.h"
#include "Thread/Trigger.hpp"
#include "DeviceBlackboard.hpp"
#include "MapWindowProjection.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"

#include <stdio.h>

DeviceBlackboard device_blackboard;

static DeviceDescriptor device;

/*
 * Fake Protection.cpp
 */

Mutex mutexBlackboard;

Trigger triggerClimbEvent(_T("triggerClimb"));

void TriggerGPSUpdate() {}
void TriggerVarioUpdate() {}

/*
 * Fake Device/device.cpp
 */

bool
devHasBaroSource()
{
  return device.IsBaroSource();
}

bool
HaveCondorDevice()
{
  return device.IsCondor();
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

bool
InputEvents::processNmea(unsigned key)
{
  return true;
}

/*
 * Fake MapWindowProjection.cpp
 */

Projection::Projection() {}
fixed Projection::GetMapScaleUser() const { return fixed_one; }

MapWindowProjection::MapWindowProjection() {}
fixed MapWindowProjection::GetMapScaleUser() const { return fixed_one; }

/*
 * Fake Settings*Blackboard.cpp
 */

SettingsComputerBlackboard::SettingsComputerBlackboard() {}
SettingsMapBlackboard::SettingsMapBlackboard() {}

/*
 * The actual code.
 */

static void
Dump(GeoPoint location)
{
  int latitude = (int)(location.Latitude.value_degrees() * 3600);
  char north_or_south = latitude < 0 ? 'S' : 'N';
  latitude = abs(latitude);

  int longitude = (int)(location.Longitude.value_degrees() * 3600);
  char east_or_west = latitude < 0 ? 'W' : 'E';
  longitude = abs(longitude);

  printf("%d.%02u.%02u%c;%d.%02u.%02u%c",
         latitude / 3600, (latitude / 60) % 60, latitude % 60, north_or_south,
         longitude / 3600, (longitude / 60) % 60, longitude % 60, east_or_west);
}

static void
Dump(const NMEA_INFO &basic)
{
  printf("Date=%02u.%02u.%04u\n",
         basic.DateTime.day, basic.DateTime.month, basic.DateTime.year);
  printf("Time=%02u:%02u:%02u\n",
         basic.DateTime.hour, basic.DateTime.minute, basic.DateTime.second);

  switch (basic.gps.Connected) {
  case 0:
    printf("GPS not connected\n");
    break;

  case 1:
    printf("Waiting for GPS fix\n");
    break;

  case 2:
    printf("GPS connected, %d satellites\n", basic.gps.SatellitesUsed);
    break;
  }

  if (basic.gps.Connected == 2) {
    printf("Position=");
    Dump(basic.Location);
    printf("\n");
  }

  if (basic.AirspeedAvailable) {
    printf("GroundSpeed=%d\n", (int)basic.GroundSpeed);
    printf("TrueAirspeed=%d\n", (int)basic.TrueAirspeed);
    printf("IndicatedAirspeed=%d\n",
           (int)basic.IndicatedAirspeed);
  }

  if (basic.gps.Connected == 2)
    printf("GPSAltitude=%d\n", (int)basic.GPSAltitude);

  if (basic.BaroAltitudeAvailable)
    printf("BaroAltitude=%d\n", (int)basic.BaroAltitude);

  if (basic.TotalEnergyVarioAvailable)
    printf("TotalEnergyVario=%.1f\n", (double)basic.TotalEnergyVario);

  if (basic.NettoVarioAvailable)
    printf("NettoVario=%.1f\n", (double)basic.NettoVario);

  if (basic.ExternalWindAvailable)
    printf("Wind=%d/%d\n",
           (int)basic.wind.bearing.value_degrees(), (int)basic.wind.norm);

  if (basic.TemperatureAvailable)
    printf("OutsideAirTemperature=%d\n", (int)basic.OutsideAirTemperature);

  if (basic.HumidityAvailable)
    printf("RelativeHumidity=%d\n", (int)basic.RelativeHumidity);

  const FLARM_STATE &flarm = basic.flarm;
  if (flarm.FLARM_Available) {
    printf("FLARM rx=%u tx=%u\n", flarm.FLARM_RX, flarm.FLARM_TX);
    printf("FLARM gps=%u\n", flarm.FLARM_GPS);
    printf("FLARM alarm=%u\n", flarm.FLARM_AlarmLevel);
    printf("FLARM traffic=%d new=%d\n", flarm.FLARMTraffic, flarm.NewTraffic);
  }
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s DRIVER\n"
            "Where DRIVER is one of:\n", argv[0]);

    const TCHAR *name;
    for (unsigned i = 0; (name = devRegisterGetName(i)) != NULL; ++i)
      _ftprintf(stderr, _T("\t%s\n"), name);

    return 1;
  }

  PathName driver_name(argv[1]);
  device.Driver = devGetDriver(driver_name);
  if (device.Driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return 1;
  }

  NullPort port(*(Port::Handler *)NULL);
  device.Com = &port;
  device.enable_baro = true;

  if (!device.Open()) {
    fprintf(stderr, "Failed to open driver: %s\n", argv[1]);
    return 1;
  }

  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    device.LineReceived(buffer);

  Dump(device_blackboard.Basic());
}
