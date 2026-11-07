// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Renderer/HorizonRenderer.hpp"
#include "TestUtil.hpp"
#include "time/Stamp.hpp"

static MoreData
Sample() noexcept
{
  MoreData data{};
  data.clock = TimeStamp{FloatDuration{1}};
  return data;
}

int
main()
{
  plan_tests(13);

  Units::current.speed_unit = Unit::KILOMETER_PER_HOUR;
  Units::current.altitude_unit = Unit::METER;

  MoreData both = Sample();
  both.airspeed_available.Update(both.clock);
  both.indicated_airspeed = 25;
  both.ground_speed_available.Update(both.clock);
  both.ground_speed = 10;
  both.gps_altitude_available.Update(both.clock);
  both.nav_altitude = 1500;

  const auto ias = HorizonRenderer::Readout(both);
  ok1(ias.speed && equals(*ias.speed, 90));
  ok1(equals(ias.speed_step, 10));
  ok1(ias.altitude && equals(*ias.altitude, 1500));
  ok1(equals(ias.altitude_step, 50));

  MoreData ground = Sample();
  ground.ground_speed_available.Update(ground.clock);
  ground.ground_speed = 10;
  const auto gs = HorizonRenderer::Readout(ground);
  ok1(gs.speed && equals(*gs.speed, 36));
  ok1(equals(gs.speed_step, 10));

  const auto empty = HorizonRenderer::Readout(Sample());
  ok1(!empty.speed);
  ok1(!empty.altitude);

  Units::current.altitude_unit = Unit::FEET;
  const auto feet = HorizonRenderer::Readout(both);
  ok1(feet.altitude &&
      equals(*feet.altitude, Units::ToUserAltitude(1500)));
  ok1(equals(feet.altitude_step, 100));

  Units::current.altitude_unit = Unit::FLIGHT_LEVEL;
  const auto level = HorizonRenderer::Readout(both);
  ok1(equals(level.altitude_step, 1));

  Units::current.speed_unit = Unit::METER_PER_SECOND;
  const auto ms = HorizonRenderer::Readout(both);
  ok1(ms.speed && equals(*ms.speed, 25));
  ok1(equals(ms.speed_step, 5));

  return exit_status();
}
