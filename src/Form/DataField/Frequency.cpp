// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Frequency.hpp"
#include "RadioFrequency.hpp"
#include "tchar.h"
#include <cstdio>

void
RadioFrequencyDataField::ModifyValue(RadioFrequency _value) noexcept
{
  if (_value == value)
    return;

  value = _value;
  Modified();
}

const TCHAR *
RadioFrequencyDataField::GetAsString() const noexcept
{
  if (!value.IsDefined())
    return _T("");

  unsigned int mhz = value.GetKiloHertz()/1000;
  unsigned int khz = value.GetKiloHertz()%1000;
  assert(mhz < 1000);
  _stprintf(string_buffer, string_buffer ,_T("%d.%03d"), mhz, khz);
  return string_buffer;
}
