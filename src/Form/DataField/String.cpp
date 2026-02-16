// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "String.hpp"

void
DataFieldString::ModifyValue(const char *new_value) noexcept
{
  if (new_value == mValue)
    return;

  SetValue(new_value);
  Modified();
}

void
DataFieldString::SetValue(const char *Value) noexcept
{
  mValue = Value;
}

const char *
DataFieldString::GetAsString() const noexcept
{
  return GetValue();
}
