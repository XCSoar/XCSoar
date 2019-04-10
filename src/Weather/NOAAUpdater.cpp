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

#include "NOAAUpdater.hpp"
#include "NOAADownloader.hpp"
#include "METARParser.hpp"
#include "Net/HTTP/Session.hpp"
#include "LogFile.hpp"

namespace NOAAUpdater {
  static bool Update(NOAAStore::Item &item,
                     Net::Session &session, JobRunner &runner);
}

bool
NOAAUpdater::Update(NOAAStore::Item &item,
                    Net::Session &session, JobRunner &runner)
{
  bool metar_downloaded = NOAADownloader::DownloadMETAR(item.code, item.metar,
                                                        session, runner);
  if (metar_downloaded) {
    item.metar_available = true;

    if (METARParser::Parse(item.metar, item.parsed_metar))
      item.parsed_metar_available = true;
  }

  bool taf_downloaded = NOAADownloader::DownloadTAF(item.code, item.taf,
                                                    session, runner);
  if (taf_downloaded)
    item.taf_available = true;

  return metar_downloaded && taf_downloaded;
}

bool
NOAAUpdater::Update(NOAAStore &store, JobRunner &runner)
{
  try {
    Net::Session session;

    bool result = true;
    for (auto &i : store) {
      try {
        result = Update(i, session, runner) && result;
      } catch (...) {
        LogError(std::current_exception());
        result = false;
      }
    }

    return result;
  } catch (...) {
    LogError(std::current_exception());
    return false;
  }
}

bool
NOAAUpdater::Update(NOAAStore::Item &item, JobRunner &runner)
{
  try {
    Net::Session session;
    return Update(item, session, runner);
  } catch (...) {
    LogError(std::current_exception());
    return false;
  }
}
