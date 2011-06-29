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

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Port.hpp"
#include "Units/Units.hpp"

#include <stdio.h>

bool
CAI302Device::Open(gcc_unused OperationEnvironment &env)
{
  CAI302::LogModeQuick(*port);

  return true;
}

bool
CAI302Device::PutMacCready(fixed MacCready)
{
  unsigned mac_cready = uround(Units::ToUserUnit(MacCready * 10, unKnots));

  char szTmp[32];
  sprintf(szTmp, "!g,m%u\r", mac_cready);
  port->Write(szTmp);

  return true;
}

bool
CAI302Device::PutBugs(fixed Bugs)
{
  unsigned bugs = uround(Bugs * 100);

  char szTmp[32];
  sprintf(szTmp, "!g,u%u\r", bugs);
  port->Write(szTmp);

  return true;
}

bool
CAI302Device::PutBallast(fixed Ballast)
{
  unsigned ballast = uround(Ballast * 100);

  char szTmp[32];
  sprintf(szTmp, "!g,b%u\r", ballast);
  port->Write(szTmp);

  return true;
}
