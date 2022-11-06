/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "system/Path.hpp"

Device::~Device() {}

bool
AbstractDevice::EnableNMEA([[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

void
AbstractDevice::LinkTimeout()
{
}

bool
AbstractDevice::ParseNMEA([[maybe_unused]] const char *line, [[maybe_unused]] struct NMEAInfo &info)
{
  return false;
}

bool
AbstractDevice::PutMacCready([[maybe_unused]] double MacCready, [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutBugs([[maybe_unused]] double bugs, [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutBallast([[maybe_unused]] double fraction, [[maybe_unused]] double overload,
                           [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutQNH([[maybe_unused]] const AtmosphericPressure &pres,
                       [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutVolume([[maybe_unused]] unsigned volume, [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}


bool
AbstractDevice::PutPilotEvent([[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutActiveFrequency([[maybe_unused]] RadioFrequency frequency,
                                   [[maybe_unused]] const TCHAR *name,
                                   [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutStandbyFrequency([[maybe_unused]] RadioFrequency frequency,
                                    [[maybe_unused]] const TCHAR *name,
                                    [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::EnablePassThrough([[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::Declare([[maybe_unused]] const Declaration &declaration, [[maybe_unused]] const Waypoint *home,
                        [[maybe_unused]] OperationEnvironment &env)
{
  return false;
}

void
AbstractDevice::OnSysTicker()
{
}

bool
AbstractDevice::ReadFlightList([[maybe_unused]] RecordedFlightList &flight_list,
                               [[maybe_unused]] OperationEnvironment &env)
{
  return false;
}

bool
AbstractDevice::DownloadFlight([[maybe_unused]] const RecordedFlightInfo &flight,
                               [[maybe_unused]] Path path, [[maybe_unused]] OperationEnvironment &env)
{
  return false;
}

bool
AbstractDevice::DataReceived(std::span<const std::byte>, NMEAInfo &) noexcept
{
  return false;
}
