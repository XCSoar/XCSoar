// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "TransponderCode.hpp"
#include "TestUtil.hpp"

#include <memory>
#include <string>

static AirspacePtr
MakeAirspace(AirspaceClass cls,
             const char *name,
             const char *station_name = "")
{
  ok1(name != nullptr);
  ok1(station_name != nullptr);
  if (name == nullptr || station_name == nullptr)
    return nullptr;

  auto airspace = std::make_shared<AirspaceCircle>(
    GeoPoint{Angle::Degrees(8), Angle::Degrees(50)}, 1000);

  AirspaceAltitude base;
  base.reference = AltitudeReference::MSL;
  base.altitude = 0;

  AirspaceAltitude top;
  top.reference = AltitudeReference::MSL;
  top.altitude = 1000;

  airspace->SetProperties(std::string{name}, std::string{station_name},
                          TransponderCode::Null(),
                          cls, cls, base, top);
  return airspace;
}

static void
TestNonNotamAckDayClear()
{
  Airspaces airspaces;
  const auto airspace = MakeAirspace(AirspaceClass::CLASSC, "Class C Test");
  airspaces.Add(airspace);
  airspaces.Optimise();

  AirspaceWarningConfig config;
  config.SetDefaults();

  AirspaceWarningManager manager(config, airspaces);

  manager.AcknowledgeDay(airspace, true);
  ok1(manager.GetAckDay(*airspace));
  const auto *warning = manager.GetWarningPtr(*airspace);
  ok1(warning != nullptr);
  ok1(warning != nullptr && warning->GetAckDay());

  manager.AcknowledgeDay(airspace, false);
  ok1(!manager.GetAckDay(*airspace));
  ok1(manager.GetWarningPtr(*airspace) != nullptr &&
      !manager.GetWarningPtr(*airspace)->GetAckDay());
}

static void
TestNotamAckDayClearAfterRefresh()
{
  Airspaces airspaces;
  const auto airspace = MakeAirspace(AirspaceClass::NOTAM, "NOTAM Test",
                                     "A1234/26");
  airspaces.Add(airspace);
  airspaces.Optimise();

  AirspaceWarningConfig config;
  config.SetDefaults();

  AirspaceWarningManager manager(config, airspaces);

  manager.AcknowledgeDay(airspace, true);
  ok1(manager.GetAckDay(*airspace));
  const auto *warning = manager.GetWarningPtr(*airspace);
  ok1(warning != nullptr);
  ok1(warning != nullptr && warning->GetAckDay());

  const auto refreshed = MakeAirspace(AirspaceClass::NOTAM, "NOTAM Test",
                                      "A1234/26");
  ok1(manager.GetWarningPtr(*refreshed) == nullptr);
  ok1(manager.GetAckDay(*refreshed));

  manager.AcknowledgeDay(refreshed, false);
  ok1(!manager.GetAckDay(*airspace));
  ok1(!manager.GetAckDay(*refreshed));
  ok1(manager.GetWarningPtr(*airspace) != nullptr &&
      !manager.GetWarningPtr(*airspace)->GetAckDay());
}

static void
TestNotamAckDayClearSiblings()
{
  Airspaces airspaces;
  const auto first = MakeAirspace(AirspaceClass::NOTAM, "NOTAM Test 1",
                                  "A2345/26");
  const auto second = MakeAirspace(AirspaceClass::NOTAM, "NOTAM Test 2",
                                   "A2345/26");
  airspaces.Add(first);
  airspaces.Add(second);
  airspaces.Optimise();

  AirspaceWarningConfig config;
  config.SetDefaults();

  AirspaceWarningManager manager(config, airspaces);

  manager.AcknowledgeDay(first, true);
  (void)manager.GetWarning(second);
  auto *first_warning = manager.GetWarningPtr(*first);
  auto *second_warning = manager.GetWarningPtr(*second);

  ok1(first_warning != nullptr);
  ok1(second_warning != nullptr);
  ok1(first_warning != nullptr && first_warning->GetAckDay());
  ok1(second_warning != nullptr && second_warning->GetAckDay());

  manager.AcknowledgeDay(first, false);
  ok1(!manager.GetAckDay(*first));
  ok1(!manager.GetAckDay(*second));
  ok1(first_warning != nullptr && !first_warning->GetAckDay());
  ok1(second_warning != nullptr && !second_warning->GetAckDay());
}

int
main()
{
  plan_tests(31);

  TestNonNotamAckDayClear();
  TestNotamAckDayClearAfterRefresh();
  TestNotamAckDayClearSiblings();

  return exit_status();
}
