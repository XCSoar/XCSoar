// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Download.hpp"
#include "Forecast.hpp"
#include "Station.hpp"
#include "Geo/GeoPoint.hpp"
#include "LocalPath.hpp"
#include "io/FileOutputStream.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "net/http/Progress.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "time/BrokenDate.hpp"

#include <stdexcept>
#include <stdio.h>

namespace {

/**
 * The station catalogue.  It is served from www.dwd.de rather than
 * the opendata host, and the query string is part of the published
 * address.
 */
constexpr const char *CATALOGUE_URL =
  "https://www.dwd.de/DE/leistungen/opendata/help/stationen"
  "/mosmix_stationskatalog.cfg?view=nasPublication&nn=495490";

/**
 * How much of the .kmz to ask for before falling back to all of it.
 *
 * The timesteps stand at the head of the document and the maximum
 * early among the forecasts, so three kilobytes of the seventeen
 * carried the answer when this was measured.  Eight leaves room for
 * the DWD to shift things about without costing a second request.
 */
constexpr std::size_t PREFIX_BYTES = 8 * 1024;

/**
 * How long to wait for a connection.
 *
 * XCSoar has no way to ask whether there is a network: the download
 * manager's IsAvailable() answers a different question and nothing
 * else knows either.  What it can do is not hang when there is none.
 * curl would otherwise wait five minutes, which before a flight is
 * merely annoying and in the air -- where this dialog is opened to
 * set the QNH -- is worse.  Without a network the attempt usually
 * fails at once because the name does not resolve, and within this at
 * the latest.
 */
constexpr long CONNECT_TIMEOUT_SECONDS = 5;

/** How long the whole transfer may take, once connected. */
constexpr long TRANSFER_TIMEOUT_SECONDS = 20;

/** The catalogue is seventeen times a forecast and needs longer. */
constexpr long CATALOGUE_TIMEOUT_SECONDS = 60;

AllocatedPath
MakeMosmixPath(const char *name) noexcept
{
  const auto directory = MakeCacheDirectory("mosmix");
  if (directory == nullptr)
    return nullptr;

  return AllocatedPath::Build(directory, name);
}

[[gnu::pure]]
bool
IsFresh(Path path, std::chrono::system_clock::duration max_age) noexcept
{
  if (!File::Exists(path))
    return false;

  const auto modified = File::GetLastModification(path);
  if (modified <= std::chrono::system_clock::time_point{})
    /* no usable timestamp; treat it as fresh rather than download the
       catalogue on every single call */
    return true;

  const auto age = std::chrono::system_clock::now() - modified;
  return age >= std::chrono::system_clock::duration::zero() && age < max_age;
}

/**
 * Fetch a URL into memory, giving up quickly when nothing answers.
 *
 * @param range how many bytes from the start to ask for, or zero for
 * all of them
 */
Co::Task<Curl::CoResponse>
CoGet(CurlGlobal &curl, const char *url, std::size_t range,
      long timeout_seconds, ProgressListener &progress)
{
  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  easy.SetFailOnError();
  easy.SetConnectTimeout(CONNECT_TIMEOUT_SECONDS);
  easy.SetTimeout(timeout_seconds);

  char range_buffer[32];
  if (range > 0) {
    snprintf(range_buffer, sizeof(range_buffer), "0-%zu", range - 1);
    easy.SetOption(CURLOPT_RANGE, range_buffer);
  }

  co_return co_await Curl::CoRequest(curl, std::move(easy));
}

} // anonymous namespace

Co::Task<std::optional<Temperature>>
MOSMIX::CoFetchForecastMaximum(CurlGlobal &curl, const GeoPoint &location,
                               const BrokenDate &date,
                               ProgressListener &progress)
{
  if (!location.IsValid())
    throw std::runtime_error("No position to look up a station for");

  const auto catalogue = MakeMosmixPath("stations.cfg");
  if (catalogue == nullptr)
    throw std::runtime_error("No cache directory");

  if (!IsFresh(catalogue, CATALOGUE_MAX_AGE)) {
    /* about 300 kB, and only once a month */
    const auto response = co_await
      CoGet(curl, CATALOGUE_URL, 0, CATALOGUE_TIMEOUT_SECONDS, progress);

    FileOutputStream file{catalogue};
    file.Write(std::as_bytes(std::span{response.body}));
    file.Commit();
  }

  const auto stations = ReadStationCatalogue(catalogue);
  if (stations.empty()) {
    /* Whatever this is, it is not the catalogue -- a captive portal's
       login page arrives with a perfectly good HTTP 200.  Drop it
       instead of letting IsFresh() serve it for the next month, which
       would fail every forecast until then. */
    File::Delete(catalogue);
    throw std::runtime_error("The station catalogue could not be read");
  }

  const auto *station = FindNearestStation(stations, location);
  if (station == nullptr)
    throw std::runtime_error("No station near this position");

  const auto url = MakeForecastURL(station->id.c_str());

  /* Ask for the head of the file first.  It is a ZIP, so this is the
     local header and the beginning of the one deflate stream inside
     -- enough to reach the maximum, and about a fifth of the
     transfer. */
  const auto prefix = co_await
    CoGet(curl, url.c_str(), PREFIX_BYTES, TRANSFER_TIMEOUT_SECONDS,
          progress);

  if (prefix.status == 200)
    /* the server ignored the range and sent everything, which is the
       same bytes a fallback would ask for a second time */
    co_return ReadForecastMaximumFromPrefix(
      std::as_bytes(std::span{prefix.body}), date);

  if (const auto from_prefix = ReadForecastMaximumFromPrefix(
        std::as_bytes(std::span{prefix.body}), date);
      from_prefix.has_value())
    co_return from_prefix;

  /* The head did not carry it: the document is laid out differently
     than it was, or the run does not reach this day -- the last of
     which makes this second request a waste, but only on a day the
     forecast could not have answered anyway. */
  const auto whole = co_await
    CoGet(curl, url.c_str(), 0, TRANSFER_TIMEOUT_SECONDS, progress);

  co_return ReadForecastMaximumFromPrefix(
    std::as_bytes(std::span{whole.body}), date);
}
