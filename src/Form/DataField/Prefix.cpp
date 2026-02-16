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
    s = "*";
  return s;
}

void
PrefixDataField::Inc() noexcept
{
  const char *chars = GetAllowedCharacters();
  if (StringIsEmpty(chars))
    return;

  const char current = GetAsString()[0];
  const char *p = current != '\0'
    ? StringFind(chars, current)
    : nullptr;

  char next;
  if (p == nullptr)
    next = chars[0];
  else
    next = p[1];

  const char new_value[2] = { next, '\0' };
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
  if (current == '\0')
    next = chars[strlen(chars) - 1];
  else {
    const char *p = current != '\0'
      ? StringFind(chars, current)
      : nullptr;

    if (p > chars)
      next = p[-1];
    else
      next = '\0';
  }

  const char new_value[2] = { next, '\0' };
  ModifyValue(new_value);
}
