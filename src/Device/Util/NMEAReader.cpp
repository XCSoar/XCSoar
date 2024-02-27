// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEAReader.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Checksum.hpp"
#include "time/TimeoutClock.hpp"
#include "util/StringCompare.hxx"

#include <algorithm>

#include <string.h>
#include <stdlib.h>

inline bool
PortNMEAReader::Fill(TimeoutClock timeout)
{
  const auto dest = buffer.Write();
  if (dest.empty())
    /* already full */
    return false;

  size_t nbytes = port.WaitAndRead(std::as_writable_bytes(dest),
                                   env, timeout);

  buffer.Append(nbytes);
  return true;
}

inline char *
PortNMEAReader::GetLine()
{
  const auto src = buffer.Read();
  char *const end = src.data() + src.size();

  /* a NMEA line starts with a dollar symbol ... */
  char *dollar = std::find(src.data(), end, '$');
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

  const uint8_t calculated_checksum = NMEAChecksum({start, asterisk});

  const char checksum_buffer[3] = { asterisk[1], asterisk[2], 0 };
  char *endptr;
  const uint8_t parsed_checksum = strtoul(checksum_buffer, &endptr, 16);
  if (endptr != checksum_buffer + 2 ||
      parsed_checksum != calculated_checksum) {
    buffer.Clear();
    return nullptr;
  }

  buffer.Consume(asterisk + 3 - src.data());

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
PortNMEAReader::ExpectLine(const char *_prefix, TimeoutClock timeout)
{
  const std::string_view prefix{_prefix};

  while (true) {
    char *line = ReadLine(timeout);
    if (line == nullptr)
      return nullptr;

    if (StringStartsWith(line, prefix))
      return line + prefix.size();
  }
}
