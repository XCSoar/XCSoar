/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Weather/METAR.hpp"
#include "Weather/NOAADownloader.hpp"

#include <cstdio>
#include <iostream>
using namespace std;

static bool
Download(const char *code)
{
  METAR metar;
  metar.clear();

  if (!NOAADownloader::DownloadMETAR(code, metar)) {
    cout << "Download failed!" << endl;
    return false;
  }

  cout << "Download successful:" << endl;
  printf("%02d.%02d.%04d %02d:%02d:%02d\n",
         (unsigned)metar.last_update.day,
         (unsigned)metar.last_update.month,
         (unsigned)metar.last_update.year,
         (unsigned)metar.last_update.hour,
         (unsigned)metar.last_update.minute,
         (unsigned)metar.last_update.second);

  if (!metar.content.empty()) {
      _tprintf(metar.content.c_str());
      cout << endl;
  }

  return true;
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <code>" << endl;
    cout << "   <code> is the four letter ICAO code (upper case)" << endl;
    cout << "          of the airport you are requesting" << endl << endl;
    cout << "   Example: " << argv[0] << " EDDL" << endl;
    return 1;
  }

  Download(argv[1]);

  return 0;
}
