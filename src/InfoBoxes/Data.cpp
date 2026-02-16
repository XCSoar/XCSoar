// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"

void
InfoBoxData::Clear() noexcept
{
  title.clear();
  SetInvalid();
}

void
InfoBoxData::SetInvalid() noexcept
{
  custom = 0;
  SetAllColors(0);
  SetValueInvalid();
  SetValueUnit(Unit::UNDEFINED);
  SetCommentInvalid();
}

void
InfoBoxData::SetValueInvalid() noexcept
{
  SetValueColor(0);
  SetValue("---");
  SetValueUnit(Unit::UNDEFINED);
}

void
InfoBoxData::SetTitle(const char *_title) noexcept
{
  title = _title;
  title.CropIncompleteUTF8();
}

void
InfoBoxData::SetValue(const char *_value) noexcept
{
  value = _value;
}

void
InfoBoxData::SetComment(const char *_comment) noexcept
{
  comment = _comment;
  comment.CropIncompleteUTF8();
}

void
InfoBoxData::SetAllColors(unsigned color) noexcept
{
  SetTitleColor(color);
  SetValueColor(color);
  SetCommentColor(color);
}

bool
InfoBoxData::CompareTitle(const InfoBoxData &other) const noexcept
{
  return title == other.title &&
    title_color == other.title_color;
}

bool
InfoBoxData::CompareValue(const InfoBoxData &other) const noexcept
{
  return value == other.value &&
    value_unit == other.value_unit &&
    value_color == other.value_color;
}

bool
InfoBoxData::CompareComment(const InfoBoxData &other) const noexcept
{
  return comment == other.comment &&
    comment_color == other.comment_color;
}
