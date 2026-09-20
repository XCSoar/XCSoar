// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The document below is shaped like a MOSMIX KML, cut down to the two
 * parts the parser looks at.  The values are the ones the DWD
 * published for Munich on 2026-09-20: TX is stated twice a day and
 * blank in between, and the 18:00 UTC entry -- 298.15 K -- is the
 * maximum forecast for the daylight half of the day.
 */

#include "Weather/MOSMIX/Forecast.hpp"
#include "io/MemoryReader.hxx"
#include "time/BrokenDate.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

#include <string>
#include <string_view>

static constexpr char DOCUMENT[] = R"(<?xml version="1.0"?>
<kml:kml xmlns:dwd="https://opendata.dwd.de/x" xmlns:kml="http://x">
  <dwd:ProductDefinition>
    <dwd:ForecastTimeSteps>
      <dwd:TimeStep>2026-09-20T04:00:00.000Z</dwd:TimeStep>
      <dwd:TimeStep>2026-09-20T06:00:00.000Z</dwd:TimeStep>
      <dwd:TimeStep>2026-09-20T12:00:00.000Z</dwd:TimeStep>
      <dwd:TimeStep>2026-09-20T18:00:00.000Z</dwd:TimeStep>
      <dwd:TimeStep>2026-09-21T06:00:00.000Z</dwd:TimeStep>
      <dwd:TimeStep>2026-09-21T18:00:00.000Z</dwd:TimeStep>
    </dwd:ForecastTimeSteps>
  </dwd:ProductDefinition>
  <kml:Placemark>
    <dwd:Forecast dwd:elementName="PPPP">
      <dwd:value>  95000  95100  95200  95300  95400  95500</dwd:value>
    </dwd:Forecast>
    <dwd:Forecast dwd:elementName="TX">
      <dwd:value>      -  290.95      -  298.15  293.05  290.65</dwd:value>
    </dwd:Forecast>
    <dwd:Forecast dwd:elementName="TTT">
      <dwd:value> 287.0  288.0  296.9  294.3  290.0  289.0</dwd:value>
    </dwd:Forecast>
  </kml:Placemark>
</kml:kml>)";

static std::optional<Temperature>
Read(std::string_view document, unsigned year, unsigned month, unsigned day)
{
  MemoryReader reader{std::as_bytes(std::span{document})};
  return MOSMIX::ReadForecastMaximum(reader,
                                     BrokenDate(year, month, day));
}

int main()
{
  plan_tests(12);

  /* the 18:00 UTC timestep of the day asked for */
  const auto today = Read(DOCUMENT, 2026, 9, 20);
  ok1(today.has_value());
  ok1(today.has_value() && equals(today->ToKelvin(), 298.15));
  ok1(today.has_value() && equals(today->ToCelsius(), 25.0));

  /* the next day is in the same document */
  const auto tomorrow = Read(DOCUMENT, 2026, 9, 21);
  ok1(tomorrow.has_value() && equals(tomorrow->ToKelvin(), 290.65));

  /* a day the run does not reach */
  ok1(!Read(DOCUMENT, 2026, 9, 25).has_value());
  ok1(!Read(DOCUMENT, 2026, 9, 19).has_value());

  /* an implausible date is not a document error */
  ok1(!Read(DOCUMENT, 2026, 13, 40).has_value());

  /* a document whose 18:00 entry is blank: the element is stated only
     twice a day, and a run may not carry this one */
  std::string blank{DOCUMENT};
  {
    const auto at = blank.find("  298.15");
    blank.replace(at, 8, "       -");
  }
  ok1(!Read(blank, 2026, 9, 20).has_value());

  /* no such element */
  std::string no_tx{DOCUMENT};
  no_tx.replace(no_tx.find("\"TX\""), 4, "\"XX\"");
  ok1(!Read(no_tx, 2026, 9, 20).has_value());

  /* truncated part way through */
  ok1(!Read(std::string_view{DOCUMENT}.substr(0, 600), 2026, 9, 20)
       .has_value());
  ok1(!Read("", 2026, 9, 20).has_value());

  /* the URL the station id goes into */
  ok1(MOSMIX::MakeForecastURL("10865") ==
      "https://opendata.dwd.de/weather/local_forecasts/mos/MOSMIX_L"
      "/single_stations/10865/kml/MOSMIX_L_LATEST_10865.kmz");

  return exit_status();
}
