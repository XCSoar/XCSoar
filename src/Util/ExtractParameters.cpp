/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ExtractParameters.hpp"

#include <assert.h>

static bool is_closing_quote_char(TCHAR const *s) {
  // Perform look-ahead to check if the detected quote_char terminates
  // the quoted field. true for (optional) ' 's followed by ',' or
  // end of input string.
  while (*s != _T('\0')) {
    if (*s == _T(','))
      return true;
    if (*s != _T(' '))
      return false;
    s++;
  }
  return true;
}

size_t
ExtractParameters(const TCHAR *src, TCHAR *dst,
                  const TCHAR **arr, size_t sz,
                  const bool trim, const TCHAR quote_char)
{
  assert(src != nullptr);
  assert(dst != nullptr);
  assert(arr != nullptr);
  assert(sz > 0);

  TCHAR *d = dst;
  TCHAR const *s = src;
  bool in_quote = false;

  size_t i = 0;
  *d = _T('\0');
  arr[i] = d;

  // pointer to last character of d after which ' ' might be discarded for
  // trimming. nullptr can be used to test for start of field condition.
  TCHAR *last_non_discardable_char = nullptr;

  do {
    if (quote_char && *s == quote_char) {
      // Handle quote characters if enabled, current char of src is quote_char
      if (in_quote && is_closing_quote_char(s+1)) {
        // If currently in quoted string leave quote handling depending on
        // look-ahead performed in is_closing_quote_char()
        in_quote = false;
      } else if (!in_quote && last_non_discardable_char == nullptr) {
        // Quote start detected at beginning of new field
        in_quote = true;
      } else {
        // Quote inside quoted string (but not end), copy to dst
        last_non_discardable_char = d;
        *d++ = *s;
        if (*(s+1) == quote_char) {
          // Spreadsheets use "" while exporting a single " contained in
          // a textfield to a CSV file - skip second "
          s++;
        }
      }
    } else if (*s == _T('\0') || ((*s == _T(',') && !in_quote))) {
      // end of src or field separator (',') outside quoted part detected
      // field content is already completely copied but may contain trailing spaces
      if (trim && (last_non_discardable_char != nullptr))
        // remove trailing unquoted whitespace from dst field
        d = last_non_discardable_char+1;

      // 0-terminate field and update pointer to individual field
      *d++ = _T('\0');
      if (++i < sz)
        // if size permits remember start of next field.
        arr[i] = d;

      // start processing of next field
      last_non_discardable_char = nullptr;
    } else {
      // main part copying from src to dst
      if (trim && !in_quote) {
        // we are not inside a quoted string and therefore need to trim
        // leading and trailing spaces. Drop whitespace if dst is still empty
        if (*s == _T(' ')) {
          if (last_non_discardable_char != nullptr)
            // dst field is not empty - append ' '
            *d++ = *s;
        } else {
          // no space, remember position and append to result
          last_non_discardable_char = d;
          *d++ = *s;
        }
      } else {
        // normal copy src to dst, all chars are retained
        // all characters are not subject to trimming (even spaces)
        last_non_discardable_char = d;
        *d++ = *s;
      }
    }
    // advance src until end reached or no space in field pointers
  } while (*s++ != _T('\0') && i < sz);

  return i;
}
