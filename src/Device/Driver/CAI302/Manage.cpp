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
#include "Device/Port/Port.hpp"

bool
CAI302Device::Reboot(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::Reboot(port);
  CAI302::LogModeQuick(port);
  return success;
}

bool
CAI302Device::PowerOff(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::PowerOff(port);
  CAI302::LogModeQuick(port);
  return success;
}

bool
CAI302Device::StartLogging(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::StartLogging(port);

  if (success)
    CAI302::LogMode(port);
  else
    CAI302::LogModeQuick(port);

  return success;
}

bool
CAI302Device::StopLogging(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::StopLogging(port);

  if (success)
    CAI302::LogMode(port);
  else
    CAI302::LogModeQuick(port);

  return success;
}

bool
CAI302Device::SetVolume(unsigned volume, OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::SetVolume(port, volume);

  if (success)
    CAI302::LogMode(port);
  else
    CAI302::LogModeQuick(port);

  return success;
}

bool
CAI302Device::ClearPoints(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::ClearPoints(port);

  if (success)
    CAI302::LogMode(port);
  else
    CAI302::LogModeQuick(port);

  return success;
}


bool
CAI302Device::ClearPilot(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::ClearPilot(port);

  if (success)
    CAI302::LogMode(port);
  else
    CAI302::LogModeQuick(port);

  return success;
}

bool
CAI302Device::ClearLog(OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  bool success = CAI302::ClearLog(port);

  if (success)
    CAI302::LogMode(port);
  else
    CAI302::LogModeQuick(port);

  return success;
}

