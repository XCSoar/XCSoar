// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "Device/RecordedFlight.hpp"
#include "co/Sleep.hxx"
#include "io/FileOutputStream.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Global.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "Operation/ProgressListener.hpp"
#include "system/Path.hpp"
#include "time/BrokenDate.hpp"
#include "time/BrokenTime.hpp"

#include <boost/json.hpp>

#include <fmt/format.h>

#include <stdexcept>

#include <limits.h>
#include <stdio.h>

namespace FlarmHub {

static constexpr long PROBE_CONNECT_TIMEOUT = 3;
static constexpr long PROBE_TIMEOUT = 5;

static constexpr long PROGRESS_TIMEOUT = 5;
/**
 * Ask the Hub this often while a transfer is running.  Co::Sleep()
 * cannot be used for this: #CoarseTimerEvent documents a granularity
 * of about one second.
 */
static constexpr Event::Duration PROGRESS_POLL_INTERVAL =
  std::chrono::milliseconds(250);

/**
 * Parse a timestamp such as "2020-05-01T06:17:48" (local time of the
 * device, without a time zone suffix).
 */
static bool
ParseTimestamp(const char *s, BrokenDate &date, BrokenTime &time) noexcept
{
  unsigned year, month, day, hour, minute, second;
  if (sscanf(s, "%u-%u-%uT%u:%u:%u", &year, &month, &day,
             &hour, &minute, &second) != 6 ||
      year > 9999 || month > 12 || day > 31 ||
      hour > 23 || minute > 59 || second > 59)
    return false;

  date = BrokenDate{year, month, day};
  time = BrokenTime{hour, minute, second};
  return date.IsPlausible();
}

/**
 * Parse a duration such as "00:12:45".
 */
static bool
ParseDuration(const char *s, unsigned &seconds) noexcept
{
  unsigned hour, minute, second;
  if (sscanf(s, "%u:%u:%u", &hour, &minute, &second) != 3 ||
      hour > 999 || minute > 59 || second > 59)
    return false;

  seconds = (hour * 60 + minute) * 60 + second;
  return true;
}

[[gnu::pure]]
static const char *
GetString(const boost::json::object &object, const char *key) noexcept
{
  const auto i = object.find(key);
  if (i == object.end() || !i->value().is_string())
    return nullptr;

  return i->value().get_string().c_str();
}

static bool
ParseFlight(const boost::json::object &object,
            RecordedFlightInfo &flight) noexcept
{
  const auto index = object.find("index");
  const char *const timestamp = GetString(object, "timestamp");
  const char *const duration = GetString(object, "duration");
  if (index == object.end() || !index->value().is_int64() ||
      index->value().get_int64() < 0 ||
      timestamp == nullptr || duration == nullptr)
    return false;

  unsigned seconds;
  if (!ParseTimestamp(timestamp, flight.date, flight.start_time) ||
      !ParseDuration(duration, seconds))
    return false;

  const unsigned end = flight.start_time.GetSecondOfDay() + seconds;
  flight.end_time = BrokenTime::FromSecondOfDayChecked(end);
  flight.internal.flarm_hub = index->value().get_int64();
  return true;
}

static void
ParseFlightList(const boost::json::value &json,
                RecordedFlightList &flight_list)
{
  if (!json.is_array())
    throw std::runtime_error("Malformed FLARM Hub flight list");

  for (const auto &i : json.get_array()) {
    if (flight_list.full())
      break;

    if (!i.is_object())
      continue;

    RecordedFlightInfo flight;
    if (ParseFlight(i.get_object(), flight))
      flight_list.append(flight);
  }
}

/**
 * Give each request its own connection: the Hub is a small embedded
 * HTTP server which stops answering once an idle connection from a
 * previous request is left on it.
 */
static void
SetupRequest(CurlEasy &easy)
{
  Curl::Setup(easy);
  easy.SetOption(CURLOPT_FRESH_CONNECT, 1L);
  easy.SetOption(CURLOPT_FORBID_REUSE, 1L);
}

/**
 * Report the progress of the transfer between the Hub and the FLARM,
 * which is the slow part; the Hub's own response to us is chunked and
 * therefore has no Content-Length.  Asks right away so the size is
 * known as early as possible, and runs until the caller destroys it.
 */
static Co::EagerTask<void>
CoPollProgress(CurlGlobal &curl, std::string url, ProgressListener &progress)
{
  while (true) {
    try {
      CurlEasy easy{url.c_str()};
      SetupRequest(easy);
      easy.SetFailOnError();
      easy.SetTimeout(PROGRESS_TIMEOUT);

      const auto response = co_await Curl::CoRequest(curl, std::move(easy));
      const auto json = boost::json::parse(response.body);
      const auto &object = json.as_object();
      const auto total = object.at("total").to_number<uint_least64_t>();
      const auto loaded = object.at("loaded").to_number<uint_least64_t>();

      /* a finished transfer is usually a leftover from the previous
         one */
      if (total > 0 && total <= UINT_MAX && loaded < total) {
        progress.SetProgressRange(total);
        progress.SetProgressPosition(loaded);
      }
    } catch (...) {
      /* the progress display is not essential */
    }

    co_await Co::FineSleep(curl.GetEventLoop(), PROGRESS_POLL_INTERVAL);
  }
}

static Co::Task<bool>
CoProbe(CurlGlobal &curl, const char *host)
{
  const auto url = fmt::format("http://{}/api/info", host);

  CurlEasy easy{url.c_str()};
  SetupRequest(easy);
  easy.SetFailOnError();
  easy.SetConnectTimeout(PROBE_CONNECT_TIMEOUT);
  easy.SetTimeout(PROBE_TIMEOUT);

  try {
    co_await Curl::CoRequest(curl, std::move(easy));
  } catch (...) {
    co_return false;
  }

  co_return true;
}

Co::Task<bool>
CoReadFlightList(CurlGlobal &curl, const char *host,
                 RecordedFlightList &flight_list,
                 ProgressListener &progress)
{
  if (!co_await CoProbe(curl, host))
    co_return false;

  const auto url = fmt::format("http://{}/api/flarm/igc", host);

  CurlEasy easy{url.c_str()};
  SetupRequest(easy);

  /* register the request before asking for progress, so that the Hub
     sees our connection first */
  Curl::CoRequest request{curl, std::move(easy)};

  const auto poll =
    CoPollProgress(curl, fmt::format("http://{}/api/transfer/progress", host),
                   progress);

  const auto response = co_await request;
  if (response.status != 200)
    throw FmtRuntimeError("Failed to read the FLARM flight list: "
                          "HTTP status {}", response.status);

  ParseFlightList(boost::json::parse(response.body), flight_list);
  co_return true;
}

Co::Task<void>
CoDownloadFlight(CurlGlobal &curl, const char *host, unsigned index,
                 Path path, ProgressListener &progress)
{
  const auto url = fmt::format("http://{}/api/flarm/igc/{}", host, index);

  FileOutputStream file{path};

  CurlEasy easy{url.c_str()};
  SetupRequest(easy);

  /* register the request before asking for progress, so that the Hub
     sees our connection first */
  Curl::CoStreamRequest request{curl, std::move(easy), file};

  const auto poll =
    CoPollProgress(curl, fmt::format("http://{}/api/transfer/progress", host),
                   progress);

  const auto response = co_await request;

  if (response.status != 200)
    throw FmtRuntimeError("Failed to download the IGC file: "
                          "HTTP status {}", response.status);

  file.Commit();
}

} // namespace FlarmHub
