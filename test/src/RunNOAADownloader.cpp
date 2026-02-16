// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "Weather/TAF.hpp"
#include "Weather/METAR.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "net/http/Init.hpp"
#include "co/Task.hxx"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/Macros.hpp"
#include "util/PrintException.hxx"

#include <cstdio>

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

static Co::InvokeTask
Run(NOAAStore &store, ProgressListener &progress)
{
  co_await NOAAUpdater::Update(store, *Net::curl, progress);
}

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
    printf("Name: %s\n", parsed.name.c_str());

  if (parsed.location_available)
    printf("Location: %s\n",
             FormatGeoPoint(parsed.location, CoordinateFormat::DDMMSS).c_str());

  if (parsed.qnh_available) {
    char buffer[256];
    FormatUserPressure(parsed.qnh, buffer);
    printf("QNH: %s\n", buffer);
  }

  if (parsed.wind_available) {
    char buffer[256];
    FormatUserWindSpeed(parsed.wind.norm, buffer);
    printf("Wind: %.0f" DEG " %s\n",
             (double)parsed.wind.bearing.Degrees(), buffer);
  }

  if (parsed.temperatures_available) {
    char buffer[256];
    FormatUserTemperature(parsed.temperature, buffer);
    printf("Temperature: %s\n", buffer);
    FormatUserTemperature(parsed.dew_point, buffer);
    printf("Dew point: %s\n", buffer);
  }

  if (parsed.visibility_available) {
    char buffer[256];
    if (parsed.visibility >= 9999)
      strcpy(buffer, "unlimited");
    else {
      FormatUserDistanceSmart(parsed.visibility, buffer);
    }
    printf("Visibility: %s\n", buffer);
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
    printf("%s\n\n", metar.content.c_str());

  if (!metar.decoded.empty())
    printf("%s\n\n", metar.decoded.c_str());

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
    printf("%s\n\n", taf.content.c_str());
}

int
main(int argc, char *argv[])
try {
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

  Instance instance;

  ConsoleOperationEnvironment env;

  printf("Updating METAR and TAF ...\n");
  instance.Run(Run(store, env));

  for (auto i = store.begin(), end = store.end(); i != end; ++i) {
    printf("---\n");
    printf("Station %s:\n\n", i->code);
    DisplayMETAR(*i);
    DisplayTAF(*i);
  }

  return 0;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
