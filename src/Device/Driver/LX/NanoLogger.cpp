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

#include "NanoLogger.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Util/NMEAReader.hpp"
#include "Operation/Operation.hpp"
#include "OS/Path.hpp"
#include "Time/TimeoutClock.hpp"
#include "NMEA/InputLine.hpp"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
RequestLogbookInfo(Port &port, OperationEnvironment &env)
{
  return PortWriteNMEA(port, "PLXVC,LOGBOOKSIZE,R,", env);
}

static char *
ReadLogbookLine(PortNMEAReader &reader, TimeoutClock timeout)
{
  return reader.ExpectLine("PLXVC,LOGBOOK,A,", timeout);
}

static int
GetNumberOfFlights(Port &port, PortNMEAReader &reader,
                   OperationEnvironment &env, TimeoutClock timeout)
{
  reader.Flush();

  if (!RequestLogbookInfo(port, env))
    return -1;

  const char *response;
  while (true) {
    response = reader.ExpectLine("PLXVC,LOGBOOK", timeout);
    if (response == nullptr)
      return -1;

    if (memcmp(response, ",A,", 3) == 0) {
      /* old Nano firmware versions (e.g. 2.05) print "LOGBOOK,A,n" */
      response += 3;
      break;
    } else if (memcmp(response, "SIZE,A,", 7) == 0) {
      /* new Nano firmware versions (e.g. 2.10) print
         "LOGBOOKSIZE,A,n" */
      response += 7;
      break;
    }
  }

  char *endptr;
  unsigned nflights = strtoul(response, &endptr, 10);
  if (endptr == response)
    return -1;

  while (*endptr == ',')
    ++endptr;

  if (*endptr != 0)
    return -1;

  return nflights;
}

static bool
ReadDate(NMEAInputLine &line, BrokenDate &date)
{
  char buffer[16];
  line.Read(buffer, sizeof(buffer));

  char *p = buffer, *endptr;
  date.day = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != '.')
    return false;

  p = endptr + 1;
  date.month = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != '.')
    return false;

  p = endptr + 1;
  date.year = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != 0)
    return false;

  return date.IsPlausible();
}

static bool
ReadTime(NMEAInputLine &line, BrokenTime &time)
{
  char buffer[10];
  line.Read(buffer, sizeof(buffer));

  char *p = buffer, *endptr;
  time.hour = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != ':')
    return false;

  p = endptr + 1;
  time.minute = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != ':')
    return false;

  p = endptr + 1;
  time.second = strtoul(p, &endptr, 10);
  if (endptr == p || *endptr != 0)
    return false;

  return time.IsPlausible();
}

static bool
RequestLogbookContents(Port &port, unsigned start, unsigned end,
                       OperationEnvironment &env)
{
  char buffer[32];
  sprintf(buffer, "PLXVC,LOGBOOK,R,%u,%u,", start, end);

  return PortWriteNMEA(port, buffer, env);
}

static bool
ReadFilename(NMEAInputLine &line, RecordedFlightInfo &info)
{
  line.Read(info.internal.lx.nano_filename,
            sizeof(info.internal.lx.nano_filename));
  return info.internal.lx.nano_filename[0] != 0;
}

static bool
ParseLogbookContent(const char *_line, RecordedFlightInfo &info)
{
  NMEAInputLine line(_line);

  unsigned n;
  return line.Skip() &&
    line.ReadChecked(n) &&
    ReadFilename(line, info) > 0 &&
    ReadDate(line, info.date) &&
    ReadTime(line, info.start_time) &&
    ReadTime(line, info.end_time);
}

static bool
ReadLogbookContent(PortNMEAReader &reader, RecordedFlightInfo &info,
                   TimeoutClock timeout)
{
  while (true) {
    const char *line = ReadLogbookLine(reader, timeout);
    if (line == nullptr)
      return false;

    if (ParseLogbookContent(line, info))
      return true;
  }
}

static bool
ReadLogbookContents(PortNMEAReader &reader, RecordedFlightList &flight_list,
                    unsigned n, TimeoutClock timeout)
{
  while (n-- > 0) {
    if (!ReadLogbookContent(reader, flight_list.append(), timeout))
      return false;
  }

  return true;
}

static bool
GetLogbookContents(Port &port, PortNMEAReader &reader,
                   RecordedFlightList &flight_list,
                   unsigned start, unsigned n,
                   OperationEnvironment &env, TimeoutClock timeout)
{
  reader.Flush();

  return RequestLogbookContents(port, start, start + n, env) &&
    ReadLogbookContents(reader, flight_list, n, timeout);
}

