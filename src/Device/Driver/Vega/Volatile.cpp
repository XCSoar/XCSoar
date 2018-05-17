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

#include "Volatile.hpp"
#include "NMEA/Derived.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Math/Util.hpp"

#include <stdio.h>

void
Vega::VolatileData::CopyFrom(const DerivedInfo &calculated)
{
  stf = uround(calculated.V_stf * 10);
  terrain_altitude = iround(calculated.terrain_altitude);
  circling = calculated.circling;
}

bool
Vega::VolatileData::SendTo(Port &port, OperationEnvironment &env) const
{
  char buffer[100];
  sprintf(buffer, "PDVMC,%u,%u,%u,%d,%u",
          mc, stf, circling, terrain_altitude, qnh);
  return PortWriteNMEA(port, buffer, env);
}
