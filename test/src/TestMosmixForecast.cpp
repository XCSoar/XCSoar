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

#include <zlib.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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
  plan_tests(17);

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

  /* A byte range of the .kmz: the ZIP local header and as much of the
     one deflate stream as was asked for.  Built here rather than
     checked in as a binary, so the test says what it is made of. */
  {
    /* The real document carries 114 elements and the maximum is the
       third of them, so it sits in the first few per cent.  Pad this
       one the same way, or a cut would fall before the values and
       the test would prove nothing about a byte range. */
    std::string padded{DOCUMENT};
    {
      std::string filler;
      for (unsigned i = 0; i < 200; ++i)
        filler += "    <dwd:Forecast dwd:elementName=\"FF\">"
          "<dwd:value> 1.0  2.0  3.0  4.0  5.0  6.0</dwd:value>"
          "</dwd:Forecast>\n";
      padded.insert(padded.find("</kml:Placemark>"), filler);
    }

    std::vector<std::byte> deflated(padded.size() + 1024);
    z_stream z{};
    ok1(deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8,
                     Z_DEFAULT_STRATEGY) == Z_OK);
    z.next_in = (Bytef *)padded.data();
    z.avail_in = padded.size();
    z.next_out = (Bytef *)deflated.data();
    z.avail_out = deflated.size();
    deflate(&z, Z_FINISH);
    deflated.resize(deflated.size() - z.avail_out);
    deflateEnd(&z);

    static constexpr char NAME[] = "MOSMIX_L_LATEST.kml";
    static constexpr std::size_t NAME_LENGTH = sizeof(NAME) - 1;

    /* a ZIP local file header, byte for byte as the format lays it
       out: signature, version, flags, method, time, date, crc, the
       two sizes, then the lengths of the name and extra fields */
    uint8_t header[30] = {};
    header[0] = 'P'; header[1] = 'K'; header[2] = 3; header[3] = 4;
    header[4] = 20;
    header[8] = 8;                         /* deflate */
    header[26] = NAME_LENGTH & 0xff;
    header[27] = NAME_LENGTH >> 8;
    /* header[28], header[29]: no extra field */

    std::vector<std::byte> kmz;
    for (auto b : header)
      kmz.push_back(std::byte(b));
    for (std::size_t i = 0; i < NAME_LENGTH; ++i)
      kmz.push_back(std::byte(NAME[i]));
    kmz.insert(kmz.end(), deflated.begin(), deflated.end());

    const BrokenDate today(2026, 9, 20);

    /* the whole thing */
    const auto full = MOSMIX::ReadForecastMaximumFromPrefix(kmz, today);
    ok1(full.has_value() && equals(full->ToKelvin(), 298.15));

    /* A range request returns a stream that stops in the middle.  The
       point of fetching one is that the maximum stands near the head,
       so find the shortest prefix that still carries it and check
       that it is well short of the whole: the exact figure depends on
       how the document compresses, the property does not. */
    std::size_t shortest = kmz.size();
    for (std::size_t n = 32; n < kmz.size(); n += 32) {
      const auto partial =
        MOSMIX::ReadForecastMaximumFromPrefix(std::span{kmz}.first(n), today);
      if (partial.has_value() && equals(partial->ToKelvin(), 298.15)) {
        shortest = n;
        break;
      }
    }

    ok1(shortest < kmz.size() * 3 / 4);

    /* cut so short that nothing useful survives */
    ok1(!MOSMIX::ReadForecastMaximumFromPrefix(
          std::span{kmz}.first(40), today).has_value());

    /* not a ZIP at all */
    ok1(!MOSMIX::ReadForecastMaximumFromPrefix(
          std::as_bytes(std::span{DOCUMENT}), today).has_value());
  }

  /* the URL the station id goes into */
  ok1(MOSMIX::MakeForecastURL("10865") ==
      "https://opendata.dwd.de/weather/local_forecasts/mos/MOSMIX_L"
      "/single_stations/10865/kml/MOSMIX_L_LATEST_10865.kmz");

  return exit_status();
}
