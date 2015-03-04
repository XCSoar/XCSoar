/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "StringUtil.hpp"
#include "StringAPI.hpp"
#include "CharUtil.hpp"

#include <algorithm>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

bool
StringStartsWith(const char *haystack, const char *needle)
{
  return memcmp(haystack, needle, StringLength(needle) * sizeof(needle[0])) == 0;
}

bool
StringEndsWith(const char *haystack, const char *needle)
{
  const size_t haystack_length = StringLength(haystack);
  const size_t needle_length = StringLength(needle);

  return haystack_length >= needle_length &&
    StringIsEqual(haystack + haystack_length - needle_length, needle);
}

bool
StringEndsWithIgnoreCase(const char *haystack, const char *needle)
{
  const size_t haystack_length = StringLength(haystack);
  const size_t needle_length = StringLength(needle);

  return haystack_length >= needle_length &&
    StringIsEqualIgnoreCase(haystack + haystack_length - needle_length,
                            needle);
}

const char *
StringAfterPrefix(const char *string, const char *prefix)
{
#if !CLANG_CHECK_VERSION(3,6)
  /* disabled on clang due to -Wtautological-pointer-compare */
  assert(string != nullptr);
  assert(prefix != nullptr);
#endif

  size_t prefix_length = strlen(prefix);
  return StringIsEqual(string, prefix, prefix_length)
    ? string + prefix_length
    : nullptr;
}

const char *
StringAfterPrefixCI(const char *string, const char *prefix)
{
#if !CLANG_CHECK_VERSION(3,6)
  /* disabled on clang due to -Wtautological-pointer-compare */
  assert(string != nullptr);
  assert(prefix != nullptr);
#endif

  size_t prefix_length = StringLength(prefix);
  return strncasecmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : nullptr;
}

char *
CopyString(char *gcc_restrict dest, const char *gcc_restrict src, size_t size)
{
  size_t length = strlen(src);
  if (length >= size)
    length = size - 1;

  char *p = std::copy_n(src, length, dest);
  *p = '\0';
  return p;
}

void
CopyASCII(char *dest, const char *src)
{
  do {
    if (IsASCII(*src))
      *dest++ = *src;
  } while (*src++ != '\0');
}

char *
CopyASCII(char *dest, size_t dest_size, const char *src, const char *src_end)
{
#if !CLANG_CHECK_VERSION(3,6)
  /* disabled on clang due to -Wtautological-pointer-compare */
  assert(dest != nullptr);
  assert(src != nullptr);
  assert(src_end != nullptr);
#endif
  assert(dest_size > 0);
  assert(src_end >= src);

  for (const char *const dest_end = dest + dest_size;
       dest != dest_end && src != src_end; ++src)
    if (IsASCII(*src))
      *dest++ = *src;

  return dest;
}

void
CopyASCIIUpper(char *dest, const char *src)
{
  do {
    char ch = *src;
    if (IsASCII(ch)) {
      if (IsLowerAlphaASCII(ch))
        ch -= 'a' - 'A';

      *dest++ = ch;
    }
  } while (*src++ != '\0');
}

const char *
StripLeft(const char *p)
{
  while (IsWhitespaceNotNull(*p))
    ++p;

  return p;
}

const char *
StripRight(const char *p, const char *end)
{
  while (end > p && IsWhitespaceOrNull(end[-1]))
    --end;

  return end;
}

size_t
StripRight(const char *p, size_t length)
{
  while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
    --length;

  return length;
}

void
StripRight(char *p)
{
  size_t old_length = strlen(p);
  size_t new_length = StripRight(p, old_length);
  p[new_length] = 0;
}

char *
NormalizeSearchString(char *gcc_restrict dest,
                      const char *gcc_restrict src)
{
  char *retval = dest;

  for (; !StringIsEmpty(src); ++src)
    if (IsAlphaNumericASCII(*src))
      *dest++ = ToUpperASCII(*src);

  *dest = '\0';

  return retval;
}

bool
StringStartsWithIgnoreCase(const char *haystack, const char *needle)
{
  return StringIsEqualIgnoreCase(haystack, needle,
                                 StringLength(needle) * sizeof(needle[0]));
}

char *
DuplicateString(const char *p, size_t length)
{
  char *q = (char *)malloc((length + 1) * sizeof(*p));
  if (q != nullptr)
    *std::copy_n(p, length, q) = '\0';
  return q;
}
