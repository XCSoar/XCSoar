// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Device/Error.hpp"
#include "Device/RecordedFlight.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "system/Path.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "Operation/Cancelled.hpp"

#include <algorithm> // for std::min(), std::max()
#include <cstdlib>
#include <cstring>
#include <span>

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
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MessageType::GETRECORDINFO);

  // Send request
  SendFrame(header, {}, env, std::chrono::seconds(1));

  /* wait for an answer and save the payload for further processing;
     the record info frame is ~100 bytes, which can take several
     seconds on a slow link (e.g. a Bluetooth LE bridge) */
  AllocatedArray<std::byte> data;
  uint16_t length;
  const auto ack_result =
    WaitForACKOrNACK(header.sequence_number, data, length,
                     env, std::chrono::seconds(10));

  // If neither ACK nor NACK was received
  if (ack_result != FLARM::MessageType::ACK || length <= 2)
    return false;

  char *record_info = (char *)data.data() + 2;
  return ParseRecordInfo(record_info, flight);
}

FLARM::MessageType
FlarmDevice::SelectFlight(uint8_t record_number, OperationEnvironment &env)
{
  static constexpr unsigned max_attempts = 3;

  std::byte data[] = { static_cast<std::byte>(record_number) };

  /* retry with a fresh frame on timeout; over high-latency links
     such as Bluetooth LE bridges, a single short timeout is not
     always enough, and a lost frame would otherwise silently drop a
     flight from the list */
  for (unsigned attempt = 1;; ++attempt) {
    // Create header for selecting a log record
    FLARM::FrameHeader header =
      PrepareFrameHeader(FLARM::MessageType::SELECTRECORD, std::span{data});

    // Send request
    SendFrame(header, std::span{data}, env, std::chrono::seconds(1));

    // Wait for an answer
    try {
      const auto result = WaitForACKOrNACK(header.sequence_number,
                                           env, std::chrono::seconds(2));
      if (result != FLARM::MessageType::ERROR || attempt >= max_attempts)
        return result;

#ifndef NDEBUG
      LogFormat("FLARM: no answer to SELECTRECORD %u (attempt %u of %u)",
                record_number, attempt, max_attempts);
#endif
    } catch (const DeviceTimeout &) {
      if (attempt >= max_attempts)
        throw;

#ifndef NDEBUG
      LogFormat("FLARM: timeout waiting for SELECTRECORD %u answer"
                " (attempt %u of %u)",
                record_number, attempt, max_attempts);
#endif
    }
  }
}

bool
FlarmDevice::ReadFlightList(RecordedFlightList &flight_list,
                            OperationEnvironment &env)
{
  if (!BinaryMode(env))
    return false;

  // Try to receive flight information until the list is full
  for (uint8_t i = 0; !flight_list.full(); ++i) {
    try {
      FLARM::MessageType ack_result = SelectFlight(i, env);

      // Last record reached -> bail out and return list
      if (ack_result == FLARM::MessageType::NACK)
        break;

      // If neither ACK nor NACK was received
      if (ack_result != FLARM::MessageType::ACK) {
        mode = Mode::UNKNOWN;
        return false;
      }

      RecordedFlightInfo flight_info;
      flight_info.internal.flarm = i;
      if (ReadFlightInfo(flight_info, env))
        flight_list.append(flight_info);
    } catch (const DeviceTimeout &) {  }
  }

  return true;
}

