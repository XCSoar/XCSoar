/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Data.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"

void
InfoBoxData::SetValueFromDistance(double new_value)
{
  Unit distance_unit =
    FormatUserDistanceSmart(new_value, value.buffer(), false);
  SetValueUnit(distance_unit);
}

void
InfoBoxData::SetValueFromAltitude(double new_value)
{
  FormatUserAltitude(new_value, value.buffer(), false);
  SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxData::SetValueFromArrival(double new_value)
{
  FormatRelativeUserAltitude(new_value, value.buffer(), false);
  SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxData::SetValueFromSpeed(double new_value, bool precision)
{
  FormatUserSpeed(new_value, value.buffer(), false, precision);
  SetValueUnit(Units::current.speed_unit);
}

void
InfoBoxData::SetValueFromTaskSpeed(double new_value, bool precision)
{
  FormatUserTaskSpeed(new_value, value.buffer(), false, precision);
  SetValueUnit(Units::current.task_speed_unit);
}

void
InfoBoxData::SetCommentFromDistance(double new_value)
{
  FormatUserDistanceSmart(new_value, comment.buffer());
}

void
InfoBoxData::SetCommentFromAlternateAltitude(double new_value)
{
  FormatAlternateUserAltitude(new_value, comment.buffer());
}

void
InfoBoxData::SetCommentFromSpeed(double new_value, bool precision)
{
  FormatUserSpeed(new_value, comment.buffer(), true, precision);
}

void
InfoBoxData::SetCommentFromTaskSpeed(double new_value, bool precision)
{
  FormatUserTaskSpeed(new_value, comment.buffer(), true, precision);
}

void
InfoBoxData::SetCommentFromVerticalSpeed(double new_value, bool include_sign)
{
  FormatUserVerticalSpeed(new_value, comment.buffer(), true, include_sign);
}
