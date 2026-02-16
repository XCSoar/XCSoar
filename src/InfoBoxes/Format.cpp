// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/GlideRatioFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Math/Angle.hpp"

void
InfoBoxData::SetValue(Angle _value, const char *suffix) noexcept
{
  assert(suffix != NULL);

  FormatBearing(value.buffer(), value.capacity(), _value, suffix);
}

void
InfoBoxData::SetValueFromBearingDifference(Angle delta) noexcept
{
  FormatAngleDelta(value.buffer(), value.capacity(), delta);
}

void
InfoBoxData::SetValueFromGlideRatio(double gr) noexcept
{
  FormatGlideRatio(value.buffer(), value.capacity(), gr);
  SetValueUnit(Unit::GRADIENT);
}

void
InfoBoxData::SetComment(Angle _value, const char *suffix) noexcept
{
  assert(suffix != NULL);

  FormatBearing(comment.buffer(), comment.capacity(), _value, suffix);
}

void
InfoBoxData::SetCommentFromBearingDifference(Angle delta) noexcept
{
  FormatAngleDelta(comment.buffer(), comment.capacity(), delta);
}

void
InfoBoxData::SetValueFromTimeTwoLines(std::chrono::seconds dd) noexcept
{
  FormatTimeTwoLines(value.buffer(), comment.buffer(), dd);
}

void
InfoBoxData::SetValueFromPercent(double dd) noexcept
{
  FmtValue(_T("{}"), (int)dd);
  SetValueUnit(Unit::PERCENT);
}

void
InfoBoxData::SetCommentFromPercent(double dd) noexcept
{
  FmtComment(_T("{} %"), (int)(dd));
}

void
InfoBoxData::SetValueFromVoltage(double dd) noexcept
{
  FmtValue(_T("{:2.1f}"), dd);
  SetValueUnit(Unit::VOLT);
}