bool
Nano::ReadFlightList(Port &port, RecordedFlightList &flight_list,
                     OperationEnvironment &env)
{
  port.StopRxThread();
  PortNMEAReader reader(port, env);

  TimeoutClock timeout(2000);
  int nflights = GetNumberOfFlights(port, reader, env, timeout);
  if (nflights <= 0)
    return nflights == 0;

  env.SetProgressRange(nflights);

  unsigned requested_tail = 1;
  while (true) {
    const unsigned room = flight_list.max_size() - flight_list.size();
    const unsigned remaining = nflights - requested_tail + 1;
    const unsigned nmax = std::min(room, remaining);
    if (nmax == 0)
      break;

    /* read 8 records at a time */
    const unsigned nrequest = std::min(nmax, 8u);

    timeout = TimeoutClock(2000);
    if (!GetLogbookContents(port, reader, flight_list,
                            requested_tail, nrequest, env, timeout))
      return false;

    requested_tail += nrequest;
    env.SetProgressPosition(requested_tail - 1);
  }

  return true;
}

static bool
RequestFlight(Port &port, const char *filename,
              unsigned start_row, unsigned end_row,
              OperationEnvironment &env)
{
  char buffer[64];
  sprintf(buffer, "PLXVC,FLIGHT,R,%s,%u,%u,", filename, start_row, end_row);

  return PortWriteNMEA(port, buffer, env);
}

static bool
HandleFlightLine(const char *_line, FILE *file,
                 unsigned &i, unsigned &row_count_r)
{
  NMEAInputLine line(_line);

  /* this is supposed to be "filename", but my Nano leaves this column
     empty, so let's just ignore its value */
  line.Skip();

  unsigned row, row_count;
  if (!line.ReadChecked(row) || !line.ReadChecked(row_count) ||
      row < 1 || row > row_count)
    return false;

  if (row != i)
    /* wrong row index, what happened here? */
    return false;

  if (row_count_r == 0)
    row_count_r = row_count;
  else if (row_count != row_count_r)
    /* don't allow changes in file size */
    return false;

  auto content = line.Rest();
  size_t length = content.end() - content.begin();
  if (fwrite(content.begin(), 1, length, file) != length)
    return false;

  fputs("\r\n", file);
  ++i;
  return true;
}

static bool
DownloadFlightInner(Port &port, const char *filename, FILE *file,
                    OperationEnvironment &env)
{
  PortNMEAReader reader(port, env);
  unsigned row_count = 0, i = 1;

  while (true) {
    /* read up to 32 lines at a time */
    unsigned nrequest = row_count == 0 ? 1 : 32;
    if (row_count > 0) {
      assert(i <= row_count);
      const unsigned remaining = row_count - i + 1;
      if (nrequest > remaining)
        nrequest = remaining;
    }

    const unsigned start = i;
    const unsigned end = start + nrequest;
    unsigned request_retry_count = 0;

    /* read the requested lines and save to file */

    while (i != end) {
      if (i == start) {
        /* send request range to Nano */
        reader.Flush();
        if (!RequestFlight(port, filename, start, end, env))
          return false;
        request_retry_count++;
      }

      TimeoutClock timeout(2000);
      const char *line = reader.ExpectLine("PLXVC,FLIGHT,A,", timeout);
      if (line == nullptr || !HandleFlightLine(line, file, i, row_count)) {
        if (request_retry_count > 5)
          return false;

        /* Discard data which might still be in-transit, e.g. buffered
           inside a bluetooth dongle */
        port.FullFlush(env, 200, 2000);

        /* If we already received parts of the request range correctly break
           out of the loop to calculate new request range */
        if (i != start)
          break;

        /* No valid reply received (i==start) - request same range again */
      }
    }

    if (i > row_count)
      /* finished successfully */
      return true;

    if (start == 1)
      /* configure the range in the first iteration, now that we know
         the length of the file */
      env.SetProgressRange(row_count);

    env.SetProgressPosition(i - 1);
  }
}

bool
Nano::DownloadFlight(Port &port, const RecordedFlightInfo &flight,
                     Path path, OperationEnvironment &env)
{
  port.StopRxThread();

  FILE *file = _tfopen(path.c_str(), _T("wb"));
  if (file == nullptr)
    return false;

  bool success = DownloadFlightInner(port, flight.internal.lx.nano_filename,
                                     file, env);
  if (fclose(file) != 0)
    success = false;

  return success;
}