bool
FlarmDevice::DownloadFlight(BufferedOutputStream &os, std::size_t &offset,
                            unsigned &max_progress,
                            OperationEnvironment &env)
{
  /* the FLARM binary protocol cannot seek inside a flight record;
     after a restarted transfer, the part which was already saved is
     discarded instead of being written twice */
  std::size_t skip = offset;

  while (true) {
    // Create header for getting IGC file data
    FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MessageType::GETIGCDATA);

    AllocatedArray<std::byte> data;
    uint16_t length = 0;

    // Send request
    SendFrame(header, {}, env, std::chrono::seconds(1));

    /* wait for an answer and save the payload for further processing;
       an IGC data frame is several hundred bytes, which can take a
       while on a slow link (e.g. a Bluetooth LE bridge) */
    auto result = FLARM::MessageType::ERROR;
    try {
      result = WaitForACKOrNACK(header.sequence_number, data,
                                length, env, std::chrono::seconds(30));
    } catch (const DeviceTimeout &) {
    }

    if (result != FLARM::MessageType::ACK || length <= 3) {
      /* the payload of this frame did not arrive completely; if it
         belongs to the part which a previous attempt has already
         saved, its contents are not needed and the transfer can carry
         on instead of starting over once more */
      if (lost_frame_length > 3 &&
          skip >= std::size_t(lost_frame_length - 3)) {
        skip -= lost_frame_length - 3;

#ifndef NDEBUG
        LogFormat("FLARM: stepping over %u lost bytes which are already"
                  " saved", unsigned(lost_frame_length - 3));
#endif
        continue;
      }

      /* a lost frame must not be requested again: the FLARM does not
         repeat it, it answers the next GETIGCDATA with the *following*
         chunk, and the missing one would leave a hole in the IGC file.
         Fail this session instead; the caller restarts the transfer
         from the beginning and skips the part which was already
         written */
      LogFormat("FLARM: %s to GETIGCDATA after %lu bytes",
                result == FLARM::MessageType::NACK
                ? "NACK"
                : (result == FLARM::MessageType::ACK
                   ? "short answer" : "no answer"),
                (unsigned long)offset);
      return false;
    }

    length -= 3;

    // Read progress (in percent)
    const auto progress = std::min(static_cast<unsigned>(data[2]), 100u);

    /* a restarted transfer sends the flight from the beginning, so
       the FLARM counts from zero again although the file is already
       written up to #offset; keep the progress bar monotonic */
    max_progress = std::max(max_progress, progress);
    env.SetProgressPosition(max_progress);

#ifndef NDEBUG
    LogFormat("FLARM: received IGC data frame (%u bytes, %u%%)",
              unsigned(length), progress);
#endif

    // Read IGC data
    std::span<const std::byte> payload{data.data() + 3, length};

    // The last packet ends with 0x1A
    const bool is_last_packet = !payload.empty() &&
      payload.back() == std::byte{0x1A};
    if (is_last_packet)
      payload = payload.first(payload.size() - 1);

    if (skip > 0) {
      const std::size_t n = std::min(skip, payload.size());
      payload = payload.subspan(n);
      skip -= n;
    }

    if (!payload.empty()) {
      os.Write(payload);
      offset += payload.size();
    }

    if (is_last_packet)
      break;
  }

  return true;
}


bool
FlarmDevice::DownloadFlight(const RecordedFlightInfo &flight,
                            Path path, OperationEnvironment &env)
{
  /* how often to restart the transfer after a mid-transfer failure;
     the equivalent of the resumable LX Nano downloads.  The FLARM
     binary protocol cannot resume at an offset, so the transfer is
     restarted and the already saved part is skipped.
     Every attempt which gets past the previous one makes progress, so
     a link which loses a frame now and then still finishes; the price
     is that each restart reads the flight from the beginning again */
  static constexpr unsigned session_attempts = 20;

  FileOutputStream fos(path);
  BufferedOutputStream os(fos);

  env.SetProgressRange(100);

  std::size_t offset = 0;
  unsigned max_progress = 0;

  for (unsigned attempt = 1;; ++attempt) {
    try {
      if (!BinaryMode(env))
        return false;

      // If no ACK was received -> cancel
      if (SelectFlight(flight.internal.flarm, env) != FLARM::MessageType::ACK)
        return false;

      if (DownloadFlight(os, offset, max_progress, env)) {
        os.Flush();
        fos.Commit();
        return true;
      }
    } catch (OperationCancelled &) {
      mode = Mode::UNKNOWN;
      throw;
    } catch (...) {
      mode = Mode::UNKNOWN;

      if (attempt >= session_attempts)
        throw;

      LogError(std::current_exception(), "FLARM: flight download error");
    }

    if (attempt >= session_attempts)
      break;

    LogFormat("FLARM: flight download attempt %u of %u failed"
              " after %lu bytes, restarting the transfer",
              attempt, session_attempts, (unsigned long)offset);

    /* force BinaryMode() to re-establish the (possibly dead) binary
       session before the next attempt */
    mode = Mode::UNKNOWN;
  }

  LogFormat("FLARM: flight download failed after %u attempts"
            " (%lu bytes received)",
            session_attempts, (unsigned long)offset);

  mode = Mode::UNKNOWN;

  return false;
}
