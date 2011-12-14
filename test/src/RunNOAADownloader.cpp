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


static void
DisplayMETAR(const NOAAStore::Item &station)
{
  METAR metar;
  if (!station.GetMETAR(metar)) {
    printf("METAR Download failed!\n");;
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
    _tprintf(_T("%s\n\n"), metar.content.c_str());

  if (!metar.decoded.empty())
    _tprintf(_T("%s\n\n"), metar.decoded.c_str());
}

static void
DisplayTAF(const NOAAStore::Item &station)
{
  TAF taf;
  if (!station.GetTAF(taf)) {
    printf("TAF Download failed!\n");;
    return;
  }

  printf("%02d.%02d.%04d %02d:%02d:%02d\n",
         (unsigned)taf.last_update.day,
         (unsigned)taf.last_update.month,
         (unsigned)taf.last_update.year,
         (unsigned)taf.last_update.hour,
         (unsigned)taf.last_update.minute,
         (unsigned)taf.last_update.second);

  if (!taf.content.empty())
    _tprintf(_T("%s\n\n"), taf.content.c_str());
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: %s <code>[ <code> ...]\n", argv[0]);
    printf("   <code> is the four letter ICAO code (upper case)\n");
    printf("          of the airport(s) you are requesting\n\n");
    printf("   Example: %s EDDL\n", argv[0]);
    return 1;
  }

  NOAAStore store;
  for (int i = 1; i < argc; i++) {
    printf("Adding station %s\n", argv[i]);
    store.AddStation(argv[i]);
  }

  Net::Initialise();

  printf("Updating METAR and TAF ...\n");
  ConsoleJobRunner runner;
  store.Update(runner);

  for (auto i = store.begin(), end = store.end(); i != end; ++i) {
    printf("---\n");
    printf("Station %s:\n\n", i->code);
    DisplayMETAR(*i);
    DisplayTAF(*i);
  }

  Net::Deinitialise();

  return 0;
}
