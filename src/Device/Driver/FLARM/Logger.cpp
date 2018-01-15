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

#include "Device.hpp"
#include "Device/RecordedFlight.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "OS/Path.hpp"
#include "Operation/Operation.hpp"

#include <cstdlib>
#include <cstring>

static bool
ParseDate(const char *str, BrokenDate &date)
{
  char *endptr;

  // Parse year
  date.year = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != '-')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse month
  date.month = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != '-')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse day
  date.day = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  return str != endptr;
}

static bool
ParseTime(const char *str, BrokenTime &time)
{
  char *endptr;

  // Parse year
  time.hour = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != ':')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse month
  time.minute = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != ':')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse day
  time.second = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  return str != endptr;
}

static BrokenTime
operator+(BrokenTime &a, BrokenTime &b)
{
  BrokenTime c;

  c.hour = a.hour + b.hour;
  c.minute = a.minute + b.minute;
  c.second = a.second + b.second;

  while (c.second >= 60) {
    c.second -= 60;
    c.minute++;
  }

  while (c.minute >= 60) {
    c.minute -= 60;
    c.hour++;
  }

  while (c.hour >= 23)
    c.hour -= 24;

  return c;
}

static bool
ParseRecordInfo(char *record_info, RecordedFlightInfo &flight)
{
  // According to testing with firmware 5.03:
  // 18CG6NG1.IGC|2011-08-12|12:23:48|02:03:25|TOBIAS BIENIEK|TH|Club

  // According to documentation:
  // 2000-11-08|20:05:21|01:21:09|J.Doe|XYZ|15M

  // Where the pilot name may take up to 100 bytes, while class, glider-
  // and competition ID can take up to 32 bytes.

  // Search for first separator
  char *p = strchr(record_info, '|');
  if (p == nullptr)
    return false;

  // Replace separator by \0
  *p = '\0';

  // Move pointer to first character after the replaced separator
  // and check for valid character
  p++;
  if (*p == '\0')
    return false;

  // Check if first field is NOT the date (length > 10)
  if (strlen(record_info) > 10) {
    record_info = p;

    // Search for second separator
    p = strchr(record_info, '|');
    if (p == nullptr)
      return false;

    // Replace separator by \0
    *p = '\0';

    // Move pointer to first character after the replaced separator
    // and check for valid character
    p++;
    if (*p == '\0')
      return false;
  }

  // Now record_info should point to the date field,
  // the date field should be null-terminated and p should
  // point to the start time field and the rest of the null-
  // terminated string

  if (!ParseDate(record_info, flight.date))
    return false;

  record_info = p;

  // Search for next separator
  p = strchr(record_info, '|');
  if (p == nullptr)
    return false;

  // Replace separator by \0
  *p = '\0';

  // Move pointer to first character after the replaced separator
  // and check for valid character
  p++;
  if (*p == '\0')
    return false;

  // Now record_info should point to the start time field,
  // the start time field should be null-terminated and p should
  // point to the duration field and the rest of the null-
  // terminated string

  if (!ParseTime(record_info, flight.start_time))
    return false;

  record_info = p;

  // Search for next separator
  p = strchr(record_info, '|');
  if (p == nullptr)
    return false;

  // Replace separator by \0
  *p = '\0';

  // Move pointer to first character after the replaced separator
  // and check for valid character
  p++;
  if (*p == '\0')
    return false;

  // Now record_info should point to the duration field,
  // the duration field should be null-terminated and p should
  // point to the pilot field and the rest of the null-
  // terminated string

  BrokenTime duration;
  if (!ParseTime(record_info, duration))
    return false;

  flight.end_time = flight.start_time + duration;

  return true;
}

