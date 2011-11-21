/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Weather/TAF.hpp"
#include "Weather/METAR.hpp"
#include "Weather/NOAAStore.hpp"
#include "Net/Init.hpp"
#include "ConsoleJobRunner.hpp"

#include <cstdio>
#include <iostream>

using namespace std;

static void
DisplayMETAR(const NOAAStore::Item &station)
{
  METAR metar;
  if (!station.GetMETAR(metar)) {
    cout << "METAR Download failed!" << endl;
    return;
  }

  printf("%02d.%02d.%04d %02d:%02d:%02d\n",
         (unsigned)metar.last_update.day,
         (unsigned)metar.last_update.month,
         (unsigned)metar.last_update.year,
         (unsigned)metar.last_update.hour,
         (unsigned)metar.last_update.minute,
         (unsigned)metar.last_update.second);

  if (!metar.content.empty())
    _tprintf(_T("%s\n"), metar.content.c_str());

  if (!metar.decoded.empty())
    _tprintf(_T("%s\n"), metar.decoded.c_str());
}

static void
DisplayTAF(const NOAAStore::Item &station)
{
  TAF taf;
  if (!station.GetTAF(taf)) {
    cout << "TAF Download failed!" << endl;
    return;
  }

  printf("%02d.%02d.%04d %02d:%02d:%02d\n",
         (unsigned)taf.last_update.day,
         (unsigned)taf.last_update.month,
         (unsigned)taf.last_update.year,
         (unsigned)taf.last_update.hour,
         (unsigned)taf.last_update.minute,
         (unsigned)taf.last_update.second);

  if (!taf.content.empty()) {
    _tprintf(_T("%s\n"), taf.content.c_str());
  }
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <code>[ <code> ...]" << endl;
    cout << "   <code> is the four letter ICAO code (upper case)" << endl;
    cout << "          of the airport(s) you are requesting" << endl << endl;
    cout << "   Example: " << argv[0] << " EDDL" << endl;
    return 1;
  }

  NOAAStore store;
  for (int i = 1; i < argc; i++) {
    cout << "Adding station " << argv[i] << endl;
    store.AddStation(argv[i]);
  }

  Net::Initialise();

  cout << "Updating METAR and TAF ..." << endl;
  ConsoleJobRunner runner;
  store.Update(runner);

  for (auto i = store.begin(), end = store.end(); i != end; ++i) {
    cout << "---" << endl;
    cout << "Station " << i->code << ":" << endl;
    DisplayMETAR(*i);
    DisplayTAF(*i);
  }

  Net::Deinitialise();

  return 0;
}
