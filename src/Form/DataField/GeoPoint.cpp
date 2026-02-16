// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoPoint.hpp"
#include "Formatter/GeoPointFormatter.hpp"

void
GeoPointDataField::ModifyValue(GeoPoint _value) noexcept
{
  if (_value == value)
    return;

  value = _value;
  Modified();
}

const char *
GeoPointDataField::GetAsString() const noexcept
{
  if (!value.IsValid())
    return _T("");

  return FormatGeoPoint(value, string_buffer, std::size(string_buffer),
                        format);
}
