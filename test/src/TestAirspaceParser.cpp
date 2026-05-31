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
#include "time/BrokenDateTime.hpp"

struct AirspaceClassTestCouple
{
  const char* name;
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
  if (!ParseFile(Path("test/data/airspace/openair.txt"), airspaces)) {
    skip(3, 0, "Failed to parse input file");
    return;
  }

  static constexpr AirspaceClassTestCouple classes[] = {
    { "Class-R-Test", RESTRICTED },
    { "Class-Q-Test", DANGER },
    { "Class-P-Test", PROHIBITED },
    { "Class-CTR-Test", CTR },
    { "Class-A-Test", CLASSA },
    { "Class-B-Test", CLASSB },
    { "Class-C-Test", CLASSC },
    { "Class-D-Test", CLASSD },
    { "Class-GP-Test", NOGLIDER },
    { "Class-W-Test", WAVE },
    { "Class-E-Test", CLASSE },
    { "Class-F-Test", CLASSF },
    { "Class-TMZ-Test", TMZ },
    { "Class-G-Test", CLASSG },
    { "Class-RMZ-Test", RMZ },
  };

  ok1(airspaces.GetSize() == 26);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();
    if (StringIsEqual("Circle-Test", airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::CIRCLE))
        continue;

      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      ok1(equals(circle.GetRadius(), Units::ToSysUnit(5, Unit::NAUTICAL_MILES)));
      ok1(equals(circle.GetReferenceLocation(),
                 Angle::Degrees(1.091667), Angle::Degrees(0.091667)));
    } else if (StringIsEqual("Arc-Test", airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::POLYGON))
        continue;

      const AirspacePolygon &polygon = (const AirspacePolygon &)airspace;
      const SearchPointVector &points = polygon.GetPoints();

      ok1(points.size() == 33);
    } else if (StringIsEqual("Polygon-Test", airspace.GetName())) {
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
    } else if (StringIsEqual("Radio-Test 1 (AR with MHz)", airspace.GetName())) {
      ok1(airspace.GetRadioFrequency() == RadioFrequency::FromMegaKiloHertz(130, 125));
    } else if (StringIsEqual("Radio-Test 2 (AF without MHz)", airspace.GetName())) {
      ok1(airspace.GetRadioFrequency() == RadioFrequency::FromMegaKiloHertz(130, 125));
    } else if (StringIsEqual("Height-Test-1", airspace.GetName())) {
      ok1(airspace.GetBase().IsTerrain());
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude,
                 Units::ToSysUnit(2000, Unit::FEET)));
    } else if (StringIsEqual("Height-Test-2", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 0));
      ok1(airspace.GetTop().reference == AltitudeReference::STD);
      ok1(equals(airspace.GetTop().flight_level, 65));
    } else if (StringIsEqual("Height-Test-3", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(100, Unit::FEET)));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual("Height-Test-4", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual("Height-Test-5", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude, 450));
    } else if (StringIsEqual("Height-Test-6", airspace.GetName())) {
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
  if (!ParseFile(Path("test/data/airspace/tnp.sua"), airspaces)) {
    skip(3, 0, "Failed to parse input file");
    return;
  }

  static constexpr AirspaceClassTestCouple classes[] = {
    { "Class-R-Test", RESTRICTED },
    { "Class-Q-Test", DANGER },
    { "Class-P-Test", PROHIBITED },
    { "Class-CTR-Test", CTR },
    { "Class-A-Test", CLASSA },
    { "Class-B-Test", CLASSB },
    { "Class-C-Test", CLASSC },
    { "Class-D-Test", CLASSD },
    { "Class-W-Test", WAVE },
    { "Class-E-Test", CLASSE },
    { "Class-F-Test", CLASSF },
    { "Class-TMZ-Test", TMZ },
    { "Class-G-Test", CLASSG },
    { "Class-CLASS-C-Test", CLASSC },
    { "Class-MATZ-Test", MATZ },
  };

  ok1(airspaces.GetSize() == 24);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();
    if (StringIsEqual("Circle-Test", airspace.GetName())) {
      if (!ok1(airspace.GetShape() == AbstractAirspace::Shape::CIRCLE))
        continue;

      const AirspaceCircle &circle = (const AirspaceCircle &)airspace;
      ok1(equals(circle.GetRadius(), Units::ToSysUnit(5, Unit::NAUTICAL_MILES)));
      ok1(equals(circle.GetReferenceLocation(),
                 Angle::Degrees(1.091667), Angle::Degrees(0.091667)));
    } else if (StringIsEqual("Polygon-Test", airspace.GetName())) {
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
    } else if (StringIsEqual("Radio-Test", airspace.GetName())) {
      ok1(airspace.GetRadioFrequency() == RadioFrequency::FromMegaKiloHertz(130, 125));
    } else if (StringIsEqual("Height-Test-1", airspace.GetName())) {
      ok1(airspace.GetBase().IsTerrain());
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude,
                 Units::ToSysUnit(2000, Unit::FEET)));
    } else if (StringIsEqual("Height-Test-2", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 0));
      ok1(airspace.GetTop().reference == AltitudeReference::STD);
      ok1(equals(airspace.GetTop().flight_level, 65));
    } else if (StringIsEqual("Height-Test-3", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude_above_terrain,
                 Units::ToSysUnit(100, Unit::FEET)));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual("Height-Test-4", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(airspace.GetTop().altitude > Units::ToSysUnit(30000, Unit::FEET));
    } else if (StringIsEqual("Height-Test-5", airspace.GetName())) {
      ok1(airspace.GetBase().reference == AltitudeReference::AGL);
      ok1(equals(airspace.GetBase().altitude, 100));
      ok1(airspace.GetTop().reference == AltitudeReference::MSL);
      ok1(equals(airspace.GetTop().altitude, 450));
    } else if (StringIsEqual("Height-Test-6", airspace.GetName())) {
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
  if (!ParseFile(Path("test/data/airspace/openair_2.txt"), airspaces)) {
    skip(3, 0, "Failed to parse input file");
    return;
  }

  ok1(airspaces.GetSize() == 4);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();
    if (StringIsEqual("Type-TMA-Test", airspace.GetName())) {
      ok1(AirspaceClass::CLASSE == airspace.GetClass());
      ok1(AirspaceClass::TMA == airspace.GetType());
    } else if (StringIsEqual("Type-GLIDING_SECTOR-Test", airspace.GetName())) {
      ok1(AirspaceClass::UNCLASSIFIED == airspace.GetClass());
      ok1(AirspaceClass::GLIDING_SECTOR == airspace.GetType());
    } else if (StringIsEqual("Type-ASRA-Test", airspace.GetName())) {
      ok1(AirspaceClass::UNCLASSIFIED == airspace.GetClass());
      ok1(AirspaceClass::AERIAL_SPORTING_RECREATIONAL == airspace.GetType());
    } else if (StringIsEqual("NO-Type-Test", airspace.GetName())) {
      ok1(AirspaceClass::CLASSA == airspace.GetClass());
      ok1(AirspaceClass::OTHER == airspace.GetType());
    }
  }
}

