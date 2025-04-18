// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAAUpdater.hpp"
#include "NOAADownloader.hpp"
#include "METARParser.hpp"
#include "co/Task.hxx"
#include "LogFile.hpp"

Co::Task<bool>
NOAAUpdater::Update(NOAAStore::Item &item,
                    CurlGlobal &curl, ProgressListener &progress) noexcept
{
  bool metar_downloaded = false, taf_downloaded = false;

  try {
    item.metar = co_await NOAADownloader::DownloadMETAR(item.code,
                                                        curl, progress);
    item.metar_available = true;
    metar_downloaded = true;

    if (METARParser::Parse(item.metar, item.parsed_metar))
      item.parsed_metar_available = true;
  } catch (...) {
    LogError(std::current_exception());
  }

  try {
    item.taf = co_await NOAADownloader::DownloadTAF(item.code, curl, progress);
    item.taf_available = true;
    taf_downloaded = true;
  } catch (...) {
    LogError(std::current_exception());
  }

  co_return metar_downloaded && taf_downloaded;
}

Co::Task<bool>
NOAAUpdater::Update(NOAAStore &store, CurlGlobal &curl,
                    ProgressListener &progress) noexcept
{
  bool result = true;
  for (auto &i : store)
    result = co_await Update(i, curl, progress) && result;

  co_return result;
}
