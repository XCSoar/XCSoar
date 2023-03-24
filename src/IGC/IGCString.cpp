// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGCString.hpp"

#include <cassert>

char *
CopyIGCString(char *dest, char *dest_limit, const char *src) noexcept
{
  assert(dest != nullptr);
  assert(dest_limit > dest);
  assert(src != nullptr);

  while (*src != '\0') {
    char ch = *src++;
    if (IsValidIGCChar(ch)) {
      *dest++ = ch;
      if (dest == dest_limit)
        break;
    }
  }

  return dest;
}

#ifdef _UNICODE

char *
CopyIGCString(char *dest, char *dest_limit, const TCHAR *src) noexcept
{
  assert(dest != nullptr);
  assert(dest_limit > dest);
  assert(src != nullptr);

  while (*src != '\0') {
    TCHAR ch = *src++;
    if (IsValidIGCChar(ch)) {
      *dest++ = ch;
      if (dest == dest_limit)
        break;
    }
  }

  return dest;
}

#endif
