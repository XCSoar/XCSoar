// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspaceInterceptSolution.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Radio/TransponderCode.hpp"
#include "TestUtil.hpp"
#include "time/FloatDuration.hxx"

#include <chrono>
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

static AirspaceInterceptSolution
MakeSolution(double distance, double seconds) noexcept
{
  AirspaceInterceptSolution sol = AirspaceInterceptSolution::Invalid();
  sol.location = GeoPoint{Angle::Degrees(8), Angle::Degrees(50)};
  sol.distance = distance;
  sol.altitude = 500;
  sol.elapsed_time = FloatDuration{seconds};
  return sol;
}

static void
TestAckClearsActiveImmediately()
{
  Airspaces airspaces;
  const auto inside_as = MakeAirspace(AirspaceClass::CLASSC, "Inside");
  const auto near_as = MakeAirspace(AirspaceClass::CLASSD, "Near");
  airspaces.Add(inside_as);
  airspaces.Add(near_as);
  airspaces.Optimise();

  AirspaceWarningConfig config;
  config.SetDefaults();
  AirspaceWarningManager manager(config, airspaces);

  const auto inside_sol = MakeSolution(0, 5);
  const auto near_sol = MakeSolution(2000, 20);
  auto &inside = manager.GetWarning(inside_as);
  inside.UpdateSolution(AirspaceWarning::WARNING_INSIDE, inside_sol);
  auto &nearby = manager.GetWarning(near_as);
  nearby.UpdateSolution(AirspaceWarning::WARNING_GLIDE, near_sol);

  const auto ack = std::chrono::seconds{30};
  const auto step = std::chrono::seconds{1};
  ok1(inside.WarningLive(ack, step));
  ok1(nearby.WarningLive(ack, step));
  ok1(inside.IsActive());
  ok1(nearby.IsActive());

  manager.AcknowledgeInside(inside_as);
  ok1(!inside.IsActive());
  ok1(nearby.IsActive());
  ok1(&manager.begin()->GetAirspace() == near_as.get());

  /* The next warning cycle must not report the ack again. */
  inside.SaveState();
  inside.UpdateSolution(AirspaceWarning::WARNING_INSIDE, inside_sol);
  ok1(inside.WarningLive(ack, step));
  ok1(!inside.ChangedState());
  ok1(!inside.IsActive());

  manager.SetCleared(near_as, true);
  ok1(!nearby.IsActive());
  ok1(&manager.begin()->GetAirspace() == inside_as.get());

  manager.SetCleared(near_as, false);
  ok1(nearby.IsActive());
  ok1(&manager.begin()->GetAirspace() == near_as.get());

  manager.Acknowledge(near_as);
  ok1(!nearby.IsActive());
  ok1(!inside.IsActive());
}

int
main()
{
  plan_tests(31 + 20);

  TestNonNotamAckDayClear();
  TestNotamAckDayClearAfterRefresh();
  TestNotamAckDayClearSiblings();
  TestAckClearsActiveImmediately();

  return exit_status();
}
