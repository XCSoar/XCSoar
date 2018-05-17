/* Copyright_License {

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

#include "IGC/IGCWriter.hpp"
#include "OS/FileUtil.hpp"
#include "NMEA/Info.hpp"
#include "IO/FileLineReader.hpp"
#include "TestUtil.hpp"
#include "Util/PrintException.hxx"

#include <assert.h>
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
  i.clock = 1;
  i.time = 1;
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
  i.ProvidePressureAltitude(490);
  i.ProvideBaroAltitudeTrue(400);

  writer.WriteHeader(i.date_time_utc, _T("Pilot Name"), _T("ASK-21"),
                     _T("D-1234"), _T("34"), "FOO", _T("bar"), false);
  writer.StartDeclaration(i.date_time_utc, 3);
  writer.AddDeclaration(home, _T("Bergneustadt"));
  writer.AddDeclaration(tp, _T("Suhl"));
  writer.AddDeclaration(home, _T("Bergneustadt"));
  writer.EndDeclaration();

  writer.LogEmptyFRecord(i.date_time_utc);

  i.date_time_utc.second += 5;
  writer.LogPoint(i);
  i.date_time_utc.second += 5;
  writer.LogEvent(i, "my_event");
  i.date_time_utc.second += 5;
  writer.LoggerNote(_T("my_note"));

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

int main(int argc, char **argv)
try {
  plan_tests(49);

  const Path path(_T("output/test/test.igc"));
  File::Delete(path);

  Run(path);

  CheckTextFile(path, expect);

  GRecord grecord;
  grecord.Initialize();
  grecord.VerifyGRecordInFile(path);

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
