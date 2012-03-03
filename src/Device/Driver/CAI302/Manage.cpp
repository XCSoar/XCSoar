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

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"

bool
CAI302Device::ReadGeneralInfo(CAI302::GeneralInfo &data,
                              OperationEnvironment &env)
{
  port.SetRxTimeout(500);
  CAI302::CommandModeQuick(port);
  if (!CAI302::UploadMode(port) || env.IsCancelled())
    return false;

  bool success = CAI302::UploadGeneralInfo(port, data);
  CAI302::LogModeQuick(port);
  return success;
}

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

bool
CAI302Device::ReadPilotList(std::vector<CAI302::Pilot> &list,
                            unsigned &active_index,
                            OperationEnvironment &env)
{
  assert(list.empty());

  port.SetRxTimeout(500);

  CAI302::CommandModeQuick(port);
  if (!CAI302::UploadMode(port) || env.IsCancelled())
    return false;

  CAI302::PilotMetaActive meta;
  if (!CAI302::UploadPilotMetaActive(port, meta) || env.IsCancelled())
    return false;

  if (meta.record_size < sizeof(CAI302::Pilot))
    /* weird record size */
    return false;

  const unsigned count = meta.count;
  list.reserve(count);
  const unsigned record_size = meta.record_size;
  active_index = meta.active_index;

  const unsigned block_count = 8;

  uint8_t buffer[1024];

  for (unsigned i = 0; i < count; i += block_count) {
    unsigned this_block = std::min(count - i, block_count);
    unsigned n = CAI302::UploadPilotBlock(port, i, this_block,
                                          record_size, buffer);
    if (n != this_block)
      return false;

    const uint8_t *p = buffer;
    for (unsigned j = 0; j < n; ++j, p += record_size)
      list.push_back(*(const CAI302::Pilot *)p);
  }

  return true;
}

bool
CAI302Device::ReadActivePilot(CAI302::Pilot &pilot, OperationEnvironment &env)
{
  port.SetRxTimeout(500);

  CAI302::CommandModeQuick(port);
  return CAI302::UploadMode(port) && !env.IsCancelled() &&
    CAI302::UploadPilot(port, 0, pilot);
}

bool
CAI302Device::WriteActivePilot(const CAI302::Pilot &pilot,
                               OperationEnvironment &env)
{
  port.SetRxTimeout(5000);

  CAI302::CommandModeQuick(port);
  return CAI302::DownloadMode(port) && !env.IsCancelled() &&
    CAI302::DownloadPilot(port, pilot, 0);
}

bool
CAI302Device::WritePilot(unsigned index, const CAI302::Pilot &pilot,
                         OperationEnvironment &env)
{
  port.SetRxTimeout(5000);

  CAI302::CommandModeQuick(port);
  if (!CAI302::DownloadMode(port) || env.IsCancelled())
    return false;

  return CAI302::DownloadPilot(port, pilot, 64 + index);
}

bool
CAI302Device::AddPilot(const CAI302::Pilot &pilot, OperationEnvironment &env)
{
  port.SetRxTimeout(5000);

  CAI302::CommandModeQuick(port);
  if (!CAI302::DownloadMode(port) || env.IsCancelled())
    return false;

  return CAI302::DownloadPilot(port, pilot, 255);
}