bool
FlarmDevice::ReadFlightInfo(RecordedFlightInfo &flight,
                            OperationEnvironment &env)
{
  // Create header for getting record information
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MT_GETRECORDINFO);

  // Send request
  if (!SendStartByte() ||
      !SendFrameHeader(header, env, 1000))
    return false;

  // Wait for an answer and save the payload for further processing
  AllocatedArray<uint8_t> data;
  uint16_t length;
  uint8_t ack_result =
    WaitForACKOrNACK(header.sequence_number, data, length, env, 1000);

  // If neither ACK nor NACK was received
  if (ack_result != FLARM::MT_ACK || length <= 2)
    return false;

  char *record_info = (char *)data.begin() + 2;
  return ParseRecordInfo(record_info, flight);
}

FLARM::MessageType
FlarmDevice::SelectFlight(uint8_t record_number, OperationEnvironment &env)
{
  // Create header for selecting a log record
  uint8_t data[1] = { record_number };
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MT_SELECTRECORD,
                                                 data, sizeof(data));

  // Send request
  if (!SendStartByte() ||
      !SendFrameHeader(header, env, 1000) ||
      !SendEscaped(data, sizeof(data), env, 1000))
    return FLARM::MT_ERROR;

  // Wait for an answer
  return WaitForACKOrNACK(header.sequence_number, env, 1000);
}

bool
FlarmDevice::ReadFlightList(RecordedFlightList &flight_list,
                            OperationEnvironment &env)
{
  if (!BinaryMode(env))
    return false;

  // Try to receive flight information until the list is full
  for (uint8_t i = 0; !flight_list.full(); ++i) {
    FLARM::MessageType ack_result = SelectFlight(i, env);

    // Last record reached -> bail out and return list
    if (ack_result == FLARM::MT_NACK)
      break;

    // If neither ACK nor NACK was received
    if (ack_result != FLARM::MT_ACK || env.IsCancelled()) {
      mode = Mode::UNKNOWN;
      return false;
    }

    RecordedFlightInfo flight_info;
    flight_info.internal.flarm = i;
    if (ReadFlightInfo(flight_info, env))
      flight_list.append(flight_info);
  }

  return true;
}

bool
FlarmDevice::DownloadFlight(Path path, OperationEnvironment &env)
{
  FileOutputStream fos(path);
  BufferedOutputStream os(fos);

  if (env.IsCancelled())
    return false;

  env.SetProgressRange(100);
  while (true) {
    // Create header for getting IGC file data
    FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MT_GETIGCDATA);

    // Send request
    if (!SendStartByte() ||
        !SendFrameHeader(header, env, 1000) ||
        env.IsCancelled())
      return false;

    // Wait for an answer and save the payload for further processing
    AllocatedArray<uint8_t> data;
    uint16_t length;
    bool ack = WaitForACKOrNACK(header.sequence_number, data,
                                length, env, 10000) == FLARM::MT_ACK;

    // If no ACK was received
    if (!ack || length <= 3 || env.IsCancelled())
      return false;

    length -= 3;

    // Read progress (in percent)
    uint8_t progress = *(data.begin() + 2);
    env.SetProgressPosition(std::min((unsigned)progress, 100u));

    const char *last_char = (const char *)data.end() - 1;
    bool is_last_packet = (*last_char == 0x1A);
    if (is_last_packet)
      length--;

    // Read IGC data
    const char *igc_data = (const char *)data.begin() + 3;
    os.Write(igc_data, length);

    if (is_last_packet)
      break;
  }

  os.Flush();
  fos.Commit();

  return true;
}


bool
FlarmDevice::DownloadFlight(const RecordedFlightInfo &flight,
                            Path path, OperationEnvironment &env)
{
  if (!BinaryMode(env))
    return false;

  FLARM::MessageType ack_result = SelectFlight(flight.internal.flarm, env);

  // If no ACK was received -> cancel
  if (ack_result != FLARM::MT_ACK || env.IsCancelled())
    return false;

  try {
    if (DownloadFlight(path, env))
      return true;
  } catch (...) {
    mode = Mode::UNKNOWN;
    throw;
  }

  mode = Mode::UNKNOWN;

  return false;
}
