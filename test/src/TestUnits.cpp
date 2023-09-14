// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Units/Units.hpp"
#include "Atmosphere/Temperature.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(12);

  UnitSetting &config = Units::current;

  config.distance_unit = Unit::METER;
  ok1(equals(Units::ToUserDistance(1), 1));

  config.distance_unit = Unit::KILOMETER;
  ok1(equals(Units::ToUserDistance(1), 0.001));

  config.temperature_unit = Unit::KELVIN;
  ok1(equals(Temperature::FromKelvin(0).ToUser(), 0));
  ok1(equals(Temperature::FromUser(0).ToKelvin(), 0));

  config.temperature_unit = Unit::DEGREES_CELCIUS;
  ok1(equals(Temperature::FromKelvin(0).ToUser(), -273.15));
  ok1(equals(Temperature::FromKelvin(20).ToUser(), -253.15));
  ok1(equals(Temperature::FromUser(0).ToKelvin(), 273.15));
  ok1(equals(Temperature::FromUser(20).ToKelvin(), 293.15));

  config.temperature_unit = Unit::DEGREES_FAHRENHEIT;
  ok1(equals(Temperature::FromKelvin(0).ToUser(), -459.67));
  ok1(equals(Temperature::FromUser(0).ToKelvin(), 255.37));

  ok1(equals(Units::ToUserUnit(1013.25, Unit::TORR), 760));
  ok1(equals(Units::ToUserUnit(1013.25, Unit::INCH_MERCURY), 29.92));

  return exit_status();
}
