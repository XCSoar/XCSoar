// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Cloud/OGNAprs.hpp"
#include "Cloud/OGNTraffic.hpp"

#include "TestUtil.hpp"

#include <cmath>

int
main()
{
  plan_tests(27);

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
  ok1(r.altitude_valid);
  ok1(r.flarm_valid);
  ok1(r.flarm_id == 0x303030u);
  ok1(r.aircraft_type == 8u);
  ok1(!r.track_valid);

  ok1(OGNPilotIdFromFlarm(0x303030u) == (0x80000000u | 0x303030u));

  OGNTrafficContainer traffic;
  OGNTrafficEntry &e = traffic.Upsert("FLRDD3030F", r.location, r.altitude,
                                      r.altitude_valid,
                                      r.track_deg, r.track_valid,
                                      r.flarm_id, r.flarm_valid,
                                      r.aircraft_type, r.callsign);
  ok1(e.pilot_id == (0x80000000u | 0x303030u));
  ok1(e.callsign == "303030");

  static constexpr std::string_view adsb =
    R"(ICA4CA6A4>OGADSB,qAS,SpainAVX:/091637h3724.87N\00559.81W^085/165/A=001275 id254CA6A4 -832fpm 0rot fnA3:RYR5VV regEI-DYO modelB738)";
  const OGNAprsParseResult adsb_r = ParseOGNAprsLine(adsb);
  ok1(adsb_r.valid);
  ok1(adsb_r.callsign == "EI-DYO");
  ok1(adsb_r.flarm_valid);
  ok1(adsb_r.flarm_id == 0x4CA6A4u);
  ok1(adsb_r.aircraft_type == 9u);
  ok1(adsb_r.track_valid);
  ok1(adsb_r.track_deg == 85u);

  static constexpr std::string_view flarm_glider =
    "FLRDDDA28B>OGFLR,qAS,bram:/141053h4327.04N/00205.37E'243/039/A=003426 "
    "!W81! id06DDA28B +416fpm +0.0rot 2.2dB 4e -1.1kHz gps3x3";
  const OGNAprsParseResult glider_r = ParseOGNAprsLine(flarm_glider);
  ok1(glider_r.valid);
  ok1(glider_r.flarm_id == 0xDDA28Bu);
  ok1(glider_r.aircraft_type == 1u);
  ok1(glider_r.track_valid);
  ok1(glider_r.track_deg == 243u);

  return exit_status();
}
