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

#ifndef NOAA_DOWNLOADER_HPP
#define NOAA_DOWNLOADER_HPP

struct METAR;
struct TAF;
class JobRunner;

namespace Net {
  class Session;
}

namespace NOAADownloader
{
  /**
   * Downloads a METAR from the NOAA server
   * @param code Four letter code of the airport (upper case)
   * @param metar METAR to write data into
   * @return True if the METAR was downloaded and parsed successfully,
   * otherwise False
   */
  bool DownloadMETAR(const char *code, METAR &metar,
                     Net::Session &session, JobRunner &runner);

  /**
   * Downloads a METAR from the NOAA server
   * @param code Four letter code of the airport (upper case)
   * @param metar METAR to write data into
   * @return True if the METAR was downloaded and parsed successfully,
   * otherwise False
   */
  bool DownloadTAF(const char *code, TAF &taf,
                   Net::Session &session, JobRunner &runner);
};

#endif
