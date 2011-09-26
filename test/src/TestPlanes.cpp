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

#include "Plane/Plane.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/KeyValueFileReader.hpp"
#include "IO/KeyValueFileWriter.hpp"
#include "Units/Units.hpp"
#include "TestUtil.hpp"

static void
TestReader()
{
  Plane plane;
  PlaneGlue::ReadFile(plane, _T("test/data/D-4449.xcp"));

  ok1(plane.registration == _T("D-4449"));
  ok1(plane.competition_id == _T("TH"));
  ok1(plane.type == _T("Hornet"));
  ok1(plane.handicap == 100);
  ok1(plane.polar_name == _T("Hornet"));
  ok1(equals(plane.v1, Units::ToSysUnit(fixed(80), unKiloMeterPerHour)));
  ok1(equals(plane.v2, Units::ToSysUnit(fixed(120), unKiloMeterPerHour)));
  ok1(equals(plane.v3, Units::ToSysUnit(fixed(160), unKiloMeterPerHour)));
  ok1(equals(plane.w1, -0.606));
  ok1(equals(plane.w2, -0.99));
  ok1(equals(plane.w3, -1.918));
  ok1(equals(plane.reference_mass, 318));
  ok1(equals(plane.dry_mass, 302));
  ok1(equals(plane.max_ballast, 100));
  ok1(plane.dump_time == 90);
  ok1(equals(plane.max_speed, 41.666));
  ok1(equals(plane.wing_area, 9.8));
}

static void
TestWriter()
{
  Plane plane;
  plane.registration = _T("D-4449");
  plane.competition_id = _T("TH");
  plane.type = _T("Hornet");
  plane.handicap = 100;
  plane.polar_name = _T("Hornet");
  plane.v1 = Units::ToSysUnit(fixed(80), unKiloMeterPerHour);
  plane.v2 = Units::ToSysUnit(fixed(120), unKiloMeterPerHour);
  plane.v3 = Units::ToSysUnit(fixed(160), unKiloMeterPerHour);
  plane.w1 = fixed(-0.606);
  plane.w2 = fixed(-0.99);
  plane.w3 = fixed(-1.918);
  plane.reference_mass = fixed(318);
  plane.dry_mass = fixed(302);
  plane.max_ballast = fixed(100);
  plane.dump_time = 90;
  plane.max_speed = fixed(41.666);
  plane.wing_area = fixed(9.8);

  PlaneGlue::WriteFile(plane, _T("output/D-4449.xcp"));

  FileLineReader reader(_T("output/D-4449.xcp"));
  if (reader.error())
    return;

  unsigned count = 0;
  bool found1 = false, found2 = false, found3 = false, found4 = false;
  bool found5 = false, found6 = false, found7 = false, found8 = false;
  bool found9 = false, found10 = false, found11 = false, found12 = false;

  TCHAR *line;
  while ((line = reader.read()) != NULL) {
    if (_tcscmp(line, _T("Registration=\"D-4449\"")) == 0)
      found1 = true;
    if (_tcscmp(line, _T("CompetitionID=\"TH\"")) == 0)
      found2 = true;
    if (_tcscmp(line, _T("Type=\"Hornet\"")) == 0)
      found3 = true;
    if (_tcscmp(line, _T("Handicap=\"100\"")) == 0)
      found4 = true;
    if (_tcscmp(line, _T("PolarName=\"Hornet\"")) == 0)
      found5 = true;
    if (_tcscmp(line, _T("PolarInformation=\"80.000,-0.606,120.000,-0.990,160.000,-1.918\"")) == 0)
      found6 = true;
    if (_tcscmp(line, _T("PolarReferenceMass=\"318.000000\"")) == 0)
      found7 = true;
    if (_tcscmp(line, _T("PolarDryMass=\"302.000000\"")) == 0)
      found8 = true;
    if (_tcscmp(line, _T("MaxBallast=\"100.000000\"")) == 0)
      found9 = true;
    if (_tcscmp(line, _T("DumpTime=\"90.000000\"")) == 0)
      found10 = true;
    if (_tcscmp(line, _T("MaxSpeed=\"41.666000\"")) == 0)
      found11 = true;
    if (_tcscmp(line, _T("WingArea=\"9.800000\"")) == 0)
      found12 = true;

    count++;
  }

  ok1(count == 12);
  ok1(found1);
  ok1(found2);
  ok1(found3);
  ok1(found4);
  ok1(found5);
  ok1(found6);
  ok1(found7);
  ok1(found8);
  ok1(found9);
  ok1(found10);
  ok1(found11);
  ok1(found12);
}

int main(int argc, char **argv)
{
  plan_tests(30);

  TestReader();
  TestWriter();

  return exit_status();
}
