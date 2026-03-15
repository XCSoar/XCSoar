// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Levels.hpp"
#include "co/Task.hxx"
#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"

#include <vector>

class AllocatedPath;
class ProgressListener;
class CurlGlobal;

namespace EDL {

/**
 * Truncate a forecast timestamp to the whole UTC hour used by the EDL service.
 */
BrokenDateTime
NormaliseForecastHour(BrokenDateTime forecast) noexcept;

struct CachedDay {
  BrokenDateTime day;
  unsigned file_count;

  /**
   * Check whether all 24 forecast hours and all supported isobars are cached.
   */
  [[gnu::pure]]
  bool IsComplete() const noexcept;
};

class TileRequest final {
public:
  const BrokenDateTime forecast;
  const unsigned isobar;

  TileRequest(const BrokenDateTime &_forecast,
              unsigned _isobar) noexcept
    :forecast(NormaliseForecastHour(_forecast)), isobar(_isobar) {}

  /**
   * Build the EDL forecast timestamp parameter used in download requests.
   */
  [[gnu::const]]
  StaticString<32>
  BuildForecastParameter() const noexcept;

  /**
   * Build the EDL download URL for this forecast/isobar combination.
   */
  [[gnu::const]]
  StaticString<256>
  BuildDownloadUrl() const noexcept;

  /**
   * Build the cache file name for this forecast/isobar combination.
   */
  [[gnu::const]]
  StaticString<64>
  BuildCacheFileName() const noexcept;

  /**
   * Build the absolute cache path for this forecast/isobar combination.
   */
  AllocatedPath
  BuildCachePath() const noexcept;

  /**
   * Ensure that the MBTiles file for this request exists in the local cache.
   */
  Co::Task<AllocatedPath>
  EnsureDownloaded(CurlGlobal &curl, ProgressListener &progress) const;
};

/**
 * Build the directory path used for cached EDL MBTiles files.
 */
AllocatedPath
BuildCacheDirectory() noexcept;

/**
 * Ensure that a full UTC forecast day is cached for all hours and isobars.
 */
Co::Task<unsigned>
EnsureDayDownloaded(BrokenDateTime day, CurlGlobal &curl,
                    ProgressListener &progress);

/**
 * Enumerate cached UTC forecast days from the EDL tile cache.
 */
std::vector<CachedDay>
ListDownloadedDays() noexcept;

/**
 * Delete all cached forecast days except the specified UTC day.
 */
unsigned
DeleteOtherDownloadedDays(BrokenDateTime keep_day) noexcept;

} // namespace EDL
