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
#include "NanoLogger.hpp"
#include "Protocol.hpp"
#include "Convert.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Operation/Operation.hpp"
#include "util/ByteOrder.hxx"
#include "system/Path.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileOutputStream.hxx"
#include "util/ScopeExit.hxx"

#include <memory>

#include <stdio.h>
#include <stdlib.h>

static bool
ParseDate(BrokenDate &date, const char *p)
{
  char *endptr;
  unsigned long l = strtoul(p, &endptr, 10);
  if (l < 1 || l > 31 || *endptr != '.')
    return false;

  date.day = l;

  p = endptr + 1;
  l = strtoul(p, &endptr, 10);
  if (l < 1 || l > 12 || *endptr != '.')
    return false;

  date.month = l;

  p = endptr + 1;
  l = strtoul(p, &endptr, 10);
  if (l >= 100 || *endptr != 0)
    return false;

  date.year = 2000 + l; // Y2100 problem
  return true;
}

static bool
ParseTime(BrokenTime &time, const char *p)
{
  char *endptr;
  unsigned long l = strtoul(p, &endptr, 10);
  if (l >= 24 || *endptr != ':')
    return false;

  time.hour = l;

  p = endptr + 1;
  l = strtoul(p, &endptr, 10);
  if (l >= 60 || *endptr != ':')
    return false;

  time.minute = l;

  p = endptr + 1;
  l = strtoul(p, &endptr, 10);
  if (l >= 60 || *endptr != 0)
    return false;

  time.second = l;
  return true;
}

static bool
Copy(RecordedFlightInfo &dest, const LX::FlightInfo &src)
{
  if (!ParseDate(dest.date, src.date) ||
      !ParseTime(dest.start_time, src.start_time) ||
      !ParseTime(dest.end_time, src.stop_time))
    return false;

  /* not a Nano recording */
  dest.internal.lx.nano_filename[0] = 0;

  dest.internal.lx.start_address[0] = src.start_address.address0;
  dest.internal.lx.start_address[1] = src.start_address.address1;
  dest.internal.lx.start_address[2] = src.start_address.address2;
  dest.internal.lx.end_address[0] = src.end_address.address0;
  dest.internal.lx.end_address[1] = src.end_address.address1;
  dest.internal.lx.end_address[2] = src.end_address.address2;

  return true;
}

static bool
ReadFlightListInner(Port &port, RecordedFlightList &flight_list,
                    OperationEnvironment &env)
{
  LX::CommandMode(port, env);

  port.Flush();
  LX::SendCommand(port, LX::READ_FLIGHT_LIST);

  bool success = false;
  while (!flight_list.full()) {
    LX::FlightInfo flight;
    if (!LX::ReadCRC(port, &flight, sizeof(flight), env,
                     std::chrono::seconds(20),
                     std::chrono::seconds(2),
                     std::chrono::minutes(3)))
      break;

    success = true;
    if (!flight.IsValid())
      break;

    RecordedFlightInfo dest;
    if (Copy(dest, flight))
      flight_list.append(dest);
  }

  return success;
}

bool
LXDevice::ReadFlightList(RecordedFlightList &flight_list,
                         OperationEnvironment &env)
{
  if (IsLXNAVLogger()) {
    if (!EnableLoggerNMEA(env))
      return false;

    assert(!busy);
    busy = true;
    AtScopeExit(this) { busy = false; };

    return Nano::ReadFlightList(port, flight_list, env);
  }

  if (!EnableCommandMode(env))
    return false;

  assert(!busy);
  busy = true;
  AtScopeExit(this) { busy = false; };

  bool success = ReadFlightListInner(port, flight_list, env);

  LX::CommandModeQuick(port, env);
  return success;
}

static bool
DownloadFlightInner(Port &port, const RecordedFlightInfo &flight,
                    BufferedOutputStream &os, OperationEnvironment &env)
{
  LX::CommandMode(port, env);

  port.Flush();

  LX::SeekMemory seek;
  seek.start_address = flight.internal.lx.start_address;
  seek.end_address = flight.internal.lx.end_address;
  LX::SendPacket(port, LX::SEEK_MEMORY, &seek, sizeof(seek), env);
  LX::ExpectACK(port, env);

  LX::MemorySection memory_section;
  if (!LX::ReceivePacketRetry(port, LX::READ_MEMORY_SECTION,
                              &memory_section, sizeof(memory_section), env,
                              std::chrono::seconds(5),
                              std::chrono::seconds(2),
                              std::chrono::minutes(1), 2))
      return false;

  unsigned lengths[LX::MemorySection::N];
  unsigned total_length = 0;
  for (unsigned i = 0; i < LX::MemorySection::N; ++i) {
    lengths[i] = FromBE16(memory_section.lengths[i]);
    total_length += lengths[i];
  }

  env.SetProgressRange(total_length);

  const auto data = std::make_unique<uint8_t[]>(total_length);
  uint8_t *p = data.get();
  for (unsigned i = 0; i < LX::MemorySection::N && lengths[i] > 0; ++i) {
    if (!LX::ReceivePacketRetry(port, (LX::Command)(LX::READ_LOGGER_DATA + i),
                                p, lengths[i], env,
                                std::chrono::seconds(20),
                                std::chrono::seconds(2),
                                std::chrono::minutes(5), 2)) {
      return false;
    }

    p += lengths[i];
    env.SetProgressPosition(p - data.get());
  }

  return LX::ConvertLXNToIGC(data.get(), total_length, os);
}

bool
LXDevice::DownloadFlight(const RecordedFlightInfo &flight,
                         Path path,
                         OperationEnvironment &env)
{
  if (flight.internal.lx.nano_filename[0] != 0) {
    assert(!busy);
    busy = true;
    AtScopeExit(this) { busy = false; };

    return Nano::DownloadFlight(port, flight, path, env);
  }

  if (!EnableCommandMode(env))
    return false;

  FileOutputStream fos(path);
  BufferedOutputStream bos(fos);

  assert(!busy);
  busy = true;
  AtScopeExit(this) { busy = false; };

  bool success = DownloadFlightInner(port, flight, bos, env);

  if (success) {
    bos.Flush();
    fos.Commit();
  }

  LX::CommandModeQuick(port, env);
  return success;
}
