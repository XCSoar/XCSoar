// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

void
Vega::VolatileData::SendTo(Port &port, OperationEnvironment &env) const
{
  char buffer[100];
  sprintf(buffer, "PDVMC,%u,%u,%u,%d,%u",
          mc, stf, circling, terrain_altitude, qnh);
  PortWriteNMEA(port, buffer, env);
}
