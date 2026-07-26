// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Device/Port/Port.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "time/TimeoutClock.hpp"
#include "Device/Error.hpp"
#include "util/StringCompare.hxx"
#include "util/StringUtil.hpp"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using std::string_view_literals::operator""sv;

namespace {

constexpr unsigned SECTOR_FIRST_BYTES = 2032;
constexpr unsigned SECTOR_NEXT_BYTES = 4064;
constexpr unsigned LOG_BYTES_PER_SECOND = 37;

/**
 * Read one CR/LF-terminated line (terminator stripped).
 * Returns false on timeout; throws on cancel / I/O error.
 */
bool
ReceiveLine(Port &port, char *buffer, size_t length,
            OperationEnvironment &env, TimeoutClock timeout)
{
  assert(length >= 2);

  size_t i = 0;
  while (true) {
    try {
      port.WaitRead(env, timeout.GetRemainingOrZero());
    } catch (const DeviceTimeout &) {
      return false;
    }

    const char c = (char)port.ReadByte();
    if (c == '\r')
      continue;

    if (c == '\n') {
      buffer[i] = '\0';
      return true;
    }

    if (i + 1 < length)
      buffer[i++] = c;
  }
}

[[nodiscard]]
constexpr unsigned
UsableLogBytes(unsigned sector_count) noexcept
{
  if (sector_count < 1)
    sector_count = 1;
  return (sector_count - 1) * SECTOR_NEXT_BYTES + SECTOR_FIRST_BYTES;
}

void
SetEstimatedEndTime(RecordedFlightInfo &flight) noexcept
{
  if (!flight.start_time.IsPlausible()) {
    flight.end_time = BrokenTime::Invalid();
    return;
  }

  const unsigned duration_secs =
    UsableLogBytes(flight.internal.bluefly.sector_count) / LOG_BYTES_PER_SECOND;
  flight.end_time = BrokenTime::FromSecondOfDayChecked(
    flight.start_time.GetSecondOfDay() + duration_secs);
}

[[nodiscard]]
bool
ParseFilenameTime(const char *filename,
                  BrokenDate &date, BrokenTime &time) noexcept
{
  /* BFVddMMyy_HHmmss.igc */
  unsigned day, month, year, hour, minute, second;
  char igc[4];
  if (sscanf(filename, "BFV%2u%2u%2u_%2u%2u%2u.%3s",
             &day, &month, &year, &hour, &minute, &second, igc) != 7 ||
      !StringIsEqualIgnoreCase(igc, "igc"))
    return false;

  date.day = day;
  date.month = month;
  date.year = 2000 + year;
  time.hour = hour;
  time.minute = minute;
  time.second = second;
  return date.IsPlausible() && time.IsPlausible();
}

/** True for bare `*` or a valid NMEA `*CC` checksum. */
[[nodiscard]]
bool
StarTerminatorOK(const char *line, const char *star) noexcept
{
  return star[1] == '\0' || VerifyNMEAChecksum(line);
}

/**
 * $BFH,…,startSector,fileName*CC — last two fields before '*' are
 * sector and filename.
 */
[[nodiscard]]
bool
ParseBFH(const char *line, unsigned &start_sector,
         char *filename, size_t filename_size) noexcept
{
  if (!StringStartsWith(line, "$BFH,"sv) || !VerifyNMEAChecksum(line))
    return false;

  const char *star = strchr(line, '*');
  if (star == nullptr)
    return false;

  const char *name = star;
  while (name > line && name[-1] != ',')
    --name;
  if (name == line || name[-1] != ',')
    return false;

  const char *sector = name - 1;
  while (sector > line && sector[-1] != ',')
    --sector;
  if (sector == line || sector[-1] != ',')
    return false;

  char *endptr;
  start_sector = strtoul(sector, &endptr, 10);
  if (endptr == sector || *endptr != ',')
    return false;

  const size_t name_length = star - name;
  if (name_length == 0 || name_length >= filename_size)
    return false;

  memcpy(filename, name, name_length);
  filename[name_length] = '\0';
  return true;
}

/**
 * $BFF,1* or $BFF,2,<byteSize>* — bare `*` per the manuals, or with
 * an optional NMEA checksum.
 *
 * @return 1 = start, 2 = finish (writes @byte_size), 0 = not a control
 */
[[nodiscard]]
int
ParseBFFControl(const char *line, unsigned &byte_size) noexcept
{
  if (!StringStartsWith(line, "$BFF,"sv))
    return 0;

  const char *star = strchr(line, '*');
  if (star == nullptr || !StarTerminatorOK(line, star))
    return 0;

  if (StringStartsWith(line, "$BFF,1*"sv))
    return 1;

  if (!StringStartsWith(line, "$BFF,2,"sv))
    return 0;

  char *endptr;
  byte_size = strtoul(line + 7, &endptr, 10);
  return (endptr != line + 7 && endptr == star) ? 2 : 0;
}

void
WriteOutputMode(Port &port, unsigned mode, OperationEnvironment &env)
{
  char buffer[16];
  sprintf(buffer, "BOM %u", mode);
  PortWriteNMEA(port, buffer, env);
}

/**
 * Stop the RX thread, silence telemetry (BOM 4), restore previous mode
 * and restart RX on destruction.
 */
class QuietLoggerMode {
  Port &port;
  OperationEnvironment &env;
  unsigned previous_mode;
  bool active = false;

public:
  QuietLoggerMode(Port &_port, unsigned _previous_mode,
                  OperationEnvironment &_env)
    :port(_port), env(_env), previous_mode(_previous_mode)
  {
    port.StopRxThread();
    try {
      port.FullFlush(env, std::chrono::milliseconds(50),
                     std::chrono::milliseconds(300));
      WriteOutputMode(port, 4, env);
      env.Sleep(std::chrono::milliseconds(300));
      port.FullFlush(env, std::chrono::milliseconds(50),
                     std::chrono::milliseconds(300));
      active = true;
    } catch (...) {
      try {
        WriteOutputMode(port, previous_mode, env);
      } catch (...) {
      }

      try {
        port.StartRxThread();
      } catch (...) {
      }

      throw;
    }
  }

