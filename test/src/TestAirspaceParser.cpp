/*
Copyright_License {

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

#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Units/System.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"
#include "Util/PrintException.hxx"
#include "IO/FileLineReader.hpp"
#include "Operation/Operation.hpp"
#include "TestUtil.hpp"

#include <tchar.h>

struct AirspaceClassTestCouple
{
  const TCHAR* name;
  AirspaceClass type;
};

static bool
ParseFile(Path path, Airspaces &airspaces)
{
  FileLineReader reader(path, Charset::AUTO);

  AirspaceParser parser(airspaces);
  NullOperationEnvironment operation;

  if (!ok1(parser.Parse(reader, operation)))
    return false;

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

  const AirspaceClassTestCouple classes[] = {
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

  ok1(airspaces.GetSize() == 24);

  const auto range = airspaces.QueryAll();
  for (auto it = range.begin(); it != range.end(); ++it) {
    const AbstractAirspace &airspace = it->GetAirspace();
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
      ok1(StringIsEqual(_T("130.125 MHz"), airspace.GetRadioText().c_str()));
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
      for (unsigned i = 0; i < ARRAY_SIZE(classes); ++i) {
        if (StringIsEqual(classes[i].name, airspace.GetName()))
          ok1(airspace.GetType() == classes[i].type);
      }
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

  const AirspaceClassTestCouple classes[] = {
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

  const auto range = airspaces.QueryAll();
  for (auto it = range.begin(); it != range.end(); ++it) {
    const AbstractAirspace &airspace = it->GetAirspace();
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
      ok1(StringIsEqual(_T("130.125 MHz"), airspace.GetRadioText().c_str()));
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
      for (unsigned i = 0; i < ARRAY_SIZE(classes); ++i) {
        if (StringIsEqual(classes[i].name, airspace.GetName()))
          ok1(airspace.GetType() == classes[i].type);
      }
    }
  }
}

int main(int argc, char **argv)
try {
  plan_tests(102);

  TestOpenAir();
  TestTNP();

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
