/* Copyright_License {

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

#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "TestUtil.hpp"
#include "Protection.hpp"

#include <string.h>

static bool vario_updated;

FLYING_STATE::FLYING_STATE() {}

void TriggerVarioUpdate()
{
  vario_updated = true;
}

bool
NMEAParser::NMEAChecksum(const char *p)
{
  return true;
}

size_t
Declaration::size() const
{
  return 0;
}

static void
TestCAI302()
{
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

  delete device;
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

int main(int argc, char **argv)
{
  plan_tests(24);

  TestCAI302();
  TestLX();

  return 0;
}
