/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "util/Macros.hpp"
#include "util/CharUtil.hxx"
#include "Engine/Waypoint/Waypoint.hpp"

bool
CAI302Device::ReadGeneralInfo(CAI302::GeneralInfo &data,
                              OperationEnvironment &env)
{
  UploadMode(env);
  return CAI302::UploadGeneralInfo(port, data, env);
}

void
CAI302Device::Reboot(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::Reboot(port, env);
}

void
CAI302Device::PowerOff(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::PowerOff(port, env);
}

void
CAI302Device::StartLogging(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::StartLogging(port, env);
}

void
CAI302Device::StopLogging(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::StopLogging(port, env);
}

void
CAI302Device::SetVolume(unsigned volume, OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::SetVolume(port, volume, env);
}

void
CAI302Device::ClearPoints(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::ClearPoints(port, env);
}


void
CAI302Device::ClearPilot(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::ClearPilot(port, env);
}

void
CAI302Device::ClearLog(OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::ClearLog(port, env);
}

bool
CAI302Device::ReadPilotList(std::vector<CAI302::Pilot> &list,
                            unsigned &active_index,
                            OperationEnvironment &env)
{
  assert(list.empty());

  UploadMode(env);

  CAI302::PilotMetaActive meta;
  if (!CAI302::UploadPilotMetaActive(port, meta, env))
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
                                          record_size, buffer, env);
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
  UploadMode(env);

  if (!CAI302::UploadPilot(port, 0, pilot, env)) {
    mode = Mode::UNKNOWN;
    return false;
  }

  return true;
}

bool
CAI302Device::WriteActivePilot(const CAI302::Pilot &pilot,
                               OperationEnvironment &env)
{
  DownloadMode(env);

  try {
    CAI302::DownloadPilot(port, pilot, 0, env);
  } catch (...) {
    mode = Mode::UNKNOWN;
    throw;
  }

  return true;
}

bool
CAI302Device::WritePilot(unsigned index, const CAI302::Pilot &pilot,
                         OperationEnvironment &env)
{
  DownloadMode(env);

  try {
    CAI302::DownloadPilot(port, pilot, 64 + index, env);
  } catch (...) {
    mode = Mode::UNKNOWN;
    throw;
  }

  return true;
}

bool
CAI302Device::AddPilot(const CAI302::Pilot &pilot, OperationEnvironment &env)
{
  DownloadMode(env);

  try {
    CAI302::DownloadPilot(port, pilot, 255, env);
  } catch (...) {
    mode = Mode::UNKNOWN;
    throw;
  }

  return true;
}

int
CAI302Device::ReadNavpointCount(OperationEnvironment &env)
{
  UploadMode(env);

  CAI302::NavpointMeta meta;
  if (!CAI302::UploadNavpointMeta(port, meta, env)) {
    mode = Mode::UNKNOWN;
    return -1;
  }

  return FromBE16(meta.count);
}

bool
CAI302Device::ReadNavpoint(unsigned index, CAI302::Navpoint &navpoint,
                           OperationEnvironment &env)
{
  UploadMode(env);

  if (!CAI302::UploadNavpoint(port, index, navpoint, env)) {
    mode = Mode::UNKNOWN;
    return false;
  }

  return true;
}

static void
ToASCII(char *dest, size_t dest_size, const TCHAR *src)
{
  char *end = dest + dest_size - 1;
  while (*src != _T('\0') && dest < end)
    if (IsPrintableASCII(*src))
      *dest++ = (char)*src++;

  *dest = 0;
}

bool
CAI302Device::WriteNavpoint(unsigned id, const Waypoint &wp,
                            OperationEnvironment &env)
{
  DownloadMode(env);

  char name[64], remark[64];
  ToASCII(name, ARRAY_SIZE(name), wp.name.c_str());
  ToASCII(remark, ARRAY_SIZE(remark), wp.comment.c_str());

  try {
    CAI302::DownloadNavpoint(port, wp.location, (int)wp.elevation, id,
                             wp.IsTurnpoint(), wp.IsAirport(), false,
                             wp.IsLandable(), wp.IsStartpoint(),
                             wp.IsFinishpoint(), wp.flags.home,
                             false, wp.IsTurnpoint(), false,
                             name, remark, env);
  } catch (...) {
    mode = Mode::UNKNOWN;
    throw;
  }

  return true;
}

void
CAI302Device::CloseNavpoints(OperationEnvironment &env)
{
  CAI302::CloseNavpoints(port, env);
}
