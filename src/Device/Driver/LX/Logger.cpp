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
#include "Convert.hpp"
#include "Device/Port.hpp"
#include "Operation.hpp"
#include "OS/ByteOrder.hpp"

#include <stdio.h>

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
  if (!LX::CommandMode(port))
      return false;

  port.Flush();
  if (!LX::SendCommand(port, LX::READ_FLIGHT_LIST))
    return false;

  bool success = true;
  while (!flight_list.full()) {
    LX::FlightInfo flight;
    if (!LX::ReadCRC(port, &flight, sizeof(flight)))
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
  bool success = ReadFlightListInner(*port, flight_list, env);

  port->SetRxTimeout(500);

  LX::CommandModeQuick(*port, env);

  return success;
}

static bool
DownloadFlightInner(Port &port, const RecordedFlightInfo &flight,
                    FILE *file, OperationEnvironment &env)
{
  if (!LX::CommandMode(port))
      return false;

  LX::SeekMemory seek;
  seek.start_address = flight.internal.lx.start_address;
  seek.end_address = flight.internal.lx.end_address;
  if (!LX::SendPacket(port, LX::SEEK_MEMORY, &seek, sizeof(seek)) ||
      !LX::ExpectACK(port))
      return false;

  LX::MemorySection memory_section;
  if (!LX::ReceivePacket(port, LX::READ_MEMORY_SECTION,
                         &memory_section, sizeof(memory_section)))
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
    if (!LX::ReceivePacket(port, LX::READ_LOGGER_DATA,
                           p, lengths[i], 60000)) {
      free(data);
      return false;
    }

    p += lengths[i];
    env.SetProgressPosition(p - data);
  }

  bool success = LX::ConvertLXNToIGC(data, total_length, file);
  free(data);

  return success;
}

bool
LXDevice::DownloadFlight(const RecordedFlightInfo &flight,
                         const TCHAR *path,
                         OperationEnvironment &env)
{
  FILE *file = _tfopen(path, _T("wb"));
  if (file == NULL)
    return false;

  bool success = DownloadFlightInner(*port, flight, file, env);
  fclose(file);

  port->SetRxTimeout(500);

  LX::CommandModeQuick(*port, env);

  return success;
}
