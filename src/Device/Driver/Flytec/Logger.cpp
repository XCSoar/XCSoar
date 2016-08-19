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
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Time/TimeoutClock.hpp"
#include "Util/Macros.hpp"
#include "OS/Path.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "Operation/Operation.hpp"
#include "IGC/IGCParser.hpp"
#include "Util/StringCompare.hxx"

#include <stdlib.h>
#include <string.h>

static bool
ExpectXOff(Port &port, OperationEnvironment &env, unsigned timeout_ms)
{
  return port.WaitForChar(0x13, env, timeout_ms) == Port::WaitResult::READY;
}

static bool
ReceiveLine(Port &port, char *buffer, size_t length, unsigned timeout_ms)
{
  TimeoutClock timeout(timeout_ms);

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.HasExpired())
      return false;

    // Read single character from port
    int c = port.GetChar();

    // On failure try again until timed out
    if (c == -1)
      continue;

    // Break on XOn
    if (c == 0x11) {
      *p = '\0';
      break;
    }

    // Write received character to buffer
    *p = c;
    p++;

    // Break on line break
    if (c == '\n') {
      *p = '\0';
      break;
    }
  }

  return true;
}

static bool
ParseDate(const char *str, BrokenDate &date)
{
  char *endptr;

  // Parse day
  date.day = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != '.')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse month
  date.month = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != '.')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse year
  date.year = strtoul(str, &endptr, 10) + 2000;

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

bool
FlytecDevice::ReadFlightList(RecordedFlightList &flight_list,
                             OperationEnvironment &env)
{
  port.StopRxThread();

  char buffer[256];
  strcpy(buffer, "$PBRTL,");
  AppendNMEAChecksum(buffer);
  strcat(buffer, "\r\n");

  port.Write(buffer);
  if (!ExpectXOff(port, env, 1000))
    return false;

  unsigned tracks = 0;
  while (true) {
    // Check if the user cancelled the operation
    if (env.IsCancelled())
      return false;

    // Receive the next line
    if (!ReceiveLine(port, buffer, ARRAY_SIZE(buffer), 1000))
      return false;

    // XON was received, last record was read already
    if (StringIsEmpty(buffer))
      break;

    // $PBRTL    Identifier
    // AA        total number of stored tracks
    // BB        actual number of track (0 indicates the most actual track)
    // DD.MM.YY  date of recorded track (UTC)(e.g. 24.03.04)
    // hh:mm:ss  starttime (UTC)(e.g. 08:23:15)
    // HH:MM:SS  duration (e.g. 03:23:15)
    // *ZZ       Checksum as defined by NMEA

    RecordedFlightInfo flight;
    NMEAInputLine line(buffer);

    // Skip $PBRTL
    line.Skip();

    if (tracks == 0) {
      // If number of tracks not read yet
      // .. read and save it
      if (!line.ReadChecked(tracks))
        continue;

      env.SetProgressRange(tracks);
    } else
      line.Skip();

    if (!line.ReadChecked(flight.internal.flytec))
      continue;

    if (tracks != 0 && flight.internal.flytec < tracks)
      env.SetProgressPosition(flight.internal.flytec);

    char field_buffer[16];
    line.Read(field_buffer, ARRAY_SIZE(field_buffer));
    if (!ParseDate(field_buffer, flight.date))
      continue;

    line.Read(field_buffer, ARRAY_SIZE(field_buffer));
    if (!ParseTime(field_buffer, flight.start_time))
      continue;

    BrokenTime duration;
    line.Read(field_buffer, ARRAY_SIZE(field_buffer));
    if (!ParseTime(field_buffer, duration))
      continue;

    flight.end_time = flight.start_time + duration;
    flight_list.append(flight);
  }

  return true;
}

bool
FlytecDevice::DownloadFlight(const RecordedFlightInfo &flight,
                             Path path, OperationEnvironment &env)
{
  port.StopRxThread();

  PeriodClock status_clock;
  status_clock.Update();

  // Request flight record
  char buffer[256];
  sprintf(buffer, "$PBRTR,%02d", flight.internal.flytec);
  AppendNMEAChecksum(buffer);
  strcat(buffer, "\r\n");

  port.Write(buffer);
  if (!ExpectXOff(port, env, 1000))
    return false;

  // Open file writer
  FileOutputStream fos(path);
  BufferedOutputStream os(fos);

  unsigned start_sec = flight.start_time.GetSecondOfDay();
  unsigned end_sec = flight.end_time.GetSecondOfDay();
  if (end_sec < start_sec)
    end_sec += 24 * 60 * 60;

  unsigned range = end_sec - start_sec;
  env.SetProgressRange(range);

  while (true) {
    // Check if the user cancelled the operation
    if (env.IsCancelled())
      return false;

    // Receive the next line
    if (!ReceiveLine(port, buffer, ARRAY_SIZE(buffer), 1000))
      return false;

    // XON was received
    if (StringIsEmpty(buffer))
      break;

    if (status_clock.CheckUpdate(250) &&
        *buffer == 'B') {
      // Parse the fix time
      BrokenTime time;
      if (IGCParseTime(buffer + 1, time)) {

        unsigned time_sec = time.GetSecondOfDay();
        if (time_sec < start_sec)
          time_sec += 24 * 60 * 60;

        if (time_sec > end_sec + 5 * 60)
          time_sec = start_sec;

        unsigned position = time_sec - start_sec;
        if (position > range)
          position = range;

        env.SetProgressPosition(position);
      }
    }

    // Write line to the file
    os.Write(buffer);
  }

  os.Flush();
  fos.Commit();

  return true;
}
