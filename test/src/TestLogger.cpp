// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/IGCWriter.hpp"
#include "system/FileUtil.hpp"
#include "NMEA/Info.hpp"
#include "io/FileLineReader.hpp"
#include "TestUtil.hpp"
#include "util/PrintException.hxx"

#include <cassert>
#include <cstdio>

static void
CheckTextFile(Path path, const char *const* expect)
{
  FileLineReaderA reader(path);

  const char *line;
  while ((line = reader.ReadLine()) != NULL) {
    if (*line == 'G')
      break;

    ok1(*expect != NULL);

    if (StringIsEqual(*expect, "HFFTYFRTYPE:", 12)) {
      ok1(StringIsEqual(line, "HFFTYFRTYPE:", 12));
    } else {
      if (strcmp(line, *expect)) {
        printf("# \"%s\" fails to match with \"%s\"\n", line, *expect);
      }
      ok1(strcmp(line, *expect) == 0);
    }

    ++expect;
  }

  ok1(*expect == NULL);
}

static const char *const expect[] = {
  "AXCSFOO",
  "HFDTE040910",
  "HFFXA050",
  "HFPLTPILOTINCHARGE:Pilot Name",
  "HFCM2CREW2:CoPilot Name",
  "HFGTYGLIDERTYPE:ASK-21",
  "HFGIDGLIDERID:D-1234",
  "HFCIDCOMPETITIONID:34",
  "HFFTYFRTYPE:XCSOAR XCSOAR",
  "HFGPS:bar",
  "HFDTM100DATUM:WGS-1984",
  "I023638FXA3940SIU",
  "C040910112233000000000001",
  "C0000000N00000000ETAKEOFF",
  "C5103117N00742367EBERGNEUSTADT",
  "C5037932N01043567ESUHL",
  "C5103117N00742367EBERGNEUSTADT",
  "C0000000N00000000ELANDING",
  "F112233",
  "B1122385103117N00742367EA004900048700000",
  "E112243my_event",
  "B1122435103117N00742367EA004900048700000",
  "LPLTmy_note",
  "F112253121701",
  "B1122535103117S00742367WA004900048700000",
  "B1122585103117S00742367WA004900000000000",
  "B1123035103117S00742367WA004900048700000",
  NULL
};

static void
Run(IGCWriter &writer)
{
  static const GeoPoint home(Angle::Degrees(7.7061111111111114),
                             Angle::Degrees(51.051944444444445));
  static const GeoPoint tp(Angle::Degrees(10.726111111111111),
                           Angle::Degrees(50.6322));

  static NMEAInfo i;
  i.clock = i.time = TimeStamp{std::chrono::seconds{1}};
  i.time_available.Update(i.clock);
  i.date_time_utc.year = 2010;
  i.date_time_utc.month = 9;
  i.date_time_utc.day = 4;
  i.date_time_utc.hour = 11;
  i.date_time_utc.minute = 22;
  i.date_time_utc.second = 33;
  i.location = home;
  i.location_available.Update(i.clock);
  i.gps_altitude = 487;
  i.gps_altitude_available.Update(i.clock);
  i.gps_ellipsoid_altitude = 487;
  i.gps_ellipsoid_altitude_available.Update(i.clock);
  i.ProvidePressureAltitude(490);
  i.ProvideBaroAltitudeTrue(400);

  writer.WriteHeader(i.date_time_utc, "Pilot Name", "CoPilot Name", "ASK-21",
                     "D-1234", "34", "FOO", "bar", false);
  writer.StartDeclaration(i.date_time_utc, 3);
  writer.AddDeclaration(home, "Bergneustadt");
  writer.AddDeclaration(tp, "Suhl");
  writer.AddDeclaration(home, "Bergneustadt");
  writer.EndDeclaration();

  writer.LogEmptyFRecord(i.date_time_utc);

  i.date_time_utc.second += 5;
  writer.LogPoint(i);
  i.date_time_utc.second += 5;
  writer.LogEvent(i, "my_event");
  i.date_time_utc.second += 5;
  writer.LoggerNote("my_note");

  int satellites[GPSState::MAXSATELLITES];
  for (unsigned i = 0; i < GPSState::MAXSATELLITES; ++i)
    satellites[i] = 0;

  satellites[2] = 12;
  satellites[4] = 17;
  satellites[7] = 1;

  i.date_time_utc.second += 5;
  writer.LogFRecord(i.date_time_utc, satellites);

  i.location = GeoPoint(Angle::Degrees(-7.7061111111111114),
                        Angle::Degrees(-51.051944444444445));
  writer.LogPoint(i);

  /* A true-zero ellipsoid height must be written as 00000, not
     replaced by AMSL + geoid. */
  i.date_time_utc.second += 5;
  i.gps_ellipsoid_altitude = 0;
  i.gps_ellipsoid_altitude_available.Update(i.clock);
  writer.LogPoint(i);

  /* Unknown ellipsoid height is derived as AMSL + FakeGeoid (0). */
  i.date_time_utc.minute += 1;
  i.date_time_utc.second = 3;
  i.gps_ellipsoid_altitude_available.Clear();
  writer.LogPoint(i);

  writer.Flush();
  writer.Sign();
  writer.Flush();
}

static void
Run(Path path)
{
  IGCWriter writer(path);
  Run(writer);
}

static void
TestIGCFixApplyEllipsoid()
{
  NMEAInfo basic{};
  basic.clock = TimeStamp{std::chrono::seconds{1}};
  basic.time = TimeStamp{std::chrono::seconds{1}};
  basic.time_available.Update(basic.clock);
  basic.date_time_utc.year = 2010;
  basic.date_time_utc.month = 9;
  basic.date_time_utc.day = 4;
  basic.date_time_utc.hour = 11;
  basic.date_time_utc.minute = 22;
  basic.date_time_utc.second = 33;
  basic.location = GeoPoint(Angle::Degrees(7.7061111111111114),
                             Angle::Degrees(51.051944444444445));
  basic.location_available.Update(basic.clock);
  basic.gps_altitude = 487;
  basic.gps_altitude_available.Update(basic.clock);

  IGCFix fix;
  fix.Clear();
  ok1(fix.Apply(basic));
  ok1(!fix.gps_ellipsoid_altitude_available);
  ok1(fix.gps_ellipsoid_altitude == 0);
  ok1(fix.gps_altitude == 487);

  basic.gps_ellipsoid_altitude = 0;
  basic.gps_ellipsoid_altitude_available.Update(basic.clock);
  ok1(fix.Apply(basic));
  ok1(fix.gps_ellipsoid_altitude_available);
  ok1(fix.gps_ellipsoid_altitude == 0);
}

int main()
try {
  plan_tests(51 + 4 + 7);

  const Path path("output/test/test.igc");
  File::Delete(path);

  Run(path);

  CheckTextFile(path, expect);

  TestIGCFixApplyEllipsoid();

  GRecord grecord;
  grecord.Initialize();
  grecord.VerifyGRecordInFile(path);

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
