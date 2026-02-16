// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EscapeBackslash.hpp"

#include <string.h>
#include <tchar.h>

tstring_view::pointer
UnescapeBackslash(tstring_view old_string) noexcept
{
  char buffer[2048]; // Note - max size of any string we cope with here !

  tstring_view::size_type used = 0;

  for (tstring_view::size_type i = 0; i < old_string.size(); i++) {
    if (used < 2045) {
      if (old_string[i] == '\\') {
        if (old_string[i + 1] == 'r') {
          buffer[used++] = '\r';
          i++;
        } else if (old_string[i + 1] == 'n') {
          buffer[used++] = '\n';
          i++;
        } else if (old_string[i + 1] == '\\') {
          buffer[used++] = '\\';
          i++;
        } else {
          buffer[used++] = old_string[i];
        }
      } else {
        buffer[used++] = old_string[i];
      }
    }
  }

  buffer[used++] = _T('\0');

  return strdup(buffer);
}
