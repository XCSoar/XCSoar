/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "PocketNav.hpp"
#include "Device/Port/Port.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

#include <stdio.h>

bool
CAI302::PutMacCready(Port &port, double mc, OperationEnvironment &env)
{
  unsigned mac_cready = uround(Units::ToUserUnit(mc * 10, Unit::KNOTS));

  char buffer[32];
  sprintf(buffer, "!g,m%u\r", mac_cready);
  return port.Write(buffer);
}

bool
CAI302::PutBugs(Port &port, double bugs, OperationEnvironment &env)
{
  unsigned bugs2 = uround(bugs * 100);

  char buffer[32];
  sprintf(buffer, "!g,u%u\r", bugs2);
  return port.Write(buffer);
}

bool
CAI302::PutBallast(Port &port, double fraction, OperationEnvironment &env)
{
  unsigned ballast = uround(fraction * 10);

  char buffer[32];
  sprintf(buffer, "!g,b%u\r", ballast);
  return port.Write(buffer);
}
