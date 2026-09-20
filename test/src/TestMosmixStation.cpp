// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The lines below are copied verbatim from the DWD station
 * catalogue.  The expected coordinates are not: they were taken from
 * each station's own MOSMIX file, which states its position in
 * decimal degrees, so the two sources are independent.
 */

#include "Weather/MOSMIX/Station.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

#include <array>

static bool
ParsesTo(const char *line, const char *id,
         double longitude, double latitude) noexcept
{
  const auto station = MOSMIX::ParseStationLine(line);
  return station.IsDefined() && StringIsEqual(station.id, id) &&
    fabs(station.location.longitude.Degrees() - longitude) < 0.001 &&
    fabs(station.location.latitude.Degrees() - latitude) < 0.001;
}

int main()
{
  plan_tests(26);

  /* degrees and minutes, not a decimal fraction: 48.50 is 48 degrees
     50 minutes.  Reading it as 48.5 would place Stuttgart 37 km from
     where it stands. */
  ok1(ParsesTo("10739 ---- STUTTGART-SCHN.       48.50    9.12   314",
               "10739", 9.2, 48.8333));
  ok1(ParsesTo("10865 ---- MUENCHEN STADT        48.10   11.32   515",
               "10865", 11.5333, 48.1667));
  ok1(ParsesTo("10382 EDDT BERLIN-TEGEL          52.34   13.19    37",
               "10382", 13.3167, 52.5667));

  /* west of Greenwich and south of the equator */
  ok1(ParsesTo("01001 ENJA JAN MAYEN             70.56   -8.40    10",
               "01001", -8.6667, 70.9333));
  ok1(ParsesTo("70026 PABR BARROW/W.POST W.ROG.  71.18 -156.47    13",
               "70026", -156.7833, 71.3));
  ok1(ParsesTo("89022 ---- HALLEY               -75.27  -26.13    30",
               "89022", -26.2167, -75.45));

  /* not every id is numeric, and some are indented */
  ok1(ParsesTo("P0641 HAGM GAMBELLA               9.51   34.35   526",
               "P0641", 34.5833, 9.85));
  ok1(ParsesTo(" Z936 ---- MOESLARNALM           47.45   12.21  1450",
               "Z936", 12.35, 47.75));

  /* the name may carry spaces; the three numbers at the end may not,
     which is why the line is taken apart from the right */
  ok1(ParsesTo("10184 ---- ST. PETER-ORDING       54.19    8.39     5",
               "10184", 8.65, 54.3167));

  /* the file's own header and separator are not stations */
  ok1(!MOSMIX::ParseStationLine(
        "ID    ICAO NAME                 LAT    LON     ELEV").IsDefined());
  ok1(!MOSMIX::ParseStationLine(
        "----- ---- -------------------- -----  ------- -----").IsDefined());
  ok1(!MOSMIX::ParseStationLine("").IsDefined());
  ok1(!MOSMIX::ParseStationLine(nullptr).IsDefined());
  ok1(!MOSMIX::ParseStationLine("10865 ---- MUENCHEN").IsDefined());

  /* a minutes field above sixty means the file is not in the format
     this parser was written for; inventing a position from it would
     be worse than having none */
  ok1(!MOSMIX::ParseStationLine(
        "10865 ---- MUENCHEN STADT        48.75   11.32   515").IsDefined());
  ok1(!MOSMIX::ParseStationLine(
        "10865 ---- MUENCHEN STADT        48.10   11.99   515").IsDefined());

  /* exactly sixty is un-normalised rather than wrong, and the
     catalogue really does carry one such station */
  ok1(ParsesTo("37531 ---- GORI                  41.60   44.07   132",
               "37531", 44.1167, 42.0));

  /* the rounding above must not turn a legitimate sixty-minute-ish
     binary fraction into a rejection */
  ok1(ParsesTo("99998 ---- SECHZIG                42.60    8.60     0",
               "99998", 8.0 + 60.0 / 60, 43.0));

  /* out of range */
  ok1(!MOSMIX::ParseStationLine(
        "99999 ---- NOWHERE                95.10   11.32     0").IsDefined());

  /* strtod() would take these, and a NaN that got in could not be
     found again afterwards: XCSoar compiles with -ffast-math, under
     which the compiler may assume no value is ever NaN and fold
     std::isnan() to false.  They are turned away by their spelling,
     which holds whatever the compiler assumes. */
  ok1(!MOSMIX::ParseStationLine(
        "99997 ---- NOT A NUMBER             nan   11.32     0").IsDefined());
  ok1(!MOSMIX::ParseStationLine(
        "99996 ---- INFINITY                 inf   11.32     0").IsDefined());
  ok1(!MOSMIX::ParseStationLine(
        "99995 ---- HEXADECIMAL           0x1p+4   11.32     0").IsDefined());
  ok1(!MOSMIX::ParseStationLine(
        "99994 ---- TWO POINTS            48.5.0   11.32     0").IsDefined());

  /* nearest station */
  const std::array<MOSMIX::Station, 3> stations{
    MOSMIX::ParseStationLine(
      "10739 ---- STUTTGART-SCHN.       48.50    9.12   314"),
    MOSMIX::ParseStationLine(
      "10865 ---- MUENCHEN STADT        48.10   11.32   515"),
    MOSMIX::ParseStationLine(
      "10382 EDDT BERLIN-TEGEL          52.34   13.19    37"),
  };

  /* Hahnweide */
  const auto *near_hahnweide = MOSMIX::FindNearestStation(
    stations, GeoPoint(Angle::Degrees(9.43), Angle::Degrees(48.63)));
  ok1(near_hahnweide != nullptr &&
      StringIsEqual(near_hahnweide->id, "10739"));

  /* Lüsse, which is closer to Berlin than to either southern station */
  const auto *near_luesse = MOSMIX::FindNearestStation(
    stations, GeoPoint(Angle::Degrees(12.66), Angle::Degrees(52.14)));
  ok1(near_luesse != nullptr && StringIsEqual(near_luesse->id, "10382"));

  /* without a fix there is nothing to be near */
  ok1(MOSMIX::FindNearestStation(stations, GeoPoint::Invalid()) == nullptr);

  return exit_status();
}
