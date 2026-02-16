// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Plane/Plane.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "io/FileLineReader.hpp"
#include "io/KeyValueFileReader.hpp"
#include "io/KeyValueFileWriter.hpp"
#include "Units/System.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"
#include "util/PrintException.hxx"

#include <stdlib.h>
#include <tchar.h>

static void
TestReader()
{
  Plane plane;
  PlaneGlue::ReadFile(plane, Path("test/data/D-4449.xcp"));

  ok1(plane.registration == "D-4449");
  ok1(plane.competition_id == "TH");
  ok1(plane.type == "Hornet");
  ok1(plane.handicap == 100);
  ok1(plane.polar_name == "Hornet");
  ok1(equals(plane.polar_shape[0].v,
             Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)));
  ok1(equals(plane.polar_shape[1].v,
             Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)));
  ok1(equals(plane.polar_shape[2].v,
             Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)));
  ok1(equals(plane.polar_shape[0].w, -0.606));
  ok1(equals(plane.polar_shape[1].w, -0.99));
  ok1(equals(plane.polar_shape[2].w, -1.918));
  ok1(equals(plane.polar_shape.reference_mass, 318));
  ok1(equals(plane.dry_mass_obsolete, 302));
  ok1(equals(plane.empty_mass, 212));
  ok1(equals(plane.max_ballast, 100));
  ok1(plane.dump_time == 90);
  ok1(equals(plane.max_speed, 41.666));
  ok1(equals(plane.wing_area, 9.8));
  ok1(equals(plane.weglide_glider_type, 261));

  plane = Plane();
  PlaneGlue::ReadFile(plane, Path("test/data/D-4449dry.xcp"));
  ok1(equals(plane.empty_mass, 212));
}

static void
TestWriter()
{
  Plane plane;
  plane.registration = "D-4449";
  plane.competition_id = "TH";
  plane.type = "Hornet";
  plane.handicap = 100;
  plane.polar_name = "Hornet";
  plane.polar_shape[0].v = Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR);
  plane.polar_shape[1].v = Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR);
  plane.polar_shape[2].v = Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR);
  plane.polar_shape[0].w = -0.606;
  plane.polar_shape[1].w = -0.99;
  plane.polar_shape[2].w = -1.918;
  plane.polar_shape.reference_mass = 318;
  plane.dry_mass_obsolete = 302;
  plane.empty_mass = 212;
  plane.max_ballast = 100;
  plane.dump_time = 90;
  plane.max_speed = 41.666;
  plane.wing_area = 9.8;
  plane.weglide_glider_type = 160;

  PlaneGlue::WriteFile(plane, Path("output/D-4449.xcp"));

  FileLineReaderA reader(Path("output/D-4449.xcp"));

  unsigned count = 0;
  bool found1 = false, found2 = false, found3 = false, found4 = false;
  bool found5 = false, found6 = false, found7 = false, found8 = false;
  bool found9 = false, found10 = false, found11 = false, found12 = false;
  bool found13 = false, found14 = false;

  char *line;
  while ((line = reader.ReadLine()) != NULL) {
    if (StringIsEqual(line, "Registration=\"D-4449\""))
      found1 = true;
    if (StringIsEqual(line, "CompetitionID=\"TH\""))
      found2 = true;
    if (StringIsEqual(line, "Type=\"Hornet\""))
      found3 = true;
    if (StringIsEqual(line, "Handicap=\"100\""))
      found4 = true;
    if (StringIsEqual(line, "PolarName=\"Hornet\""))
      found5 = true;
    if (StringIsEqual(line, "PolarInformation=\"80.000,-0.606,120.000,-0.990,160.000,-1.918\""))
      found6 = true;
    if (StringIsEqual(line, "PolarReferenceMass=\"318.000000\""))
      found7 = true;
    if (StringIsEqual(line, "PlaneEmptyMass=\"212.000000\""))
      found8 = true;
    if (StringIsEqual(line, "MaxBallast=\"100.000000\""))
      found9 = true;
    if (StringIsEqual(line, "DumpTime=\"90.000000\""))
      found10 = true;
    if (StringIsEqual(line, "MaxSpeed=\"41.666000\""))
      found11 = true;
    if (StringIsEqual(line, "WingArea=\"9.800000\""))
      found12 = true;
    if (StringIsEqual(line, "WeGlideAircraftType=\"160\""))
      found13 = true;
    if (StringIsEqual(line, "PolarDryMass=\"302.000000\""))
      found14 = true;

    count++;
  }

  ok1(count == 14);
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
  ok1(found13);
  ok1(found14);
}

int main()
try {
  plan_tests(35);

  TestReader();
  TestWriter();

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
