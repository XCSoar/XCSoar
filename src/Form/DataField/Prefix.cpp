// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Prefix.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"

const char *
PrefixDataField::GetAsDisplayString() const noexcept
{
  const char *s = DataFieldString::GetAsDisplayString();
  if (StringIsEmpty(s))
    s = _T("*");
  return s;
}

void
PrefixDataField::Inc() noexcept
{
  const char *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const char current = GetAsString()[0];
  const char *p = current != _T('\0')
    ? StringFind(chars, current)
    : nullptr;

  char next;
  if (p == nullptr)
    next = chars[0];
  else
    next = p[1];

  const char new_value[2] = { next, _T('\0') };
  ModifyValue(new_value);
}

void
PrefixDataField::Dec() noexcept
{
  const char *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const char current = GetAsString()[0];

  char next;
  if (current == _T('\0'))
    next = chars[strlen(chars) - 1];
  else {
    const char *p = current != _T('\0')
      ? StringFind(chars, current)
      : nullptr;

    if (p > chars)
      next = p[-1];
    else
      next = _T('\0');
  }

  const char new_value[2] = { next, _T('\0') };
  ModifyValue(new_value);
}
