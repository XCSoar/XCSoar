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

#include "Weather/TAF.hpp"
#include "Weather/METAR.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "Net/HTTP/Init.hpp"
#include "ConsoleJobRunner.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Util/Macros.hpp"

#include <cstdio>

static void
DisplayParsedMETAR(const NOAAStore::Item &station)
{
  if (!station.parsed_metar_available) {
    printf("METAR parsing failed!\n");;
    return;
  }

  const ParsedMETAR &parsed = station.parsed_metar;

  printf("Parsed Data:\n");

  if (parsed.name_available)
    _tprintf(_T("Name: %s\n"), parsed.name.c_str());

  if (parsed.location_available)
    _tprintf(_T("Location: %s\n"),
             FormatGeoPoint(parsed.location, CoordinateFormat::DDMMSS).c_str());

  if (parsed.qnh_available) {
    TCHAR buffer[256];
    FormatUserPressure(parsed.qnh, buffer, ARRAY_SIZE(buffer));
    _tprintf(_T("QNH: %s\n"), buffer);
  }

  if (parsed.wind_available) {
    TCHAR buffer[256];
    FormatUserWindSpeed(parsed.wind.norm, buffer, ARRAY_SIZE(buffer));
    _tprintf(_T("Wind: %.0f" DEG " %s\n"),
             (double)parsed.wind.bearing.Degrees(), buffer);
  }

  if (parsed.temperatures_available) {
    TCHAR buffer[256];
    FormatUserTemperature(parsed.temperature, buffer, ARRAY_SIZE(buffer));
    _tprintf(_T("Temperature: %s\n"), buffer);
    FormatUserTemperature(parsed.dew_point, buffer, ARRAY_SIZE(buffer));
    _tprintf(_T("Dew point: %s\n"), buffer);
  }

  if (parsed.visibility_available) {
    TCHAR buffer[256];
    if (parsed.visibility >= 9999)
      _tcscpy(buffer, _T("unlimited"));
    else {
      FormatUserDistanceSmart(parsed.visibility, buffer, ARRAY_SIZE(buffer));
    }
    _tprintf(_T("Visibility: %s\n"), buffer);
  }

  printf("\n");
}

static void
DisplayMETAR(const NOAAStore::Item &station)
{
  if (!station.metar_available) {
    printf("METAR Download failed!\n");;
    return;
  }

  const METAR &metar = station.metar;

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

  DisplayParsedMETAR(station);
}

static void
DisplayTAF(const NOAAStore::Item &station)
{
  if (!station.taf_available) {
    printf("TAF Download failed!\n");;
    return;
  }

  const TAF &taf = station.taf;

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
  NOAAUpdater::Update(store, runner);

  for (auto i = store.begin(), end = store.end(); i != end; ++i) {
    printf("---\n");
    printf("Station %s:\n\n", i->code);
    DisplayMETAR(*i);
    DisplayTAF(*i);
  }

  Net::Deinitialise();

  return 0;
}
