/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Device/Driver.hpp"
#include "RadioFrequency.hpp"

Device::~Device() {}

bool
AbstractDevice::EnableNMEA(OperationEnvironment &env)
{
  return true;
}

void
AbstractDevice::LinkTimeout()
{
}

bool
AbstractDevice::ParseNMEA(const char *line, struct NMEAInfo &info)
{
  return false;
}

bool
AbstractDevice::PutMacCready(fixed MacCready, OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutBugs(fixed bugs, OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutBallast(fixed fraction, fixed overload,
                           OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutQNH(const AtmosphericPressure &pres,
                       OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutActiveFrequency(RadioFrequency frequency,
                                   OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutStandbyFrequency(RadioFrequency frequency,
                                    OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::EnablePassThrough(OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::Declare(const Declaration &declaration, const Waypoint *home,
                        OperationEnvironment &env)
{
  return false;
}

void
AbstractDevice::OnSysTicker()
{
}
