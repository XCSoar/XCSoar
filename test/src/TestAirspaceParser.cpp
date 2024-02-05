// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Units/System.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "util/PrintException.hxx"
#include "io/FileLineReader.hpp"
#include "Operation/Operation.hpp"
#include "TestUtil.hpp"

#include <tchar.h>

struct AirspaceClassTestCouple
{
  const TCHAR* name;
  AirspaceClass asclass;
};

static bool
ParseFile(Path path, Airspaces &airspaces)
{
  FileReader file_reader{path};
  BufferedReader buffered_reader{file_reader};

  try {
    ParseAirspaceFile(airspaces, buffered_reader);
    ok1(true);
  } catch (...) {
    ok1(false);
    return false;
  }

  airspaces.Optimise();
  return true;
}

static void
TestOpenAir()
{
  Airspaces airspaces;
  if (!ParseFile(Path(_T("test/data/airspace/openair.txt")), airspaces)) {
    skip(3, 0, "Failed to parse input file");
    return;
  }

  static constexpr AirspaceClassTestCouple classes[] = {
    { _T("Class-R-Test"), RESTRICT },
    { _T("Class-Q-Test"), DANGER },
    { _T("Class-P-Test"), PROHIBITED },
    { _T("Class-CTR-Test"), CTR },
    { _T("Class-A-Test"), CLASSA },
    { _T("Class-B-Test"), CLASSB },
    { _T("Class-C-Test"), CLASSC },
    { _T("Class-D-Test"), CLASSD },
    { _T("Class-GP-Test"), NOGLIDER },
    { _T("Class-W-Test"), WAVE },
    { _T("Class-E-Test"), CLASSE },
    { _T("Class-F-Test"), CLASSF },
    { _T("Class-TMZ-Test"), TMZ },
    { _T("Class-G-Test"), CLASSG },
    { _T("Class-RMZ-Test"), RMZ },
  };

  ok1(airspaces.GetSize() == 26);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();
    if (StringIsEqual(_T("Circle-Test"), airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::CIRCLE))
        continue;

      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      ok1(equals(circle.GetRadius(), Units::ToSysUnit(5, Unit::NAUTICAL_MILES)));
      ok1(equals(circle.GetReferenceLocation(),
                 Angle::Degrees(1.091667), Angle::Degrees(0.091667)));
    } else if (StringIsEqual(_T("Arc-Test"), airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::POLYGON))
        continue;

      const AirspacePolygon &polygon = (const AirspacePolygon &)airspace;
      const SearchPointVector &points = polygon.GetPoints();

      ok1(points.size() == 33);
    } else if (StringIsEqual(_T("Polygon-Test"), airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::POLYGON))
        continue;

      const AirspacePolygon &polygon = (const AirspacePolygon &)airspace;
      const SearchPointVector &points = polygon.GetPoints();

      if (!ok1(points.size() == 5))
        continue;

      ok1(equals(points[0].GetLocation(),
                 Angle::DMS(1, 30, 30),
                 Angle::DMS(1, 30, 30, true)));
      ok1(equals(points[1].GetLocation(),
                 Angle::DMS(1, 30, 30),
                 Angle::DMS(1, 30, 30)));
      ok1(equals(points[2].GetLocation(),
                 Angle::DMS(1, 30, 30, true),
                 Angle::DMS(1, 30, 30)));
      ok1(equals(points[3].GetLocation(),
                 Angle::DMS(1, 30, 30, true),
                 Angle::DMS(1, 30, 30, true)));
      ok1(equals(points[4].GetLocation(),
                 Angle::DMS(1, 30, 30),
                 Angle::DMS(1, 30, 30, true)));
    } else if (StringIsEqual(_T("Radio-Test 1 (AR with MHz)"), airspace.GetName())) {
      ok1(airspace.GetRadioFrequency() == RadioFrequency::FromMegaKiloHertz(130, 125));
    } else if (StringIsEqual(_T("Radio-Test 2 (AF without MHz)"), airspace.GetName())) {
      ok1(airspace.GetRadioFrequency() == RadioFrequency::FromMegaKiloHertz(130, 125));
    } else if (StringIsEqual(_T("Height-Test-1"), airspace.GetName())) {
      ok1(airspace.GetBase().IsTerrain());
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude,
                 Units::ToSysUnit(2000, Unit::FEET)));
    } else if (StringIsEqual(_T("Height-Test-2"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 0));
      ok1(airspace.GetTop().reference == AltitudeReference::STD);
      ok1(equals(airspace.GetTop().flight_level, 65));
    } else if (StringIsEqual(_T("Height-Test-3"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(100, Unit::FEET)));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual(_T("Height-Test-4"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual(_T("Height-Test-5"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude, 450));
    } else if (StringIsEqual(_T("Height-Test-6"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(50, Unit::FEET)));
      ok1(airspace.GetTop().reference == AltitudeReference::STD);
      ok1(equals(airspace.GetTop().flight_level, 50));
    } else {
      for (const auto &c : classes)
        if (StringIsEqual(c.name, airspace.GetName()))
          ok1(airspace.GetClass() == c.asclass);
    }
  }
}

