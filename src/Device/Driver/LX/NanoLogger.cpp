// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogFile.hpp"
#include <exception>
#include "NanoLogger.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Util/NMEAReader.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileOutputStream.hxx"
#include "time/TimeoutClock.hpp"
#include "NMEA/InputLine.hpp"
#include "util/SpanCast.hxx"
#include "util/StringCompare.hxx"
#include "system/FileUtil.hpp"
#include "util/TextFile.hxx"
#include "io/FileLineReader.hpp"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

using std::string_view_literals::operator""sv;

static unsigned
CountLinesInFile(Path path)
{
  std::ifstream file(path.c_str());
  if (!file.is_open())
    return 1;

  unsigned line_count = 0;
  std::string line;
  while (std::getline(file, line)) {
    line_count++;
  }

  // Return next line number to download (1-indexed)
  return line_count + 1;
}

static void
RequestLogbookInfo(Port &port, OperationEnvironment &env)
{
  PortWriteNMEA(port, "PLXVC,LOGBOOKSIZE,R,", env);
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

  RequestLogbookInfo(port, env);

  const char *response;
  while (true) {
    response = reader.ExpectLine("PLXVC,LOGBOOK", timeout);
    if (response == nullptr)
      return -1;

    if (auto a = StringAfterPrefix(response, ",A,"sv)) {
      /* old Nano firmware versions (e.g. 2.05) print "LOGBOOK,A,n" */
      response = a;
      break;
    } else if (auto size_a = StringAfterPrefix(response, "SIZE,A,"sv)) {
      /* new Nano firmware versions (e.g. 2.10) print
         "LOGBOOKSIZE,A,n" */
      response = size_a;
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

static void
RequestLogbookContents(Port &port, unsigned start, unsigned end,
                       OperationEnvironment &env)
{
  char buffer[32];
  sprintf(buffer, "PLXVC,LOGBOOK,R,%u,%u,", start, end);

  PortWriteNMEA(port, buffer, env);
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
  line.Skip();

  unsigned n;
  return line.ReadChecked(n) &&
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

  RequestLogbookContents(port, start, start + n, env);
  return ReadLogbookContents(reader, flight_list, n, timeout);
}

bool
Nano::ReadFlightList(Port &port, RecordedFlightList &flight_list,
                     OperationEnvironment &env)
{
  port.StopRxThread();
  PortNMEAReader reader(port, env);

  TimeoutClock timeout(std::chrono::seconds(2));
  int nflights = GetNumberOfFlights(port, reader, env, timeout);
  if (nflights <= 0)
    return nflights == 0;

  env.SetProgressRange(nflights);

  /* Start download at first flight in logger if capacity of flight_list is
     enough for all flights in logger. Otherwise, calculate the starting
     point to fill flight_list to capacity with only the latest flights. */
  unsigned requested_tail = (unsigned) std::max(1,
                     (signed) nflights - (signed) flight_list.max_size() + 1);

  while (true) {
    const unsigned room = flight_list.max_size() - flight_list.size();
    const unsigned remaining = nflights - requested_tail + 1;
    const unsigned nmax = std::min(room, remaining);
    if (nmax == 0)
      break;

    /* read 8 records at a time */
    const unsigned nrequest = std::min(nmax, 8u);

    timeout = TimeoutClock(std::chrono::seconds(2));
    if (!GetLogbookContents(port, reader, flight_list,
                            requested_tail, nrequest, env, timeout))
      return false;

    requested_tail += nrequest;
    env.SetProgressPosition(requested_tail - 1);
  }
  if (flight_list.size() > 1) {
    std::reverse(flight_list.begin(), flight_list.end());
  }

  return true;
}

static void
RequestFlight(Port &port, const char *filename,
              unsigned start_row, unsigned end_row,
              OperationEnvironment &env)
{
  char buffer[64];
  sprintf(buffer, "PLXVC,FLIGHT,R,%s,%u,%u,", filename, start_row, end_row);

  PortWriteNMEA(port, buffer, env);
}

static bool
HandleFlightLine(const char *_line, BufferedOutputStream &os,
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

  os.Write(AsBytes(line.Rest()));
  os.Write("\r\n");
  ++i;
  return true;
}

static bool
DownloadFlightInner(Port &port, const char *filename, BufferedOutputStream &os,
                    OperationEnvironment &env, unsigned *resume_row = nullptr)
{
  PortNMEAReader reader(port, env);
  unsigned row_count = 0, i = (resume_row && *resume_row > 0) ? *resume_row : 1;
  const unsigned FLUSH_INTERVAL = 500;  // Flush to disk every 500 lines
  unsigned lines_since_last_flush = 0;

  while (true) {
    /* read up to 50 lines at a time */
    unsigned nrequest = row_count == 0 ? 1 : 50;
    if (row_count > 0) {
      assert(i <= row_count);
      const unsigned remaining = row_count - i + 1;
      if (nrequest > remaining)
        nrequest = remaining;
    }

    const unsigned start = i;
    const unsigned end = start + nrequest;
    unsigned request_retry_count = 0;
    constexpr unsigned MAX_request_retry_count = 2; // based on testing retrying on this lvl has little to no effect

    /* read the requested lines and save to file */

    while (i != end) {
      if (i == start) {
        /* send request range to Nano */
        reader.Flush();
        RequestFlight(port, filename, start, end, env);
        request_retry_count++;
      }

      TimeoutClock timeout(std::chrono::seconds(i == 1 ? 20 : 2));
      const char *line = nullptr;
      try {
        line = reader.ExpectLine("PLXVC,FLIGHT,A,", timeout);
      } catch (...) {
        LogFormat(_T("Communication with logger timed out, tries: %u, line: %u"), request_retry_count, i);
        LogError(std::current_exception(), "Download failing");
      }

      if (line == nullptr || !HandleFlightLine(line, os, i, row_count)) {
        if (request_retry_count > MAX_request_retry_count) {
          /* Update resume point before throwing - but note that buffered data
             may not be flushed to disk yet, so resume will restart from last flush */
          if (resume_row)
            *resume_row = i - lines_since_last_flush;  // Safe resume point
          throw std::runtime_error("Flight download failed: maximum retries exceeded");
        }

        /* Discard data which might still be in-transit, e.g. buffered
           inside a bluetooth dongle */
        port.FullFlush(env, std::chrono::milliseconds(200),
                       std::chrono::seconds(2));

        /* If we already received parts of the request range correctly break
           out of the loop to calculate new request range */
        if (i != start)
          break;

        /* No valid reply received (i==start) - request same range again */
      } else {
        /* Line was successfully processed and written to buffer */
        lines_since_last_flush++;

        /* Periodic flush: write buffered data to disk */
        if (lines_since_last_flush >= FLUSH_INTERVAL) {
          try {
            os.Flush();
            /* Only update resume_row after successful flush to disk */
            if (resume_row)
              *resume_row = i;
            lines_since_last_flush = 0;
          } catch (...) {
            /* If flush fails, keep resume_row at previous safe point */
            LogError(std::current_exception(), "Failed to flush data to disk");
            throw;
          }
        }
      }
    }

    if (i > row_count) {
      /* Download complete - perform final flush */
      try {
        os.Flush();
        if (resume_row)
          *resume_row = i;
      } catch (...) {
        LogError(std::current_exception(), "Failed to flush final data to disk");
        throw;
      }
      /* finished successfully */
      return true;
    }

    if (start == 1)
      /* configure the range in the first iteration, now that we know
         the length of the file */
      env.SetProgressRange(row_count);

    env.SetProgressPosition(i - 1);
  }
}

bool
Nano::DownloadFlight(Port &port, const RecordedFlightInfo &flight,
                     Path path, OperationEnvironment &env,
                     [[maybe_unused]] unsigned *resume_row)
{
  port.StopRxThread();
  port.FullFlush(env, std::chrono::milliseconds(200), std::chrono::seconds(2));

  const auto partial_path = MakePartialPath(path);

  // Check if partial file exists and count lines to determine resume point
  unsigned calculated_resume_row = 1;
  if (File::Exists(partial_path)) {
    try {
      // Count lines in existing partial file
      calculated_resume_row = CountLinesInFile(partial_path);
      if (calculated_resume_row > 1) {
        LogFormat(_T("Resuming download from line %u"), calculated_resume_row);
      }
    } catch (...) {
      LogError(std::current_exception(), "Failed to count lines in partial file");
      // If we can't count, delete partial and start fresh
      File::Delete(partial_path);
      calculated_resume_row = 1;
    }
  }

  // Open file in appropriate mode
  FileOutputStream fos(partial_path, 
                      calculated_resume_row > 1 
                        ? FileOutputStream::Mode::APPEND_OR_CREATE
                        : FileOutputStream::Mode::CREATE);
  BufferedOutputStream bos(fos);

  bool success = DownloadFlightInner(port, flight.internal.lx.nano_filename,
                                     bos, env, &calculated_resume_row);

  if (success) {
    bos.Flush();
    fos.Commit();
    File::Rename(partial_path, path);
    return true;
  } else {
    // Flush partial data for resume
    try {
      bos.Flush();
      fos.Commit();
    } catch (...) {
      // If we can't save partial data, delete it
      File::Delete(partial_path);
    }
    throw std::runtime_error("Download incomplete");
  }
}
