/*
Copyright_License {

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

#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Units/Units.hpp"
#include "Util/Macros.hpp"
#include "IO/FileLineReader.hpp"
#include "Operation.hpp"
#include "TestUtil.hpp"

#include <tchar.h>

struct AirspaceClassTestCouple
{
  const TCHAR* name;
  AirspaceClass type;
};

static bool
ParseFile(const TCHAR *path, Airspaces &airspaces)
{
  FileLineReader reader(path, ConvertLineReader::AUTO);

  if (!ok1(!reader.error())) {
    skip(1, 0, "Failed to read input file");
    return false;
  }

  AirspaceParser parser(airspaces);
  NullOperationEnvironment operation;

  if (!ok1(parser.Parse(reader, operation)))
    return false;

  airspaces.optimise();
  return true;
}

static void
TestOpenAir()
{
  Airspaces airspaces;
  if (!ParseFile(_T("test/data/airspace/openair.txt"), airspaces)) {
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
  };

  ok1(airspaces.size() == 23);

  for (Airspaces::AirspaceTree::const_iterator it = airspaces.begin();
       it != airspaces.end(); ++it) {
    const AbstractAirspace &airspace = *it->get_airspace();
    if (_tcscmp(_T("Circle-Test"), airspace.GetName()) == 0) {
      if (!ok1(airspace.shape == AbstractAirspace::CIRCLE))
        continue;

      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      ok1(equals(circle.GetRadius(), Units::ToSysUnit(fixed(5), unNauticalMiles)));
      ok1(equals(circle.GetCenter().Latitude, Angle::degrees(fixed(1.091667))));
      ok1(equals(circle.GetCenter().Longitude, Angle::degrees(fixed(0.091667))));
    } else if (_tcscmp(_T("Polygon-Test"), airspace.GetName()) == 0) {
      if (!ok1(airspace.shape == AbstractAirspace::POLYGON))
        continue;

      const AirspacePolygon &polygon = (const AirspacePolygon &)airspace;
      const SearchPointVector &points = polygon.GetPoints();

      if (!ok1(points.size() == 5))
        continue;

      ok1(equals(points[0].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[0].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[1].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[1].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[2].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[2].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[3].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[3].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[4].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[4].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
    } else if (_tcscmp(_T("Radio-Test"), airspace.GetName()) == 0) {
      ok1(_tcscmp(_T("130.125 MHz"), airspace.GetRadioText().c_str()) == 0);
    } else if (_tcscmp(_T("Height-Test-1"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().IsTerrain());
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetTop().altitude,
                 Units::ToSysUnit(fixed(2000), unFeet)));
    } else if (_tcscmp(_T("Height-Test-2"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetBase().altitude, 0));
      ok1(airspace.GetTop().type == AirspaceAltitude::FL);
      ok1(equals(airspace.GetTop().flight_level, 65));
    } else if (_tcscmp(_T("Height-Test-3"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(fixed(100), unFeet)));
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(fixed(30000), unFeet));
    } else if (_tcscmp(_T("Height-Test-4"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(fixed(30000), unFeet));
    } else if (_tcscmp(_T("Height-Test-5"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::AGL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetTop().altitude, 450));
    } else if (_tcscmp(_T("Height-Test-6"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(fixed(50), unFeet)));
      ok1(airspace.GetTop().type == AirspaceAltitude::FL);
      ok1(equals(airspace.GetTop().flight_level, 50));
    } else {
      for (unsigned i = 0; i < ARRAY_SIZE(classes); ++i) {
        if (_tcscmp(classes[i].name, airspace.GetName()) == 0)
          ok1(airspace.GetType() == classes[i].type);
      }
    }
  }
}

static void
TestTNP()
{
  Airspaces airspaces;
  if (!ParseFile(_T("test/data/airspace/tnp.sua"), airspaces)) {
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
  };

  ok1(airspaces.size() == 22);

  for (Airspaces::AirspaceTree::const_iterator it = airspaces.begin();
       it != airspaces.end(); ++it) {
    const AbstractAirspace &airspace = *it->get_airspace();
    if (_tcscmp(_T("Circle-Test"), airspace.GetName()) == 0) {
      if (!ok1(airspace.shape == AbstractAirspace::CIRCLE))
        continue;

      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      ok1(equals(circle.GetRadius(), Units::ToSysUnit(fixed(5), unNauticalMiles)));
      ok1(equals(circle.GetCenter().Latitude, Angle::degrees(fixed(1.091667))));
      ok1(equals(circle.GetCenter().Longitude, Angle::degrees(fixed(0.091667))));
    } else if (_tcscmp(_T("Polygon-Test"), airspace.GetName()) == 0) {
      if (!ok1(airspace.shape == AbstractAirspace::POLYGON))
        continue;

      const AirspacePolygon &polygon = (const AirspacePolygon &)airspace;
      const SearchPointVector &points = polygon.GetPoints();

      if (!ok1(points.size() == 5))
        continue;

      ok1(equals(points[0].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[0].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[1].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[1].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[2].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[2].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[3].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[3].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
      ok1(equals(points[4].get_location().Latitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30))));
      ok1(equals(points[4].get_location().Longitude,
                 Angle::dms(fixed(1), fixed(30), fixed(30)).flipped()));
    } else if (_tcscmp(_T("Radio-Test"), airspace.GetName()) == 0) {
      ok1(_tcscmp(_T("130.125 MHz"), airspace.GetRadioText().c_str()) == 0);
    } else if (_tcscmp(_T("Height-Test-1"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().IsTerrain());
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetTop().altitude,
                 Units::ToSysUnit(fixed(2000), unFeet)));
    } else if (_tcscmp(_T("Height-Test-2"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetBase().altitude, 0));
      ok1(airspace.GetTop().type == AirspaceAltitude::FL);
      ok1(equals(airspace.GetTop().flight_level, 65));
    } else if (_tcscmp(_T("Height-Test-3"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(fixed(100), unFeet)));
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(fixed(30000), unFeet));
    } else if (_tcscmp(_T("Height-Test-4"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(fixed(30000), unFeet));
    } else if (_tcscmp(_T("Height-Test-5"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::AGL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().type == AirspaceAltitude::MSL);
      ok1(equals(airspace.GetTop().altitude, 450));
    } else if (_tcscmp(_T("Height-Test-6"), airspace.GetName()) == 0) {
      ok1(airspace.GetBase().type == AirspaceAltitude::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(fixed(50), unFeet)));
      ok1(airspace.GetTop().type == AirspaceAltitude::FL);
      ok1(equals(airspace.GetTop().flight_level, 50));
    } else {
      for (unsigned i = 0; i < ARRAY_SIZE(classes); ++i) {
        if (_tcscmp(classes[i].name, airspace.GetName()) == 0)
          ok1(airspace.GetType() == classes[i].type);
      }
    }
  }
}

int main(int argc, char **argv)
{
  plan_tests(113);

  TestOpenAir();
  TestTNP();

  return exit_status();
}
