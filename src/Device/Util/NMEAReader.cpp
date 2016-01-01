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

#include "NMEAReader.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Checksum.hpp"
#include "Time/TimeoutClock.hpp"

#include <algorithm>

#include <string.h>
#include <stdlib.h>

inline bool
PortNMEAReader::Fill(TimeoutClock timeout)
{
  const auto dest = buffer.Write();
  if (dest.IsEmpty())
    /* already full */
    return false;

  size_t nbytes = port.WaitAndRead(dest.data, dest.size, env, timeout);
  if (nbytes == 0)
    return false;

  buffer.Append(nbytes);
  return true;
}

inline char *
PortNMEAReader::GetLine()
{
  const auto src = buffer.Read();
  char *const end = src.data + src.size;

  /* a NMEA line starts with a dollar symbol ... */
  char *dollar = std::find(src.data, end, '$');
  if (dollar == end) {
    buffer.Clear();
    return nullptr;
  }

  char *start = dollar + 1;

  /* ... and ends with an asterisk */
  char *asterisk = std::find(start, end, '*');
  if (asterisk + 3 > end)
    /* need more data */
    return nullptr;

  /* verify the checksum following the asterisk (two hex digits) */

  const uint8_t calculated_checksum = NMEAChecksum(start, asterisk - start);

  const char checksum_buffer[3] = { asterisk[1], asterisk[2], 0 };
  char *endptr;
  const uint8_t parsed_checksum = strtoul(checksum_buffer, &endptr, 16);
  if (endptr != checksum_buffer + 2 ||
      parsed_checksum != calculated_checksum) {
    buffer.Clear();
    return nullptr;
  }

  buffer.Consume(asterisk + 3 - src.data);

  *asterisk = 0;
  return start;
}

void
PortNMEAReader::Flush()
{
  port.Flush();
  buffer.Clear();
}

char *
PortNMEAReader::ReadLine(TimeoutClock timeout)
{
  while (true) {
    char *line = GetLine();
    if (line != nullptr)
      return line;

    if (!Fill(timeout))
      return nullptr;
  }
}

char *
PortNMEAReader::ExpectLine(const char *prefix, TimeoutClock timeout)
{
  const size_t prefix_length = strlen(prefix);

  while (true) {
    char *line = ReadLine(timeout);
    if (line == nullptr)
      return nullptr;

    if (memcmp(line, prefix, prefix_length) == 0)
      return line + prefix_length;
  }
}