static void
TestActivationTime()
{
  Airspaces airspaces;
  if (!ParseFile(Path("test/data/airspace/openair_activation_time.txt"), airspaces)) {
    skip(12, 0, "Failed to parse activation-time input file");
    return;
  }

  // 10 airspace definitions in the file, 2 omitted because of AA NONE → 8 in database
  ok1(airspaces.GetSize() == 8);

  for (const auto &as_ : airspaces.QueryAll()) {
    const AbstractAirspace &airspace = as_.GetAirspace();

    if (StringIsEqual("AA-No-Tag-Test", airspace.GetName())) {
      // Legacy airspace: no AA line → no activation-time metadata, always considered active
      ok1(!airspace.HasActivationTime());
      ok1(airspace.IsActiveToday());

    } else if (StringIsEqual("AA-Active-Test", airspace.GetName())) {
      // Single AA covering [2000-01-01 … 2099-12-31]: always IS_ACTIVE_NOW at test time
      ok1(airspace.HasActivationTime());
      ok1(airspace.IsActiveToday());
      ok1(!airspace.IsNeverActive());
      // Activation-times must be non-empty
      ok1(airspace.GetActivationStartTime().IsPlausible());
      ok1(airspace.GetActivationEndTime().IsPlausible());

    } else if (StringIsEqual("AA-Active-Test-short-time", airspace.GetName())) {
      // Single AA covering [2000-01-01 … 2099-12-31]: always IS_ACTIVE_NOW at test time
      ok1(airspace.HasActivationTime());
      ok1(airspace.IsActiveToday());
      ok1(!airspace.IsNeverActive());
      // Activation-times must be non-empty
      ok1(airspace.GetActivationStartTime().IsPlausible());
      ok1(airspace.GetActivationEndTime().IsPlausible());

    } else if (StringIsEqual("AA-Active-Test-undef-Start", airspace.GetName())) {
      // Single AA covering [undef... 2099-12-31]: always IS_ACTIVE_NOW at test time
      ok1(airspace.HasActivationTime());
      ok1(airspace.IsActiveToday());
      ok1(!airspace.IsNeverActive());
      // Activation-times end must be non-empty
      ok1(!airspace.GetActivationStartTime().IsPlausible());
      ok1(airspace.GetActivationEndTime().IsPlausible());

    } else if (StringIsEqual("AA-Active-Test-undef-End", airspace.GetName())) {
      // Single AA covering [2000-01-01 ... undef]: always IS_ACTIVE_NOW at test time
      ok1(airspace.HasActivationTime());
      ok1(airspace.IsActiveToday());
      ok1(!airspace.IsNeverActive());
      // Activation-times start must be non-empty
      ok1(airspace.GetActivationStartTime().IsPlausible());
      ok1(!airspace.GetActivationEndTime().IsPlausible());

    } else if (StringIsEqual("AA-Past-Test", airspace.GetName())) {
      // Interval entirely in the past → NOT_APPLICABLE → not active today
      ok1(airspace.HasActivationTime());
      ok1(!airspace.IsActiveToday());
      ok1(!airspace.IsNeverActive());

    } else if (StringIsEqual("AA-Malformed-Test", airspace.GetName())) {
      // Malformed value → UNDETECTABLE_CONTENT: AA flag set, but content is
      // implementation-defined; at minimum HasActivationTime must be true
      ok1(airspace.HasActivationTime());

    } else if (StringIsEqual("AA-MultiLine-Active-Test", airspace.GetName())) {
      // Past interval followed by active interval → airspace is active
      ok1(airspace.HasActivationTime());
      ok1(airspace.IsActiveToday());
      ok1(!airspace.IsNeverActive());

    } else {
      // AA-Never-NONE-Test and AA-MultiLine-NeverActive-Test must NOT appear here
      ok1(false); // Unexpected airspace in database
    }
  }
}

int main()
try {
  plan_tests(146);

  TestOpenAir();
  TestTNP();
  TestOpenAirExtended();
  TestActivationTime();

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
