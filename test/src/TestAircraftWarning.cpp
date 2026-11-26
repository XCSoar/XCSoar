// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Monitor/AircraftWarning.hpp"
#include "TestUtil.hpp"

int
main()
{
  plan_tests(12);

  SwitchState sw;
  sw.Reset();
  const GlidePolar polar;

  ok1(!IsStallWarning(sw, false, 0));
  sw.vega.inputs = VegaSwitchState::INPUT_MASK_STALL;
  ok1(IsStallWarning(sw, false, 0));
  sw.Reset();
  ok1(IsStallWarning(sw, true, 0.9));
  ok1(!IsStallWarning(sw, true, 0.89));

  ok1(!IsOverspeedWarning(sw, polar, true, true, 100));
  sw.vega.inputs = VegaSwitchState::INPUT_MASK_VELOCITY_NEVEREXCEED;
  ok1(IsOverspeedWarning(sw, polar, false, false, 0));
  sw.Reset();

  sw.airbrake_state = SwitchState::AirbrakeState::NOT_LOCKED;
  ok1(!IsAirbrakeWarning(sw, false));
  ok1(IsAirbrakeWarning(sw, true));
  sw.Reset();

  ok1(!IsGearWarning(sw, true));
  sw.vega.outputs = VegaSwitchState::OUTPUT_MASK_GEAR_LANDING;
  ok1(IsGearWarning(sw, true));
  ok1(!IsGearWarning(sw, false));
  sw.Reset();
  sw.vega.outputs = VegaSwitchState::OUTPUT_MASK_FLAP_LANDING;
  ok1(IsGearWarning(sw, true));

  return exit_status();
}
