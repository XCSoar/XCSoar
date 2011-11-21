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

#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <stdlib.h>
#include <math.h>

class FlymasterF1Device : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);
};

static bool
VARIO(NMEAInputLine &line, NMEAInfo &info)
{
  // $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*CS

  fixed value;
  if (line.read_checked(value))
    info.ProvideStaticPressure(AtmosphericPressure::HectoPascal(value));

  if (line.read_checked(value))
    info.ProvideTotalEnergyVario(value / 10);

  return true;
}

bool
FlymasterF1Device::ParseNMEA(const char *String, NMEAInfo &info)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$VARIO") == 0)
    return VARIO(line, info);
  else
    return false;
}

static Device *
FlymasterF1CreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new FlymasterF1Device();
}

const struct DeviceRegister flymasterf1Device = {
  _T("FlymasterF1"),
  _T("Flymaster F1"),
  0,
  FlymasterF1CreateOnPort,
};
