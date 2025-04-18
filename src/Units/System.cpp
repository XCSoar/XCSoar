// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Units/System.hpp"
#include "Units/Descriptor.hpp"

double
Units::ToUserUnit(double value, Unit unit) noexcept
{
  const UnitDescriptor *ud = &unit_descriptors[(unsigned)unit];
  return value * ud->factor_to_user + ud->offset_to_user;
}

double
Units::ToSysUnit(double value, Unit unit) noexcept
{
  const UnitDescriptor *ud = &unit_descriptors[(unsigned)unit];
  return (value - ud->offset_to_user) / ud->factor_to_user;
}
