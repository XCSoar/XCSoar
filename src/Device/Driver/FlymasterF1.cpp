/*
Copyright_License {

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
  drfBaroAlt,
  FlymasterF1CreateOnPort,
};

// *****************************************************************************
// local stuff

static bool
VARIO(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  // $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*CS

  fixed value;
  if (line.read_checked(value))
    GPS_INFO->ProvideBaroAltitude1013(NMEA_INFO::BARO_ALTITUDE_FLYMASTER,
                                      value * 100);

  if (line.read_checked(value)) {
    // vario is in dm/s
    GPS_INFO->TotalEnergyVario = value / 10;
    GPS_INFO->TotalEnergyVarioAvailable = true;
  }

  TriggerVarioUpdate();

  return true;
}
