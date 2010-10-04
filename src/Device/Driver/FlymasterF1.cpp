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

#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Protection.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <stdlib.h>
#include <math.h>

class FlymasterF1Device : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info,
                         bool enable_baro);
};

static bool
VARIO(NMEAInputLine &line, NMEA_INFO *GPS_INFO);

bool
FlymasterF1Device::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO,
                             bool enable_baro)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$VARIO") == 0)
    return VARIO(line, GPS_INFO);
  else
    return false;
}

static Device *
FlymasterF1CreateOnPort(Port *com_port)
{
  return new FlymasterF1Device();
}

const struct DeviceRegister flymasterf1Device = {
  _T("FlymasterF1"),
  drfGPS | drfBaroAlt | drfVario,
  FlymasterF1CreateOnPort,
};

// *****************************************************************************
// local stuff

static bool
VARIO(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  // $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*CS

  fixed value;
  if (line.read_checked(value)) {
    GPS_INFO->BaroAltitude =
      GPS_INFO->pressure.StaticPressureToQNHAltitude(value * 100);
    GPS_INFO->BaroAltitudeAvailable = true;
  }

  if (line.read_checked(value)) {
    // vario is in dm/s
    GPS_INFO->TotalEnergyVario = value / 10;
    GPS_INFO->TotalEnergyVarioAvailable = true;
  }

  TriggerVarioUpdate();

  return true;
}
