/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Logger/IGCWriter.hpp"
#include "OS/FileUtil.hpp"
#include "NMEA/Info.hpp"
#include "IO/FileLineReader.hpp"
#include "TestUtil.hpp"

#include <assert.h>
#include <cstdio>

static void
CheckTextFile(const TCHAR *path, const char *const* expect)
{
  FileLineReaderA reader(path);
  ok1(!reader.error());

  const char *line;
  while ((line = reader.read()) != NULL) {
    if (*line == 'G')
      break;

    ok1(*expect != NULL);

    if (strncmp(*expect, "HFFTYFR TYPE:", 13) == 0) {
      ok1(strncmp(line, "HFFTYFR TYPE:", 13) == 0);
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
  "AXCSfoo",
  "HFDTE040910",
  "HFFXA50",
  "HFPLTPILOT:Pilot Name",
  "HFGTYGLIDERTYPE:ASK-21",
  "HFGIDGLIDERID:D-1234",
  "HFFTYFR TYPE:XCSOAR XCSOAR",
  "HFGPS: bar",
  "HFDTM100Datum: WGS-84",
  "I023638FXA3940SIU",
  "C040910112233000000000001",
  "C00000000N000000000ETAKEOFF",
  "C5103117N00742367EBERGNEUSTADT",
  "C5037932N01043567ESUHL",
  "C5103117N00742367EBERGNEUSTADT",
  "C00000000N000000000ELANDING",
  "B1122385103117N00742367EA004900048700000",
  "E112243my_event",
  "B1122435103117N00742367EA004900048700000",
  "LPLTmy_note",
  "B1122535103117S00742367WA004900048700000",
  NULL
};

int main(int argc, char **argv)
{
  plan_tests(45);

  const TCHAR *path = _T("output/test/test.igc");
  File::Delete(path);

  static const GeoPoint home(Angle::degrees(fixed(7.7061111111111114)),
                             Angle::degrees(fixed(51.051944444444445)));
  static const GeoPoint tp(Angle::degrees(fixed(10.726111111111111)),
                           Angle::degrees(fixed(50.6322)));

  static NMEAInfo i;
  i.clock = fixed_one;
  i.time = fixed(1);
  i.time_available.Update(i.clock);
  i.date_time_utc.year = 2010;
  i.date_time_utc.month = 9;
  i.date_time_utc.day = 4;
  i.date_time_utc.hour = 11;
  i.date_time_utc.minute = 22;
  i.date_time_utc.second = 33;
  i.location = home;
  i.location_available.Update(i.clock);
  i.gps_altitude = fixed(487);
  i.gps_altitude_available.Update(i.clock);
  i.ProvideBaroAltitudeTrue(fixed(490));

  IGCWriter writer(path, i);

  writer.header(i.date_time_utc, _T("Pilot Name"), _T("ASK-21"), _T("D-1234"),
                _T("foo"), _T("bar"));
  writer.StartDeclaration(i.date_time_utc, 3);
  writer.AddDeclaration(home, _T("Bergneustadt"));
  writer.AddDeclaration(tp, _T("Suhl"));
  writer.AddDeclaration(home, _T("Bergneustadt"));
  writer.EndDeclaration();

  i.date_time_utc.second += 5;
  writer.LogPoint(i);
  i.date_time_utc.second += 5;
  writer.LogEvent(i, "my_event");
  i.date_time_utc.second += 5;
  writer.LoggerNote(_T("my_note"));

  i.date_time_utc.second += 5;
  i.location = GeoPoint(Angle::degrees(fixed(-7.7061111111111114)),
                        Angle::degrees(fixed(-51.051944444444445)));
  writer.LogPoint(i);

  writer.finish(i);
  writer.sign();

  CheckTextFile(path, expect);

  GRecord grecord;
  grecord.Init();
  grecord.SetFileName(path);
  ok1(grecord.VerifyGRecordInFile());

  return exit_status();
}
