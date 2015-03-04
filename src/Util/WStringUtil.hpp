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

#ifndef XCSOAR_WSTRING_UTIL_HPP
#define XCSOAR_WSTRING_UTIL_HPP

#ifndef _UNICODE
#error Cannot use this header without _UNICODE
#endif

#include "Compiler.h"

#include <assert.h>
#include <tchar.h>
#include <string.h>

static inline bool
StringIsEmpty(const TCHAR *string)
{
  return *string == 0;
}

gcc_pure
static inline size_t
StringLength(const TCHAR *p)
{
  return _tcslen(p);
}

gcc_pure
static inline bool
StringStartsWith(const TCHAR *haystack, const TCHAR *needle)
{
  return memcmp(haystack, needle, StringLength(needle) * sizeof(needle[0])) == 0;
}

gcc_pure
bool
StringEndsWith(const TCHAR *haystack, const TCHAR *needle);

gcc_pure
bool
StringEndsWithIgnoreCase(const TCHAR *haystack, const TCHAR *needle);

gcc_pure
static inline const TCHAR *
StringFind(const TCHAR *haystack, const TCHAR *needle)
{
  return _tcsstr(haystack, needle);
}

static inline TCHAR *
StringToken(TCHAR *str, const TCHAR *delim)
{
  return _tcstok(str, delim);
}

/**
 * Returns the portion of the string after a prefix.  If the string
 * does not begin with the specified prefix, this function returns
 * nullptr.
 */
gcc_nonnull_all
const TCHAR *
StringAfterPrefix(const TCHAR *string, const TCHAR *prefix);

/**
 * Returns the portion of the string after a prefix.  If the string
 * does not begin with the specified prefix, this function returns
 * nullptr.
 * This function is case-independent.
 */
gcc_nonnull_all
const TCHAR *
StringAfterPrefixCI(const TCHAR *string, const TCHAR *prefix);

static inline void
UnsafeCopyString(TCHAR *dest, const TCHAR *src)
{
  _tcscpy(dest, src);
}

/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param size the size of the destination buffer (including the null
 * terminator)
 * @return a pointer to the null terminator
 */
gcc_nonnull_all
TCHAR *
CopyString(TCHAR *dest, const TCHAR *src, size_t size);

gcc_nonnull_all
void
CopyASCII(TCHAR *dest, const TCHAR *src);

gcc_nonnull_all
TCHAR *
CopyASCII(TCHAR *dest, size_t dest_size,
          const TCHAR *src, const TCHAR *src_end);

gcc_nonnull_all
void
CopyASCII(TCHAR *dest, const char *src);

gcc_nonnull_all
TCHAR *
CopyASCII(TCHAR *dest, size_t dest_size, const char *src, const char *src_end);

gcc_nonnull_all
char *
CopyASCII(char *dest, size_t dest_size, const TCHAR *src, const TCHAR *src_end);

gcc_nonnull_all
void
CopyASCIIUpper(char *dest, const TCHAR *src);

gcc_pure gcc_nonnull_all
const TCHAR *
StripLeft(const TCHAR *p);

gcc_pure
const TCHAR *
StripRight(const TCHAR *p, const TCHAR *end);

/**
 * Determine the string's end as if it was stripped on the right side.
 */
gcc_pure
static inline TCHAR *
StripRight(TCHAR *p, TCHAR *end)
{
  return const_cast<TCHAR *>(StripRight((const TCHAR *)p,
                                        (const TCHAR *)end));
}

/**
 * Determine the string's length as if it was stripped on the right
 * side.
 */
gcc_pure
size_t
StripRight(const TCHAR *p, size_t length);

gcc_nonnull_all
void
StripRight(TCHAR *p);

gcc_nonnull_all
TCHAR *
NormalizeSearchString(TCHAR *dest, const TCHAR *src);

/**
 * Checks whether str1 and str2 are equal.
 * @param str1 String 1
 * @param str2 String 2
 * @return True if equal, False otherwise
 */
static inline bool
StringIsEqual(const TCHAR *str1, const TCHAR *str2)
{
  assert(str1 != nullptr);
  assert(str2 != nullptr);

  return _tcscmp(str1, str2) == 0;
}

static inline bool
StringIsEqualIgnoreCase(const TCHAR *a, const TCHAR *b)
{
  assert(a != nullptr);
  assert(b != nullptr);

  return _tcsicmp(a, b) == 0;
}

static inline bool
StringIsEqualIgnoreCase(const TCHAR *a, const TCHAR *b, size_t size)
{
  assert(a != nullptr);
  assert(b != nullptr);

  return _tcsnicmp(a, b, size) == 0;
}

gcc_pure
static inline bool
StringStartsWithIgnoreCase(const TCHAR *haystack, const TCHAR *needle)
{
  return StringIsEqualIgnoreCase(haystack, needle,
                                 StringLength(needle) * sizeof(needle[0]));
}

gcc_malloc
static inline TCHAR *
DuplicateString(const TCHAR *p)
{
  return _tcsdup(p);
}

gcc_malloc gcc_nonnull_all
TCHAR *
DuplicateString(const TCHAR *p, size_t length);

#endif
