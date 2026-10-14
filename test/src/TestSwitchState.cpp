// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/SwitchState.hpp"
#include "Computer/Settings.hpp"
#include "TestUtil.hpp"

static void
TestSpeedCommandPage()
{
  using Mode = SwitchState::FlightMode;

  SwitchState state{};

  state.flight_mode = Mode::CRUISE;
  ok1(state.SpeedCommandPage() == Mode::CRUISE);

  state.flight_mode = Mode::CIRCLING;
  ok1(state.SpeedCommandPage() == Mode::CIRCLING);

  state.flight_mode = Mode::UNKNOWN;
  ok1(state.SpeedCommandPage() == Mode::UNKNOWN);

  state.flight_mode = Mode::CIRCLING;
  state.vega.inputs = VegaSwitchState::INPUT_MASK_SC;
  ok1(state.SpeedCommandPage() == Mode::CRUISE);

  state.vega.inputs = 0;
  state.vega.outputs = VegaSwitchState::OUTPUT_MASK_CIRCLING;
  state.flight_mode = Mode::CRUISE;
  ok1(state.SpeedCommandPage() == Mode::CIRCLING);
}

static void
TestProfileMigration()
{
  using Switch = CirclingSettings::SpeedCommandSwitch;

  ok1(CirclingSettings::SpeedCommandSwitchFromProfile(false, 0, true) ==
      Switch::FLIGHT_MODE);
  ok1(CirclingSettings::SpeedCommandSwitchFromProfile(false, 0, false) ==
      Switch::OFF);
  ok1(CirclingSettings::SpeedCommandSwitchFromProfile(
        true, static_cast<int>(Switch::DISPLAY_MODE), true) ==
      Switch::DISPLAY_MODE);
  ok1(CirclingSettings::SpeedCommandSwitchFromProfile(
        true, static_cast<int>(Switch::OFF), true) ==
      Switch::OFF);
  ok1(CirclingSettings::SpeedCommandSwitchFromProfile(
        true, static_cast<int>(Switch::FLIGHT_MODE), false) ==
      Switch::FLIGHT_MODE);
  ok1(CirclingSettings::SpeedCommandSwitchFromProfile(true, 99, true) ==
      Switch::OFF);
}

int
main()
{
  plan_tests(11);

  TestSpeedCommandPage();
  TestProfileMigration();

  return exit_status();
}
