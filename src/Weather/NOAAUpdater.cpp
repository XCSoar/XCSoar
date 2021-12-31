/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
