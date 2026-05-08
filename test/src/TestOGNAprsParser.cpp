// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Cloud/OGNAprs.hpp"

#include "TestUtil.hpp"

#include <cmath>

int
main()
{
  plan_tests(9);

  const OGNAprsParseResult bad = ParseOGNAprsLine("# comment");
  ok1(!bad.valid);

  static constexpr std::string_view sample =
    "FLRDD3030F>APRS,qAO,FNB:"
    "/092742h4825.11NI00947.88E&/A=003322 !W37! id21303030 +550fpm +2.2rot";

  const OGNAprsParseResult r = ParseOGNAprsLine(sample);
  ok1(r.valid);
  ok1(r.station_id == "FLRDD3030F");
  ok1(r.location.IsValid());
  ok1(equals(r.location.latitude.Degrees(), 48 + 25.11 / 60.));
  ok1(equals(r.location.longitude.Degrees(), 9 + 47.88 / 60.));
  ok1(r.altitude == int(std::lround(3322 * 0.3048)));
  ok1(r.flarm_valid);
  ok1(r.flarm_id == 0x303030u);

  return exit_status();
}