  ~QuietLoggerMode() noexcept {
    if (!active)
      return;

    try {
      WriteOutputMode(port, previous_mode, env);
    } catch (...) {
    }

    try {
      port.StartRxThread();
    } catch (...) {
    }
  }

  QuietLoggerMode(const QuietLoggerMode &) = delete;
  QuietLoggerMode &operator=(const QuietLoggerMode &) = delete;
};

} // namespace

bool
BlueFlyDevice::ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env)
{
  QuietLoggerMode quiet{port, GetSettings().output_mode, env};
  PortWriteNMEA(port, "BLF", env);

  char line[256];
  char filename[64];
  bool got_bfh = false;
  TimeoutClock overall{std::chrono::seconds{30}};

  while (!overall.HasExpired()) {
    TimeoutClock line_timeout{got_bfh
                              ? std::chrono::seconds{1}
                              : std::chrono::seconds{5}};

    if (!ReceiveLine(port, line, sizeof(line), env, line_timeout))
      return got_bfh && !flight_list.empty();

    if (env.IsCancelled())
      throw OperationCancelled{};

    unsigned start_sector;
    if (!ParseBFH(line, start_sector, filename, sizeof(filename)))
      continue;

    got_bfh = true;

    if (!flight_list.empty() &&
        StringIsEqual(filename, flight_list.back().internal.bluefly.filename)) {
      auto &last = flight_list.back();
      if (start_sector != last.internal.bluefly.start_sector) {
        /* same name, different start sector → partial/corrupt */
        flight_list.shrink(flight_list.size() - 1);
        continue;
      }

      last.internal.bluefly.sector_count++;
      SetEstimatedEndTime(last);
      continue;
    }

    if (flight_list.full())
      break;

    RecordedFlightInfo &flight = flight_list.append();
    flight.internal.bluefly.start_sector = start_sector;
    flight.internal.bluefly.sector_count = 1;
    CopyString(flight.internal.bluefly.filename,
               sizeof(flight.internal.bluefly.filename), filename);

    if (ParseFilenameTime(filename, flight.date, flight.start_time))
      SetEstimatedEndTime(flight);
    else {
      flight.date = BrokenDate::Invalid();
      flight.start_time = BrokenTime::Invalid();
      flight.end_time = BrokenTime::Invalid();
    }
  }

  return !flight_list.empty();
}

bool
BlueFlyDevice::DownloadFlight(const RecordedFlightInfo &flight,
                              Path path, OperationEnvironment &env)
{
  QuietLoggerMode quiet{port, GetSettings().output_mode, env};

  FileOutputStream fos(path);
  BufferedOutputStream os(fos);

  const unsigned usable_bytes =
    UsableLogBytes(flight.internal.bluefly.sector_count);
  env.SetProgressRange(usable_bytes);

  char cmd[32];
  sprintf(cmd, "BFF %u", flight.internal.bluefly.start_sector);
  PortWriteNMEA(port, cmd, env);

  char line[512];
  bool downloading = false;
  unsigned bytes_written = 0;
  TimeoutClock overall{std::chrono::minutes{10}};

  while (!overall.HasExpired()) {
    TimeoutClock line_timeout{downloading
                              ? std::chrono::seconds{5}
                              : std::chrono::seconds{10}};

    if (!ReceiveLine(port, line, sizeof(line), env, line_timeout))
      return false;

    if (env.IsCancelled())
      throw OperationCancelled{};

    unsigned reported_bytes = 0;
    switch (ParseBFFControl(line, reported_bytes)) {
    case 1:
      downloading = true;
      continue;

    case 2:
      if (!downloading || reported_bytes != bytes_written)
        return false;
      os.Flush();
      fos.Commit();
      return true;
    }

    if (!downloading || line[0] == '$')
      continue;

    os.Write(line);
    os.Write("\r\n");
    bytes_written += strlen(line) + 2;
    env.SetProgressPosition(std::min(bytes_written, usable_bytes));
  }

  return false;
}
