// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Atmosphere/Temperature.hpp"
#include "Units.hpp"

Temperature
Temperature::FromUser(double value) noexcept
{
  return FromKelvin(Units::ToSysUnit(value, Units::GetUserTemperatureUnit()));
}

double
Temperature::ToUser() const noexcept
{
  return Units::ToUserUnit(value, Units::GetUserTemperatureUnit());
}
