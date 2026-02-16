// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  std::byte buffer[1024];

  for (unsigned i = 0; i < count; i += block_count) {
    unsigned this_block = std::min(count - i, block_count);
    unsigned n = CAI302::UploadPilotBlock(port, i, this_block,
                                          record_size, buffer, env);
    if (n != this_block)
      return false;

    const std::byte *p = buffer;
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
ToASCII(char *dest, size_t dest_size, const char *src)
{
  char *end = dest + dest_size - 1;
  while (*src != '\0' && dest < end)
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
    CAI302::DownloadNavpoint(port, wp.location, (int)wp.GetElevationOrZero(),
                             id,
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
