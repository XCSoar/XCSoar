/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Device/Driver/LX/Internal.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Internal.hpp"
#include "Atmosphere/Pressure.hpp"

#include <cstdio>

bool
LXDevice::PutBallast(gcc_unused fixed fraction, fixed overload,
                     OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  // This is a copy of the routine done in LK8000 for LX MiniMap, realized
  // by Lx developers.

  char tmp[100];
  sprintf(tmp, "PFLX2,,%.2f,,,,", (double)overload);
  PortWriteNMEA(port, tmp);

  // LXNAV V7 variant:
  sprintf(tmp, "PLXV0,BAL,W,%.2f", (double)overload);
  PortWriteNMEA(port, tmp);

  return true;
}

bool
LXDevice::PutBugs(fixed bugs, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  // This is a copy of the routine done in LK8000 for LX MiniMap, realized
  // by Lx developers.

  char tmp[100];
  int transformed_bugs_value = 100 - (int)(bugs*100);
  sprintf(tmp, "PFLX2,,,%d,,,", transformed_bugs_value);
  PortWriteNMEA(port, tmp);

  // LXNAV V7 variant:
  sprintf(tmp, "PLXV0,BUGS,W,%d", transformed_bugs_value);
  PortWriteNMEA(port, tmp);

  return true;
}

bool
LXDevice::PutMacCready(fixed mac_cready, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  char tmp[32];
  sprintf(tmp, "PFLX2,%1.1f,,,,,,", (double)mac_cready);
  PortWriteNMEA(port, tmp);

  // LXNAV V7 variant:
  sprintf(tmp, "PLXV0,MC,W,%.1f", (double)mac_cready);
  PortWriteNMEA(port, tmp);

  return true;
}

bool
LXDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  double altitude_offset =
    (double)pres.StaticPressureToQNHAltitude(AtmosphericPressure::Standard()) / 0.3048;

  char buffer[100];
  sprintf(buffer, "PFLX3,%.2f,,,,,,,,,,,,", altitude_offset);
  PortWriteNMEA(port, buffer);

  // LXNAV V7 variant:
  sprintf(buffer, "PLXV0,QNH,W,%.2f", altitude_offset);
  PortWriteNMEA(port, buffer);

  return true;
}
