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

#include "Device/Driver/Vega.hpp"
#include "Internal.hpp"
#include "Operation/Operation.hpp"

void
VegaDevice::LinkTimeout()
{
  AbstractDevice::LinkTimeout();
  detected = false;

  settings.Lock();
  settings.clear();
  settings.Unlock();
}

void
VegaDevice::OnCalculatedUpdate(const MoreData &basic,
                               const DerivedInfo &calculated)
{
  volatile_data.CopyFrom(calculated);

  if (detected) {
    NullOperationEnvironment env;
    volatile_data.SendTo(port, env);
  }

#ifdef UAV_APPLICATION
  const ThermalLocatorInfo &t = calculated.thermal_locator;
  char tbuf[100];
  sprintf(tbuf, "PTLOC,%d,%3.5f,%3.5f,%g,%g",
          (int)(t.estimate_valid),
          t.estimate_location.Longitude.Degrees(),
          t.estimate_location.Latitude.Degrees(),
          0.,
          0.);

  PortWriteNMEA(port, tbuf);
#endif
}
