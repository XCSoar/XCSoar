/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

bool
NOAAUpdater::Update(NOAAStore &store, JobRunner &runner)
{
  bool result = true;
  for (auto i = store.begin(), e = store.end(); i != e; ++i)
    result = Update(*i, runner) && result;

  return result;
}

bool
NOAAUpdater::Update(NOAAStore::Item &item, JobRunner &runner)
{
  bool metar_downloaded = NOAADownloader::DownloadMETAR(item.code, item.metar,
                                                        runner);
  if (metar_downloaded) {
    item.metar_available = true;

    if (METARParser::Parse(item.metar, item.parsed_metar))
      item.parsed_metar_available = true;
  }

  bool taf_downloaded = NOAADownloader::DownloadTAF(item.code, item.taf, runner);
  if (taf_downloaded)
    item.taf_available = true;

  return metar_downloaded && taf_downloaded;
}
