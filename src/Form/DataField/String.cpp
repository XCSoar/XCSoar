// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "String.hpp"

void
DataFieldString::ModifyValue(const TCHAR *new_value) noexcept
{
  if (new_value == mValue)
    return;

  SetValue(new_value);
  Modified();
}

void
DataFieldString::SetValue(const TCHAR *Value) noexcept
{
  mValue = Value;
}

const TCHAR *
DataFieldString::GetAsString() const noexcept
{
  return GetValue();
}
