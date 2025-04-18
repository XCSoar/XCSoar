// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Prefix.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"

const TCHAR *
PrefixDataField::GetAsDisplayString() const noexcept
{
  const TCHAR *s = DataFieldString::GetAsDisplayString();
  if (StringIsEmpty(s))
    s = _T("*");
  return s;
}

void
PrefixDataField::Inc() noexcept
{
  const TCHAR *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const TCHAR current = GetAsString()[0];
  const TCHAR *p = current != _T('\0')
    ? StringFind(chars, current)
    : nullptr;

  TCHAR next;
  if (p == nullptr)
    next = chars[0];
  else
    next = p[1];

  const TCHAR new_value[2] = { next, _T('\0') };
  ModifyValue(new_value);
}

void
PrefixDataField::Dec() noexcept
{
  const TCHAR *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const TCHAR current = GetAsString()[0];

  TCHAR next;
  if (current == _T('\0'))
    next = chars[_tcslen(chars) - 1];
  else {
    const TCHAR *p = current != _T('\0')
      ? StringFind(chars, current)
      : nullptr;

    if (p > chars)
      next = p[-1];
    else
      next = _T('\0');
  }

  const TCHAR new_value[2] = { next, _T('\0') };
  ModifyValue(new_value);
}
