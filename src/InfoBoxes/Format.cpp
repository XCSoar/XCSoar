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
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/GlideRatioFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Math/Angle.hpp"

void
InfoBoxData::SetValue(const TCHAR *format, double value)
{
  UnsafeFormatValue(format, (double)value);
}

void
InfoBoxData::SetValue(Angle _value, const TCHAR *suffix)
{
  assert(suffix != NULL);

  FormatBearing(value.buffer(), value.capacity(), _value, suffix);
}

void
InfoBoxData::SetValueFromBearingDifference(Angle delta)
{
  FormatAngleDelta(value.buffer(), value.capacity(), delta);
}

void
InfoBoxData::SetValueFromGlideRatio(double gr)
{
  FormatGlideRatio(value.buffer(), value.capacity(), gr);
  SetValueUnit(Unit::GRADIENT);
}

void
InfoBoxData::SetComment(Angle _value, const TCHAR *suffix)
{
  assert(suffix != NULL);

  FormatBearing(comment.buffer(), comment.capacity(), _value, suffix);
}

void
InfoBoxData::SetCommentFromBearingDifference(Angle delta)
{
  FormatAngleDelta(comment.buffer(), comment.capacity(), delta);
}

void
InfoBoxData::SetValueFromTimeTwoLines(int dd)
{
  FormatTimeTwoLines(value.buffer(), comment.buffer(), dd);
}

void
InfoBoxData::SetValueFromPercent(double dd)
{
  UnsafeFormatValue(_T("%d"), (int)(dd));
  SetValueUnit(Unit::PERCENT);
}

void
InfoBoxData::SetCommentFromPercent(double dd)
{
  UnsafeFormatComment(_T("%d %%"), (int)(dd));
}

void
InfoBoxData::SetValueFromVoltage(double dd)
{
  UnsafeFormatValue(_T("%2.1f"), dd);
  SetValueUnit(Unit::VOLT);
}
