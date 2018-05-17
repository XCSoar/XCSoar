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

#include "Plane/Plane.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/KeyValueFileReader.hpp"
#include "IO/KeyValueFileWriter.hpp"
#include "Units/System.hpp"
#include "TestUtil.hpp"
#include "Util/StringAPI.hxx"
#include "Util/PrintException.hxx"

#include <stdlib.h>

static void
TestReader()
{
  Plane plane;
  PlaneGlue::ReadFile(plane, Path(_T("test/data/D-4449.xcp")));

  ok1(plane.registration == _T("D-4449"));
  ok1(plane.competition_id == _T("TH"));
  ok1(plane.type == _T("Hornet"));
  ok1(plane.handicap == 100);
  ok1(plane.polar_name == _T("Hornet"));
  ok1(equals(plane.polar_shape[0].v,
             Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)));
  ok1(equals(plane.polar_shape[1].v,
             Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)));
  ok1(equals(plane.polar_shape[2].v,
             Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)));
  ok1(equals(plane.polar_shape[0].w, -0.606));
  ok1(equals(plane.polar_shape[1].w, -0.99));
  ok1(equals(plane.polar_shape[2].w, -1.918));
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
  plane.polar_shape[0].v = Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR);
  plane.polar_shape[1].v = Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR);
  plane.polar_shape[2].v = Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR);
  plane.polar_shape[0].w = -0.606;
  plane.polar_shape[1].w = -0.99;
  plane.polar_shape[2].w = -1.918;
  plane.reference_mass = 318;
  plane.dry_mass = 302;
  plane.max_ballast = 100;
  plane.dump_time = 90;
  plane.max_speed = 41.666;
  plane.wing_area = 9.8;

  PlaneGlue::WriteFile(plane, Path(_T("output/D-4449.xcp")));

  FileLineReader reader(Path(_T("output/D-4449.xcp")));

  unsigned count = 0;
  bool found1 = false, found2 = false, found3 = false, found4 = false;
  bool found5 = false, found6 = false, found7 = false, found8 = false;
  bool found9 = false, found10 = false, found11 = false, found12 = false;

  TCHAR *line;
  while ((line = reader.ReadLine()) != NULL) {
    if (StringIsEqual(line, _T("Registration=\"D-4449\"")))
      found1 = true;
    if (StringIsEqual(line, _T("CompetitionID=\"TH\"")))
      found2 = true;
    if (StringIsEqual(line, _T("Type=\"Hornet\"")))
      found3 = true;
    if (StringIsEqual(line, _T("Handicap=\"100\"")))
      found4 = true;
    if (StringIsEqual(line, _T("PolarName=\"Hornet\"")))
      found5 = true;
    if (StringIsEqual(line, _T("PolarInformation=\"80.000,-0.606,120.000,-0.990,160.000,-1.918\"")))
      found6 = true;
    if (StringIsEqual(line, _T("PolarReferenceMass=\"318.000000\"")))
      found7 = true;
    if (StringIsEqual(line, _T("PolarDryMass=\"302.000000\"")))
      found8 = true;
    if (StringIsEqual(line, _T("MaxBallast=\"100.000000\"")))
      found9 = true;
    if (StringIsEqual(line, _T("DumpTime=\"90.000000\"")))
      found10 = true;
    if (StringIsEqual(line, _T("MaxSpeed=\"41.666000\"")))
      found11 = true;
    if (StringIsEqual(line, _T("WingArea=\"9.800000\"")))
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
try {
  plan_tests(30);

  TestReader();
  TestWriter();

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
