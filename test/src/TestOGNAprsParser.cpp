// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Cloud/OGNAprs.hpp"
#include "Cloud/OGNTraffic.hpp"
#include "Cloud/OGNDeviceDatabase.hpp"

#include "TestUtil.hpp"

#include <cmath>

static OGNXCSFix
MakeFix(uint32_t id, std::chrono::seconds time, double lat_min,
        double lon_min, unsigned track, unsigned knots, int feet,
        int fpm) noexcept
{
  return {
    id,
    time,
    GeoPoint(Angle::Degrees(lon_min / 60), Angle::Degrees(lat_min / 60)),
    track,
    knots / 1.943844,
    int(std::lround(feet * 0.3048)),
    fpm / 196.850394,
  };
}

/**
 * The OGNXCS messages the XCSoar Cloud sends, against the examples of
 * XCSoar_APRS_format.md in glidernet/ogn-aprs-protocol.
 */
static void
TestFormatOGNXCS()
{
  using namespace std::chrono;

  ok1(FormatOGNXCSBeacon(MakeFix(0x06dd5a01, hours{12} + minutes{30},
                                 47 * 60 + 44.298, 12 * 60 + 26.100,
                                 60, 52, 3609, 98)) ==
      "FLRDD5A01>OGNXCS,qOR:/123000h4744.29N/01226.10E'060/052"
      "/A=003609 !W80! id06DD5A01 +098fpm");

  ok1(FormatOGNXCSBeacon(MakeFix(0x06dd5a01,
                                 hours{12} + minutes{55} + seconds{22},
                                 47 * 60 + 56.767, 12 * 60 + 36.793,
                                 95, 93, 5600, -276)) ==
      "FLRDD5A01>OGNXCS,qOR:/125522h4756.76N/01236.79E'095/093"
      "/A=005600 !W73! id06DD5A01 -276fpm");

  ok1(FormatOGNXCSBeacon(MakeFix(0x053e5a01,
                                 hours{13} + minutes{10} + seconds{4},
                                 47 * 60 + 51.727, 12 * 60 + 46.993,
                                 210, 85, 6398, -177)) ==
      "ICA3E5A01>OGNXCS,qOR:/131004h4751.72N/01246.99E'210/085"
      "/A=006398 !W73! id053E5A01 -177fpm");

  /* north is 360, an unknown track and speed 000/000; southern and
     western hemispheres; no climb rate */
  OGNXCSFix fix = MakeFix(0x06dd5a01, hours{1}, -(33 * 60 + 1.5),
                          -(70 * 60 + 30.25), 0, 40, 1000, 0);
  ok1(FormatOGNXCSBeacon(fix).find("S/07030.25W'360/040/") !=
      std::string::npos);
  fix.track_deg.reset();
  fix.ground_speed.reset();
  fix.vertical_speed.reset();
  const std::string unknown = FormatOGNXCSBeacon(fix);
  ok1(unknown.find("'000/000/") != std::string::npos);
  ok1(unknown.find("fpm") == std::string::npos);

  /* not forwarded: address type unknown or OGN, stealth, no tracking */
  fix.id = 0x00dd5a01;
  ok1(FormatOGNXCSBeacon(fix).empty());
  fix.id = 0x07dd5a01;
  ok1(FormatOGNXCSBeacon(fix).empty());
  fix.id = 0x86dd5a01;
  ok1(FormatOGNXCSBeacon(fix).empty());
  fix.id = 0x46dd5a01;
  ok1(FormatOGNXCSBeacon(fix).empty());

  /* below sea level: OGN parsers cannot read a negative altitude */
  fix.id = 0x06dd5a01;
  fix.gps_altitude = -30;
  ok1(FormatOGNXCSBeacon(fix).empty());
  fix.gps_altitude = 0;
  ok1(FormatOGNXCSBeacon(fix).find("/A=000000 ") != std::string::npos);

  /* OGN reads it back as it was sent */
  const std::string line =
    FormatOGNXCSBeacon(MakeFix(0x06dd5a01, hours{12} + minutes{30},
                               47 * 60 + 44.298, 12 * 60 + 26.100,
                               60, 52, 3609, 98));
  const OGNAprsParseResult r = ParseOGNAprsLine(line);
  ok1(r.valid);
  ok1(r.station_id == "FLRDD5A01");
  ok1(r.flarm_valid && r.flarm_id == 0xdd5a01u);
  ok1(r.address_type == 2u);
  ok1(r.altitude_valid && r.altitude == int(std::lround(3609 * 0.3048)));
  ok1(r.track_valid && r.track_deg == 60u);
}

/**
 * The CSV download of ddb.glidernet.org decides which aircraft must
 * not be forwarded.
 */
