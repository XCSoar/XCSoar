// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NanoLogger.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Util/NMEAReader.hpp"
#include "Operation/Cancelled.hpp"
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
#include "LogFile.hpp"
#include "IGC/IGCExtensions.hpp"
#include "IGC/IGCParser.hpp"
#include "NanoFileTransfer.hpp"
#include "util/NumberParser.hxx"

#include <algorithm>
#include <string>
#include <string_view>
#include <chrono>
#include <stdlib.h>
#include <fstream>
#include <exception>

#include <fmt/format.h>

using std::string_view_literals::operator""sv;

namespace {

/**
 * The lengths the file's own "I" and "J" records give its B and K
 * records.
 *
 * A row can arrive with a correct checksum and still be wrong: an
 * LXNAV S100 has been seen to splice the head of one row onto the tail
 * of another and checksum the result (#3229).  The row number matches
 * and the checksum matches; only the length gives it away, since every
 * B record of a file is as long as its I record says.
 */
struct RecordLengths {
  /** 0 until the I / J record has been seen */
  unsigned b = 0, k = 0;

  void Learn(std::string_view row) noexcept {
    if (row.empty() || (row.front() != 'I' && row.front() != 'J'))
      return;

    /* IGCParseExtensions() reads the "I" layout, which "J" shares */
    std::string declaration{row};
    const bool is_j = declaration.front() == 'J';
    declaration.front() = 'I';

    IGCExtensions extensions;
    if (!IGCParseExtensions(declaration.c_str(), extensions))
      return;

    if (is_j)
      k = IGCRecordLength(extensions, 7);
    else
      b = IGCRecordLength(extensions, 35);
  }

  [[gnu::pure]]
  bool Fits(std::string_view row) const noexcept {
    if (row.empty())
      return true;

    switch (row.front()) {
    case 'B':
      return b == 0 || row.size() == b;
    case 'K':
      return k == 0 || row.size() == k;
    default:
      return true;
    }
  }
};

} // anonymous namespace

/**
 * Count the rows already in a partial download, and learn the record
 * lengths from its I and J records, which a resumed download will not
 * receive again.
 */
static unsigned
CountLinesInFile(Path path, RecordLengths &lengths)
{
  FileLineReaderA reader(path);
  unsigned line_count = 0;
  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    lengths.Learn(line);
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

  /* accept implausible dates (e.g. 00.00.1980) from devices
     without an RTC -- the flight is still downloadable */
  return true;
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

  return true;
}

static void
RequestLogbookContents(Port &port, unsigned start, unsigned end,
                       OperationEnvironment &env)
{
  const auto cmd = fmt::format("PLXVC,LOGBOOK,R,{},{},", start, end);
  PortWriteNMEA(port, cmd.c_str(), env);
}

static bool
ReadFilename(NMEAInputLine &line, RecordedFlightInfo &info)
{
  line.Read(info.internal.lx.nano_filename,
            sizeof(info.internal.lx.nano_filename));
  return info.internal.lx.nano_filename[0] != 0;
}

bool
Nano::ParseLogbookContent(const char *_line, RecordedFlightInfo &info)
{
  NMEAInputLine line(_line);
  line.Skip();

  unsigned n;
  if (!line.ReadChecked(n) ||
      !ReadFilename(line, info) ||
      !ReadDate(line, info.date) ||
      !ReadTime(line, info.start_time) ||
      !ReadTime(line, info.end_time))
    return false;

  /* An S10 (firmware 9.41) reports 00:00:00 as the end of nearly
     every flight: the end was not recorded, it is not midnight */
  if (info.end_time == BrokenTime::Midnight())
    info.end_time = BrokenTime::Invalid();

  /* the file size; old firmware versions may not send it */
  unsigned size;
  info.internal.lx.nano_file_size = line.ReadChecked(size) ? size : 0;
  return true;
}

/**
 * Read exactly @p n logbook response lines, appending parseable
 * entries to @p flight_list.  Entries with implausible dates
 * (e.g. 00.00.1980 from devices without an RTC) are still
 * included so the user can download those flights.
 */
