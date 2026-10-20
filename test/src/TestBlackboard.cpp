// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Blackboard/BlackboardListener.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Blackboard/InterfaceBlackboard.hpp"
#include "Computer/GlideComputerBlackboard.hpp"
#include "Protection.hpp"
#include "Simulator.hpp"
#include "TestUtil.hpp"

/**
 * Merge() reads simulator_data even when this is false. The flag only
 * gates ProcessSimulation().
 */
bool global_simulator_flag;

static unsigned merge_triggers;

void
TriggerMergeThread() noexcept
{
  ++merge_triggers;
}

static GeoPoint
Point(double longitude, double latitude) noexcept
{
  return GeoPoint{Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

static NMEAInfo
At(const GeoPoint &location, double altitude) noexcept
{
  NMEAInfo basic;
  basic.Reset();
  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.location = location;
  basic.location_available.Update(basic.clock);
  basic.gps_altitude = altitude;
  basic.gps_altitude_available.Update(basic.clock);
  return basic;
}

struct CountingListener final : BlackboardListener
{
  unsigned gps_updates        = 0;
  unsigned calculated_updates = 0;
  unsigned computer_updates   = 0;
  unsigned ui_updates         = 0;
  GeoPoint location;
  bool terrain_valid            = false;
  bool set_system_time_from_gps = false;

  void OnGPSUpdate(const MoreData &basic) override
  {
    ++gps_updates;
    location = basic.location;
  }

  void OnCalculatedUpdate([[maybe_unused]] const MoreData &basic,
                          const DerivedInfo &calculated) override
  {
    ++calculated_updates;
    terrain_valid = calculated.terrain_valid;
  }

  void OnComputerSettingsUpdate(const ComputerSettings &settings) override
  {
    ++computer_updates;
    set_system_time_from_gps = settings.set_system_time_from_gps;
  }

  void OnUISettingsUpdate([[maybe_unused]] const UISettings &settings) override
  {
    ++ui_updates;
  }
};

static void
TestCopy() noexcept
{
  DeviceBlackboard device;
  const auto here      = Point(7, 48);
  const auto other     = Point(1, 2);
  const auto replay_at = Point(8, 49);
  const auto sim_at    = Point(9, 50);

  auto location = At(here, 0);
  location.gps_altitude_available.Clear();
  auto altitude = At(Point(0, 0), 1500);
  altitude.location_available.Clear();

  device.LockSetDeviceDataScheduleMerge(0, location);
  device.LockSetDeviceDataScheduleMerge(1, altitude);

  ok1(merge_triggers == 2);
  ok1(device.RealState(0).location_available);
  ok1(equals(device.RealState(0).location, here));
  ok1(!device.RealState(0).gps_altitude_available);
  ok1(device.RealState(1).gps_altitude_available);
  ok1(equals(device.RealState(1).gps_altitude, 1500));
  ok1(!device.RealState(1).location_available);
  ok1(!device.Basic().location_available);
  ok1(!device.Basic().gps_altitude_available);

  {
    const std::lock_guard lock{device.mutex};
    auto dead = At(other, 50);
    dead.alive.Clear();
    device.SetRealState(2) = dead;
    device.Merge();
  }

  ok1(device.Basic().location_available);
  ok1(equals(device.Basic().location, here));
  ok1(device.Basic().gps_altitude_available);
  ok1(equals(device.Basic().gps_altitude, 1500));
  ok1(equals(device.RealState().location, here));
  ok1(equals(device.RealState(0).location, here));
  ok1(equals(device.RealState(2).location, other));

  GlideComputerBlackboard calc;
  calc.ReadBlackboard(device.Basic());
  ok1(equals(calc.Basic().location, here));
  ok1(equals(calc.Basic().gps_altitude, 1500));

  DerivedInfo calculated;
  calculated.Reset();
  calculated.terrain_valid = true;
  {
    const std::lock_guard lock{device.mutex};
    device.ReadBlackboard(calculated);
  }
  ok1(device.Calculated().terrain_valid);

  InterfaceBlackboard interface;
  CountingListener first, second;
  interface.AddListener(first);
  interface.AddListener(second);

  interface.ReadBlackboardBasic(device.Basic());
  ok1(first.gps_updates == 0 && second.gps_updates == 0);

  interface.BroadcastGPSUpdate();
  ok1(first.gps_updates == 1 && second.gps_updates == 1);
  ok1(equals(first.location, here));
  ok1(equals(second.location, here));
  ok1(first.calculated_updates == 0 && first.computer_updates == 0 &&
      first.ui_updates == 0);

  interface.ReadBlackboardCalculated(device.Calculated());
  ok1(first.calculated_updates == 0);

  interface.BroadcastCalculatedUpdate();
  ok1(first.calculated_updates == 1 && second.calculated_updates == 1);
  ok1(first.terrain_valid);
  ok1(first.gps_updates == 1);

  interface.RemoveListener(second);
  interface.BroadcastGPSUpdate();
  ok1(first.gps_updates == 2 && second.gps_updates == 1);

  interface.SetComputerSettings().set_system_time_from_gps = true;
  {
    const std::lock_guard lock{device.mutex};
    device.ReadComputerSettings(interface.GetComputerSettings());
  }
  ok1(device.GetComputerSettings().set_system_time_from_gps);

  interface.BroadcastComputerSettingsUpdate();
  ok1(first.computer_updates == 1 && first.set_system_time_from_gps);
  ok1(second.computer_updates == 0);

  interface.SetComputerSettings().set_system_time_from_gps = false;
  ok1(first.computer_updates == 1 && first.set_system_time_from_gps);

  const unsigned triggers = merge_triggers;
  {
    const std::lock_guard lock{device.mutex};
    device.SetReplayState() = At(replay_at, 800);
    device.Merge();
  }

  ok1(equals(device.Basic().location, replay_at));
  ok1(equals(device.Basic().gps_altitude, 800));
  ok1(equals(device.RealState().location, here));

  device.StopReplay();
  ok1(merge_triggers == triggers + 1);
  ok1(!device.SetReplayState().alive);

  {
    const std::lock_guard lock{device.mutex};
    device.SetSimulatorState() = At(sim_at, 900);
    device.Merge();
  }

  ok1(equals(device.Basic().location, sim_at));
  ok1(equals(device.Basic().gps_altitude, 900));
  ok1(equals(device.RealState().location, here));
}

int
main()
{
  plan_tests(9 + 7 + 3 + 14 + 8);

  TestCopy();
  return exit_status();
}
