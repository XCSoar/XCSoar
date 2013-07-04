/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "CharUtil.hpp"

#include <algorithm>

#include <string.h>
#include <stdlib.h>

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

#ifdef _UNICODE

bool
StringEndsWith(const TCHAR *haystack, const TCHAR *needle)
{
  const size_t haystack_length = StringLength(haystack);
  const size_t needle_length = StringLength(needle);

  return haystack_length >= needle_length &&
    StringIsEqual(haystack + haystack_length - needle_length, needle);
}

bool
StringEndsWithIgnoreCase(const TCHAR *haystack, const TCHAR *needle)
{
  const size_t haystack_length = StringLength(haystack);
  const size_t needle_length = StringLength(needle);

  return haystack_length >= needle_length &&
    StringIsEqualIgnoreCase(haystack + haystack_length - needle_length,
                            needle);
}

#endif

const char *
StringAfterPrefix(const char *string, const char *prefix)
{
  assert(string != nullptr);
  assert(prefix != nullptr);

  size_t prefix_length = strlen(prefix);
  return strncmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : nullptr;
}

const char *
StringAfterPrefixCI(const char *string, const char *prefix)
{
  assert(string != nullptr);
  assert(prefix != nullptr);

  size_t prefix_length = StringLength(prefix);
  return strncasecmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : nullptr;
}

#ifdef _UNICODE

const TCHAR *
StringAfterPrefix(const TCHAR *string, const TCHAR *prefix)
{
  assert(string != nullptr);
  assert(prefix != nullptr);

  size_t prefix_length = _tcslen(prefix);
  return _tcsncmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : nullptr;
}

const TCHAR *
StringAfterPrefixCI(const TCHAR *string, const TCHAR *prefix)
{
  assert(string != nullptr);
  assert(prefix != nullptr);

  size_t prefix_length = _tcslen(prefix);
  return _tcsnicmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : nullptr;
}

#endif

char *
CopyString(char *gcc_restrict dest, const char *gcc_restrict src, size_t size)
{
  size_t length = strlen(src);
  if (length >= size)
    length = size - 1;

  char *p = std::copy(src, src + length, dest);
  *p = '\0';
  return p;
}

#ifdef _UNICODE

TCHAR *
CopyString(TCHAR *gcc_restrict dest, const TCHAR *gcc_restrict src,
           size_t size)
{
  size_t length = _tcslen(src);
  if (length >= size)
    length = size - 1;

  TCHAR *p = std::copy(src, src + length, dest);
  *p = _T('\0');
  return p;
}

#endif

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
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);
  assert(src_end != nullptr);
  assert(src_end >= src);

  for (const char *const dest_end = dest + dest_size;
       dest != dest_end && src != src_end; ++src)
    if (IsASCII(*src))
      *dest++ = *src;

  return dest;
}

#ifdef _UNICODE
void
CopyASCII(TCHAR *dest, const TCHAR *src)
{
  do {
    if (IsASCII(*src))
      *dest++ = *src;
  } while (*src++ != _T('\0'));
}

TCHAR *
CopyASCII(TCHAR *dest, size_t dest_size,
          const TCHAR *src, const TCHAR *src_end)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);
  assert(src_end != nullptr);
  assert(src_end >= src);

  const TCHAR *const dest_end = dest + dest_size;
  for (; dest != dest_end && src != src_end; ++src)
    if (IsASCII(*src))
      *dest++ = *src;

  return dest;
}

void
CopyASCII(TCHAR *dest, const char *src)
{
  do {
    if (IsASCII(*src))
      *dest++ = (TCHAR)*src;
  } while (*src++ != '\0');
}

template<typename D, typename S>
static D *
TemplateCopyASCII(D *dest, size_t dest_size, const S *src, const S *src_end)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);
  assert(src_end != nullptr);
  assert(src_end >= src);

  const D *const dest_end = dest + dest_size;
  for (; dest != dest_end && src != src_end; ++src)
    if (IsASCII(*src))
      *dest++ = *src;

  return dest;
}

TCHAR *
CopyASCII(TCHAR *dest, size_t dest_size, const char *src, const char *src_end)
{
  return TemplateCopyASCII(dest, dest_size, src, src_end);
}

char *
CopyASCII(char *dest, size_t dest_size, const TCHAR *src, const TCHAR *src_end)
{
  return TemplateCopyASCII(dest, dest_size, src, src_end);
}

#endif

void
CopyASCIIUppper(char *dest, const char *src)
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

#ifdef _UNICODE

void
CopyASCIIUppper(char *dest, const TCHAR *src)
{
  do {
    TCHAR t = *src;
    if (IsASCII(t)) {
      char ch = (char)t;
      if (IsLowerAlphaASCII(ch))
        ch -= 'a' - 'A';

      *dest++ = ch;
    }
  } while (*src++ != '\0');
}

#endif

const char *
TrimLeft(const char *p)
{
  while (IsWhitespaceNotNull(*p))
    ++p;
  return p;
}

#ifdef _UNICODE
const TCHAR *
TrimLeft(const TCHAR *p)
{
  while (IsWhitespaceNotNull(*p))
    ++p;
  return p;
}
#endif

void
TrimRight(char *p)
{
  size_t length = strlen(p);

  while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
    --length;

  p[length] = 0;
}

#ifdef _UNICODE
void
TrimRight(TCHAR *p)
{
  size_t length = _tcslen(p);

  while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
    --length;

  p[length] = 0;
}
#endif

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

#ifdef _UNICODE
TCHAR *
NormalizeSearchString(TCHAR *gcc_restrict dest,
                      const TCHAR *gcc_restrict src)
{
  TCHAR *retval = dest;

  for (; !StringIsEmpty(src); ++src)
    if (IsAlphaNumericASCII(*src))
      *dest++ = ToUpperASCII(*src);

  *dest = _T('\0');

  return retval;
}
#endif

char *
DuplicateString(const char *p, size_t length)
{
  char *q = (char *)malloc((length + 1) * sizeof(*p));
  if (q != nullptr)
    *std::copy(p, p + length, q) = '\0';
  return q;
}

#ifdef _UNICODE
TCHAR *
DuplicateString(const TCHAR *p, size_t length)
{
  TCHAR *q = (TCHAR *)malloc((length + 1) * sizeof(*p));
  if (q != nullptr)
    *std::copy(p, p + length, q) = _T('\0');
  return q;
}
#endif
