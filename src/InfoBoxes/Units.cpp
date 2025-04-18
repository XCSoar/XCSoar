// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"

void
InfoBoxData::SetValueFromDistance(double new_value) noexcept
{
  Unit distance_unit =
    FormatUserDistanceSmart(new_value, value.buffer(), false);
  SetValueUnit(distance_unit);
}

void
InfoBoxData::SetValueFromAltitude(double new_value) noexcept
{
  FormatUserAltitude(new_value, value.buffer(), false);
  SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxData::SetValueFromArrival(double new_value) noexcept
{
  FormatRelativeUserAltitude(new_value, value.buffer(), false);
  SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxData::SetValueFromSpeed(double new_value, bool precision) noexcept
{
  FormatUserSpeed(new_value, value.buffer(), false, precision);
  SetValueUnit(Units::current.speed_unit);
}

void
InfoBoxData::SetValueFromTaskSpeed(double new_value, bool precision) noexcept
{
  FormatUserTaskSpeed(new_value, value.buffer(), false, precision);
  SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxData::SetCommentFromDistance(double new_value) noexcept
{
  FormatUserDistanceSmart(new_value, comment.buffer());
}

void
InfoBoxData::SetCommentFromAlternateAltitude(double new_value) noexcept
{
  FormatAlternateUserAltitude(new_value, comment.buffer());
}

void
InfoBoxData::SetCommentFromSpeed(double new_value, bool precision) noexcept
{
  FormatUserSpeed(new_value, comment.buffer(), true, precision);
}

void
InfoBoxData::SetCommentFromTaskSpeed(double new_value, bool precision) noexcept
{
  FormatUserTaskSpeed(new_value, comment.buffer(), true, precision);
}

void
InfoBoxData::SetCommentFromVerticalSpeed(double new_value,
                                         bool include_sign) noexcept
{
  FormatUserVerticalSpeed(new_value, comment.buffer(), true, include_sign);
}