static void
TestDeviceDatabase()
{
  OGNDeviceDatabase ddb;

  /* nothing is forwarded without a database */
  ok1(!ddb.IsLoaded());
  ok1(!ddb.IsAllowed(0x06dd5a01));

  ok1(!ddb.Parse("not the database\n'F','DD5A01','LS 4','D-1234','XY',"
                 "'N','Y'\n"));
  ok1(!ddb.IsLoaded());

  ok1(ddb.Parse(
    "#DEVICE_TYPE,DEVICE_ID,AIRCRAFT_MODEL,REGISTRATION,CN,TRACKED,"
    "IDENTIFIED\r\n"
    "'F','000000','AS 33Me','OY-XEL','EL','Y','Y'\r\n"
    "'F','DD5A01','LS 4','D-1234','XY','N','Y'\r\n"
    "'I','3E5A01','Duo Discus, XLT','D-KABC','AB','Y','N'\r\n"
    "'O','123456','Paraglider','','','N','N'\r\n"));
  ok1(ddb.IsLoaded());
  ok1(ddb.GetBlockedCount() == 3);

  /* tracked and identified, and an address the database does not
     know */
  ok1(ddb.IsAllowed(0x06000000));
  ok1(ddb.IsAllowed(0x06dd5a02));

  /* not tracked, whatever the aircraft type */
  ok1(!ddb.IsAllowed(0x06dd5a01));
  ok1(!ddb.IsAllowed(0x0add5a01));

  /* not identified, with a comma in the model */
  ok1(!ddb.IsAllowed(0x053e5a01));

  /* the same address as an ICAO address is another aircraft */
  ok1(ddb.IsAllowed(0x05dd5a01));

  /* a download cut off in the middle of a line is rejected, and the
     old database stays */
  ok1(!ddb.Parse(
    "#DEVICE_TYPE,DEVICE_ID,AIRCRAFT_MODEL,REGISTRATION,CN,TRACKED,"
    "IDENTIFIED\n"
    "'F','000000','AS 33Me','OY-XEL','EL','Y','Y'\n"
    "'F','DD5A"));
  ok1(!ddb.IsAllowed(0x06dd5a01));
  ok1(ddb.GetBlockedCount() == 3);
}

int
main()
{
  plan_tests(42 + 18 + 16);

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
  ok1(r.address_type == 1u);
  ok1(!r.track_valid);
  ok1(IsForwardableOgnTraffic(r, sample));

  ok1(OGNPilotIdFromFlarm(0x303030u) == (0x80000000u | 0x303030u));

  OGNTrafficContainer traffic;
  OGNTrafficEntry &e = traffic.Upsert("FLRDD3030F", r.location, r.altitude,
                                      r.altitude_valid,
                                      r.track_deg, r.track_valid,
                                      r.flarm_id, r.flarm_valid,
                                      r.aircraft_type, r.address_type,
                                      r.callsign);
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
  ok1(adsb_r.address_type == 1u);
  ok1(adsb_r.track_valid);
  ok1(adsb_r.track_deg == 85u);
  ok1(IsForwardableOgnTraffic(adsb_r, adsb));

  static constexpr std::string_view flarm_glider =
    "FLRDDDA28B>OGFLR,qAS,bram:/141053h4327.04N/00205.37E'243/039/A=003426 "
    "!W81! id06DDA28B +416fpm +0.0rot 2.2dB 4e -1.1kHz gps3x3";
  const OGNAprsParseResult glider_r = ParseOGNAprsLine(flarm_glider);
  ok1(glider_r.valid);
  ok1(glider_r.flarm_id == 0xDDA28Bu);
  ok1(glider_r.aircraft_type == 1u);
  ok1(glider_r.address_type == 2u);
  ok1(glider_r.track_valid);
  ok1(glider_r.track_deg == 243u);
  ok1(IsForwardableOgnTraffic(glider_r, flarm_glider));

  static constexpr std::string_view ground_sdr =
    "Barton>OGNSDR:/080643h5145.94NI00111.49W&/A=000295";
  const OGNAprsParseResult ground_r = ParseOGNAprsLine(ground_sdr);
  ok1(ground_r.valid);
  ok1(!ground_r.flarm_valid);
  ok1(!IsForwardableOgnTraffic(ground_r, ground_sdr));

  static constexpr std::string_view ground_tracker =
    "OGN395F39>OGNT,qOR:/080629h5145.94N/00111.51W'000/000/A=000321 "
    "!W81! id07395F39 -019fpm +0.0rot 45.8dB 0e +3.2kHz gps4x6";
  const OGNAprsParseResult tracker_r = ParseOGNAprsLine(ground_tracker);
  ok1(tracker_r.valid);
  ok1(tracker_r.flarm_valid);
  ok1(tracker_r.address_type == OGN_ADDRESS_TYPE_TRACKER);
  ok1(!IsForwardableOgnTraffic(tracker_r, ground_tracker));

  static constexpr std::string_view no_altitude =
    "FLRDD3030F>APRS,qAO,FNB:/092742h4825.11NI00947.88E&"
    " !W37! id21303030 +550fpm +2.2rot";
  const OGNAprsParseResult no_alt_r = ParseOGNAprsLine(no_altitude);
  ok1(no_alt_r.valid);
  ok1(!no_alt_r.altitude_valid);

  TestFormatOGNXCS();
  TestDeviceDatabase();

  return exit_status();
}