static void
TestTNP()
{
  Airspaces airspaces;
  if (!ParseFile(Path(_T("test/data/airspace/tnp.sua")), airspaces)) {
    skip(3, 0, "Failed to parse input file");
    return;
  }

  static constexpr AirspaceClassTestCouple classes[] = {
    { _T("Class-R-Test"), RESTRICT },
    { _T("Class-Q-Test"), DANGER },
    { _T("Class-P-Test"), PROHIBITED },
    { _T("Class-CTR-Test"), CTR },
    { _T("Class-A-Test"), CLASSA },
    { _T("Class-B-Test"), CLASSB },
    { _T("Class-C-Test"), CLASSC },
    { _T("Class-D-Test"), CLASSD },
    { _T("Class-W-Test"), WAVE },
    { _T("Class-E-Test"), CLASSE },
    { _T("Class-F-Test"), CLASSF },
    { _T("Class-TMZ-Test"), TMZ },
    { _T("Class-G-Test"), CLASSG },
    { _T("Class-CLASS-C-Test"), CLASSC },
    { _T("Class-MATZ-Test"), MATZ },
  };

  ok1(airspaces.GetSize() == 24);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();
    if (StringIsEqual(_T("Circle-Test"), airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::CIRCLE))
        continue;

      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      ok1(equals(circle.GetRadius(), Units::ToSysUnit(5, Unit::NAUTICAL_MILES)));
      ok1(equals(circle.GetReferenceLocation(),
                 Angle::Degrees(1.091667), Angle::Degrees(0.091667)));
    } else if (StringIsEqual(_T("Polygon-Test"), airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::POLYGON))
        continue;

      const AirspacePolygon &polygon = (const AirspacePolygon &)airspace;
      const SearchPointVector &points = polygon.GetPoints();

      if (!ok1(points.size() == 5))
        continue;

      ok1(equals(points[0].GetLocation(),
                 Angle::DMS(1, 30, 30),
                 Angle::DMS(1, 30, 30, true)));
      ok1(equals(points[1].GetLocation(),
                 Angle::DMS(1, 30, 30),
                 Angle::DMS(1, 30, 30)));
      ok1(equals(points[2].GetLocation(),
                 Angle::DMS(1, 30, 30, true),
                 Angle::DMS(1, 30, 30)));
      ok1(equals(points[3].GetLocation(),
                 Angle::DMS(1, 30, 30, true),
                 Angle::DMS(1, 30, 30, true)));
      ok1(equals(points[4].GetLocation(),
                 Angle::DMS(1, 30, 30),
                 Angle::DMS(1, 30, 30, true)));
    } else if (StringIsEqual(_T("Radio-Test"), airspace.GetName())) {
      ok1(airspace.GetRadioFrequency() == RadioFrequency::FromMegaKiloHertz(130, 125));
    } else if (StringIsEqual(_T("Height-Test-1"), airspace.GetName())) {
      ok1(airspace.GetBase().IsTerrain());
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude,
                 Units::ToSysUnit(2000, Unit::FEET)));
    } else if (StringIsEqual(_T("Height-Test-2"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 0));
      ok1(airspace.GetTop().reference == AltitudeReference::STD);
      ok1(equals(airspace.GetTop().flight_level, 65));
    } else if (StringIsEqual(_T("Height-Test-3"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(100, Unit::FEET)));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual(_T("Height-Test-4"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual(_T("Height-Test-5"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude, 450));
    } else if (StringIsEqual(_T("Height-Test-6"), airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(50, Unit::FEET)));
      ok1(airspace.GetTop().reference == AltitudeReference::STD);
      ok1(equals(airspace.GetTop().flight_level, 50));
    } else {
      for (const auto &c : classes)
        if (StringIsEqual(c.name, airspace.GetName()))
          ok1(airspace.GetClass() == c.asclass);
    }
  }
}

static void
TestOpenAirExtended()
{
  Airspaces airspaces;
  if (!ParseFile(Path(_T("test/data/airspace/openair_extended.txt")), airspaces)) {
    skip(3, 0, "Failed to parse input file");
    return;
  }

  ok1(airspaces.GetSize() == 2);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();
    if (StringIsEqual(_T("Type-TMA-Test"), airspace.GetName())) {
      ok1(StringIsEqual(_T("TMA"), airspace.GetType()));
    } else if (StringIsEqual(_T("Type-GLIDING_SECTOR-Test"), airspace.GetName())) {
      ok1(StringIsEqual(_T("GLIDING_SECTOR"), airspace.GetType()));
    }
  }
}

int main()
try {
  plan_tests(109);

  TestOpenAir();
  TestTNP();
  // todo -- test more AY tags!!
  TestOpenAirExtended();

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
