// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Cloud/OGNAprs.hpp"
#include "Cloud/OGNTraffic.hpp"

#include "TestUtil.hpp"

#include <cmath>

int
main()
{
  plan_tests(14);

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

  ok1(OGNPilotIdFromFlarm(0x303030u) == (0x80000000u | 0x303030u));

  OGNTrafficContainer traffic;
  OGNTrafficEntry &e = traffic.Upsert("FLRDD3030F", r.location, r.altitude,
                                      0, false, r.flarm_id, r.flarm_valid, 0,
                                      r.callsign);
  ok1(e.pilot_id == (0x80000000u | 0x303030u));
  ok1(e.callsign == "303030");

  static constexpr std::string_view adsb =
    R"(ICA4CA6A4>OGADSB,qAS,SpainAVX:/091637h3724.87N\00559.81W^085/165/A=001275 id254CA6A4 -832fpm 0rot fnA3:RYR5VV regEI-DYO modelB738)";
  const OGNAprsParseResult adsb_r = ParseOGNAprsLine(adsb);
  ok1(adsb_r.valid);
  ok1(adsb_r.callsign == "EI-DYO");

  return exit_status();
}