static bool
ReadLogbookContents(PortNMEAReader &reader, RecordedFlightList &flight_list,
                    unsigned n, TimeoutClock timeout)
{
  while (n-- > 0) {
    const char *line = ReadLogbookLine(reader, timeout);
    if (line == nullptr)
      return false;

    RecordedFlightInfo info;
    if (Nano::ParseLogbookContent(line, info) && !flight_list.full())
      flight_list.append() = info;
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
  const auto cmd = fmt::format("PLXVC,FLIGHT,R,{},{},{},",
                               filename, start_row, end_row);
  PortWriteNMEA(port, cmd.c_str(), env);
}

/**
 * Write one flight row.  Returns the number of bytes written, or 0
 * when the row is rejected.
 */
static unsigned
HandleFlightLine(const char *_line, BufferedOutputStream &os,
                 unsigned &i, unsigned &row_count_r,
                 RecordLengths &lengths)
{
  NMEAInputLine line(_line);

  /* this is supposed to be "filename", but my Nano leaves this column
     empty, so let's just ignore its value */
  line.Skip();

  unsigned row, row_count;
  if (!line.ReadChecked(row) || !line.ReadChecked(row_count) ||
      row < 1 || row > row_count)
    return 0;

  if (row != i)
    /* wrong row index, what happened here? */
    return 0;

  if (row_count_r == 0)
    row_count_r = row_count;
  else if (row_count != row_count_r)
    /* don't allow changes in file size */
    return 0;

  const std::string_view payload = line.Rest();
  if (!lengths.Fits(payload)) {
    LogFormat("NanoLogger: row %u is %u characters, its record type"
              " declares %u; rejecting it",
              row, unsigned(payload.size()),
              payload.front() == 'B' ? lengths.b : lengths.k);
    return 0;
  }

  lengths.Learn(payload);
  os.Write(AsBytes(payload));
  os.Write("\r\n");
  ++i;
  return unsigned(payload.size() + 2);
}

static bool
DownloadFlightInner(Port &port, const char *filename, BufferedOutputStream &os,
                    OperationEnvironment &env, RecordLengths &lengths,
                    unsigned *resume_row = nullptr)
{
  PortNMEAReader reader(port, env);
  unsigned row_count = 0, i = (resume_row && *resume_row > 0) ? *resume_row : 1;
  const unsigned FLUSH_INTERVAL = 500;  // Flush to disk every 500 lines
  unsigned lines_since_last_flush = 0;
  unsigned bytes_written = 0;
  bool range_set = false;

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
    constexpr unsigned MAX_REQUEST_RETRY_COUNT = 2; // based on testing retrying on this lvl has little to no effect

    /* read the requested lines and save to file */

    while (i != end) {
      if (i == start) {
        /* send request range to Nano */
        reader.Flush();
        RequestFlight(port, filename, start, end, env);
        request_retry_count++;
      }

      TimeoutClock timeout(std::chrono::seconds(row_count == 0 ? 20 : 2)); // using row_count to detect first request
      const char *line = nullptr;
      try {
        line = reader.ExpectLine("PLXVC,FLIGHT,A,", timeout);
      } catch (const OperationCancelled &) {
        throw;
      } catch (...) {
        LogFormat("NanoLogger: communication with logger timed out,"
                  " tries: %u, line: %u", request_retry_count, i);
        LogError(std::current_exception(), "NanoLogger: download failing");
      }

      const unsigned wrote = line == nullptr
        ? 0
        : HandleFlightLine(line, os, i, row_count, lengths);
      if (wrote == 0) {
        if (request_retry_count > MAX_REQUEST_RETRY_COUNT) {
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
        bytes_written += wrote;
        lines_since_last_flush++;

        /* This range has delivered a row.  A later bad line must
           start a new range instead of tripping the retry limit. */
        request_retry_count = 0;

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
            LogError(std::current_exception(),
                     "NanoLogger: failed to flush data to disk");
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
        LogError(std::current_exception(),
                 "NanoLogger: failed to flush final data to disk");
        throw;
      }

      if (row_count > 0) {
        if (!range_set)
          env.SetProgressRange(row_count);
        env.SetProgressBytes(bytes_written);
        env.SetProgressPosition(row_count);
      }
      /* finished successfully */
      return true;
    }

    if (!range_set){
      /* configure the range in the first iteration, now that we know
         the length of the file */
      env.SetProgressRange(row_count);
      range_set = true;
    }

    env.SetProgressBytes(bytes_written);
    env.SetProgressPosition(i - 1);
  }
}

static void
WriteFileCommand(Port &port, std::string_view command,
                 OperationEnvironment &env)
{
  const std::string line = fmt::format("PLXVC,{}", command);
  PortWriteNMEA(port, line.c_str(), env);
}

/**
 * Download a flight with the LXNAV file transfer protocol: base64
 * blocks of #PAYLOAD bytes, acknowledged in windows of @p window
 * blocks, each with a CRC-32 chained over all blocks so far, and a
 * CRC-32 of the whole file at the end.  Unlike the "FLIGHT" rows, a
 * block which lost bytes on the way cannot pass these checks (#3229).
 *
 * @return false if the logger does not answer the request, i.e. does
 * not know the protocol; throws once the transfer has started and
 * then fails
 */
static bool
DownloadFlightFile(Port &port, const char *filename, unsigned file_size,
                   unsigned window,
                   BufferedOutputStream &os, OperationEnvironment &env)
{
  constexpr unsigned PAYLOAD = 140;
  constexpr unsigned MAX_TIMEOUTS = 5;

  PortNMEAReader reader(port, env);
  reader.Flush();

  std::string request = fmt::format("FILE_INFO,R,/{},{},{}",
                                    filename, window, PAYLOAD);
  WriteFileCommand(port, request, env);

  env.SetProgressRange(file_size);

  Nano::FileTransferCrc chain, file_crc;
  unsigned next_block = 0, in_window = 0, received = 0, timeouts = 0;
  bool answered = false, lost_reported = false;
  std::byte data[PAYLOAD];

  while (true) {
    const char *line;
    try {
      line = reader.ExpectLine("PLXVC,FILE_",
                               TimeoutClock(std::chrono::seconds(2)));
    } catch (const DeviceTimeout &) {
      line = nullptr;
    }

    if (line == nullptr) {
      if (!answered)
        /* no answer at all: the logger does not know FILE_INFO */
        return false;

      if (++timeouts > MAX_TIMEOUTS) {
        if (received == 0)
          /* not one block passed the checks; let the row protocol
             try instead */
          return false;

        throw std::runtime_error("Flight download failed:"
                                 " no reply from the logger");
      }

      /* repeat the last request, as the logger may have missed it */
      WriteFileCommand(port, request, env);
      continue;
    }

    if (const char *crc_s = StringAfterPrefix(line, "CRC32,A,"sv)) {
      int32_t crc;
      if (received != file_size ||
          !ParseIntegerTo(std::string_view{crc_s}, crc) ||
          crc != file_crc.Get())
        throw std::runtime_error("Flight download failed:"
                                 " file checksum does not match");

      WriteFileCommand(port, "FILE_CRC_OK,R", env);
      os.Flush();
      return true;
    }

    const char *data_s = StringAfterPrefix(line, "DATA,A,"sv);
    if (data_s == nullptr)
      continue;

    const auto block = Nano::ParseFileDataBlock(data_s);
    if (!block)
      continue;

    /* only a data block proves that the logger knows the protocol */
    answered = true;

    if (block->number < next_block)
      /* left over from before a resend */
      continue;

    Nano::FileTransferCrc crc = chain;
    crc.Update(block->base64);

    const auto size = block->number == next_block &&
      crc.Get() == block->crc
      ? Nano::DecodeBase64(block->base64, data)
      : std::nullopt;
    const unsigned expected_size =
      std::min(PAYLOAD, file_size - received);
    if (!size || *size != expected_size) {
      /* ask once for a resend from the missing block.  An S series
         vario (firmware 9.41) ignores FILE_DATA_LOST: it finishes the
         window and waits for the acknowledgement, so the request
         repeated after a timeout acknowledges up to the missing
         block instead, which starts the next window there. */
      if (!lost_reported) {
        LogFormat("NanoLogger: block %u is missing or damaged,"
                  " requesting it again", next_block);
        WriteFileCommand(port,
                         fmt::format("FILE_DATA_LOST,R,{}", next_block),
                         env);
        request = fmt::format("FILE_OK,R,{}", next_block);
        in_window = 0;
        lost_reported = true;
      }
      continue;
    }

    chain = crc;
    file_crc.Update(std::span{data, *size});
    os.Write(std::span{data, *size});
    received += *size;
    ++next_block;
    timeouts = 0;
    lost_reported = false;

    env.SetProgressBytes(received);
    env.SetProgressPosition(received);

    if (++in_window == window || received == file_size) {
      in_window = 0;
      request = fmt::format("FILE_OK,R,{}", next_block);
      WriteFileCommand(port, request, env);
    }
  }
}

bool
Nano::DownloadFlight(Port &port, const RecordedFlightInfo &flight,
                     Path path, unsigned file_window,
                     OperationEnvironment &env)
{
  port.StopRxThread();
  port.FullFlush(env, std::chrono::milliseconds(200), std::chrono::seconds(2));

  const char *filename = flight.internal.lx.nano_filename;
  /*
  LXNANO filename length limit nano_filename uses a size 16 buffer
  but actual are only 12 characters long and i do not know if it is /0 terminated
  so to be safe we limit to 12 characters since we want to have predictable filenames
  */ 
  constexpr int NANO_FILENAME_LEN = 12; 

  char partial_filename[64];
  snprintf(partial_filename,
           sizeof(partial_filename),
           "%.*s.partial",
           NANO_FILENAME_LEN,
           filename);

  
  const auto partial_path = AllocatedPath::Build(path.GetParent(), partial_filename);

  /* prefer the verified file transfer; a partial file left by the
     row protocol is resumed with that protocol instead */
  const unsigned file_size = flight.internal.lx.nano_file_size;
  if (file_size > 0 && !File::Exists(partial_path)) {
    FileOutputStream fos(path);
    BufferedOutputStream bos(fos);
    try {
      if (DownloadFlightFile(port, filename, file_size, file_window,
                             bos, env)) {
        fos.Commit();
        LogFormat("NanoLogger: download complete (file transfer)");
        return true;
      }

      LogFormat("NanoLogger: file transfer not available,"
                " falling back to the row protocol");
    } catch (const OperationCancelled &) {
      /* stop the logger from sending the rest; the uncommitted file
         is discarded.  The cancelled environment refuses every
         further write, so send this one without it. */
      try {
        NullOperationEnvironment cancel_env;
        WriteFileCommand(port, "FILE_CANCEL,R", cancel_env);
      } catch (...) {
      }
      throw;
    } catch (...) {
      /* the rows carry the same flight and check each line's
         length, so a transfer that broke off is not the end */
      LogError(std::current_exception(),
               "NanoLogger: file transfer failed,"
               " falling back to the row protocol");
    }

    WriteFileCommand(port, "FILE_CANCEL,R", env);
    port.FullFlush(env, std::chrono::milliseconds(200),
                   std::chrono::seconds(2));
  }

  // Check if partial file exists and count lines to determine resume point
  unsigned calculated_resume_row = 1;
  RecordLengths lengths;
  if (File::Exists(partial_path)) {
    try {
      // Count lines in existing partial file
      calculated_resume_row = CountLinesInFile(partial_path, lengths);
      if (calculated_resume_row > 1) {
        LogFormat("NanoLogger: resuming download from line %u",
                  calculated_resume_row);
      }
    } catch (...) {
      LogError(std::current_exception(),
               "NanoLogger: failed to count lines in partial file,"
               " deleting it for clean fresh download.");
      // If we can't count, delete partial and start fresh
      File::Delete(partial_path);
      calculated_resume_row = 1;
      lengths = {};
    }
  }

  // Open file in appropriate mode
  FileOutputStream fos(partial_path, 
                      calculated_resume_row > 1 
                        ? FileOutputStream::Mode::APPEND_OR_CREATE
                        : FileOutputStream::Mode::CREATE);
  BufferedOutputStream bos(fos);
  try {
    bool success = DownloadFlightInner(port, filename,
                                      bos, env, lengths,
                                      &calculated_resume_row);

    if (success) {
      bos.Flush();
      fos.Commit();
      LogFormat("NanoLogger: download complete, renaming to final filename");
      if (!File::Rename(partial_path, path)) {
        LogFormat("NanoLogger: failed to rename partial flight log to final"
                  " filename");
        return false;
      }
      return true;
    } 
  } catch (...) {
    try {
        bos.Flush();
        fos.Commit();
      } catch (...) {
        LogFormat("NanoLogger: failed to flush partial data to disk");
      }
    throw;
  }
  return false; //never hapens but compiler does not know DownloadFlightInner throws on failure
}
