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

#include "Internal.hpp"
#include "NanoLogger.hpp"
#include "Protocol.hpp"
#include "Convert.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Operation/Operation.hpp"
#include "OS/ByteOrder.hpp"
#include "OS/Path.hpp"

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
  if (!LX::CommandMode(port, env))
    return false;

  port.Flush();
  if (!LX::SendCommand(port, LX::READ_FLIGHT_LIST))
    return false;

  bool success = false;
  while (!flight_list.full()) {
    LX::FlightInfo flight;
    if (!LX::ReadCRC(port, &flight, sizeof(flight), env,
                     20000, 2000, 180000))
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
  if (IsNano()) {
    if (!EnableNanoNMEA(env))
      return false;

    assert(!busy);
    busy = true;

    bool success = Nano::ReadFlightList(port, flight_list, env);
    busy = false;
    return success;
  }

  if (!EnableCommandMode(env))
    return false;

  assert(!busy);
  busy = true;

  bool success = ReadFlightListInner(port, flight_list, env);

  LX::CommandModeQuick(port, env);

  busy = false;

  return success;
}

static bool
DownloadFlightInner(Port &port, const RecordedFlightInfo &flight,
                    FILE *file, OperationEnvironment &env)
{
  if (!LX::CommandMode(port, env))
    return false;

  port.Flush();

  LX::SeekMemory seek;
  seek.start_address = flight.internal.lx.start_address;
  seek.end_address = flight.internal.lx.end_address;
  if (!LX::SendPacket(port, LX::SEEK_MEMORY, &seek, sizeof(seek), env) ||
      !LX::ExpectACK(port, env))
      return false;

  LX::MemorySection memory_section;
  if (!LX::ReceivePacketRetry(port, LX::READ_MEMORY_SECTION,
                              &memory_section, sizeof(memory_section), env,
                              5000, 2000, 60000, 2))
      return false;

  unsigned lengths[LX::MemorySection::N];
  unsigned total_length = 0;
  for (unsigned i = 0; i < LX::MemorySection::N; ++i) {
    lengths[i] = FromBE16(memory_section.lengths[i]);
    total_length += lengths[i];
  }

  env.SetProgressRange(total_length);

  uint8_t *data = new uint8_t[total_length], *p = data;
  for (unsigned i = 0; i < LX::MemorySection::N && lengths[i] > 0; ++i) {
    if (!LX::ReceivePacketRetry(port, (LX::Command)(LX::READ_LOGGER_DATA + i),
                                p, lengths[i], env,
                                20000, 2000, 300000, 2)) {
      delete [] data;
      return false;
    }

    p += lengths[i];
    env.SetProgressPosition(p - data);
  }

  bool success = LX::ConvertLXNToIGC(data, total_length, file);
  delete [] data;

  return success;
}

bool
LXDevice::DownloadFlight(const RecordedFlightInfo &flight,
                         Path path,
                         OperationEnvironment &env)
{
  if (flight.internal.lx.nano_filename[0] != 0) {
    assert(!busy);
    busy = true;

    bool success = Nano::DownloadFlight(port, flight, path, env);
    busy = false;
    return success;
  }

  if (!EnableCommandMode(env))
    return false;

  FILE *file = _tfopen(path.c_str(), _T("wb"));
  if (file == nullptr)
    return false;

  assert(!busy);
  busy = true;

  bool success = DownloadFlightInner(port, flight, file, env);
  fclose(file);

  LX::CommandModeQuick(port, env);

  busy = false;

  return success;
}
