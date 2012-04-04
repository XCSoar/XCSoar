/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_STRING_UTIL_HPP
#define XCSOAR_STRING_UTIL_HPP

#include "Compiler.h"

#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static inline bool
StringIsEmpty(const TCHAR *string)
{
  return *string == 0;
}

#ifdef _UNICODE
static inline bool
StringIsEmpty(const char *string)
{
  return *string == 0;
}
#endif

gcc_pure
static inline size_t
StringLength(const char *p)
{
  return strlen(p);
}

#ifdef _UNICODE
gcc_pure
static inline size_t
StringLength(const TCHAR *p)
{
  return _tcslen(p);
}
#endif

gcc_pure
static inline bool
StringStartsWith(const char *haystack, const char *needle)
{
  return memcmp(haystack, needle, StringLength(needle) * sizeof(needle[0])) == 0;
}

#ifdef _UNICODE
gcc_pure
static inline bool
StringStartsWith(const TCHAR *haystack, const TCHAR *needle)
{
  return memcmp(haystack, needle, StringLength(needle) * sizeof(needle[0])) == 0;
}
#endif

gcc_pure
static inline const char *
StringFind(const char *haystack, const char *needle)
{
  return strstr(haystack, needle);
}

#ifdef _UNICODE
gcc_pure
static inline const TCHAR *
StringFind(const TCHAR *haystack, const TCHAR *needle)
{
  return _tcsstr(haystack, needle);
}
#endif

static inline char *
StringToken(char *str, const char *delim)
{
  return strtok(str, delim);
}

#ifdef _UNICODE
static inline TCHAR *
StringToken(TCHAR *str, const TCHAR *delim)
{
  return _tcstok(str, delim);
}
#endif

template<typename... Args>
static inline void
StringFormat(char *buffer, size_t size, const char *fmt, Args&&... args)
{
  snprintf(buffer, size, fmt, args...);
}

template<typename... Args>
static inline void
StringFormatUnsafe(char *buffer, const char *fmt, Args&&... args)
{
  sprintf(buffer, fmt, args...);
}

#ifdef _UNICODE
template<typename... Args>
static inline void
StringFormat(TCHAR *buffer, size_t
 size, const TCHAR *fmt, Args&&... args)
{
  _sntprintf(buffer, size, fmt, args...);
}

template<typename... Args>
static inline void
StringFormatUnsafe(TCHAR *buffer, const TCHAR *fmt, Args&&... args)
{
  _stprintf(buffer, fmt, args...);
}
#endif

/**
 * Returns the portion of the string after a prefix.  If the string
 * does not begin with the specified prefix, this function returns
 * NULL.
 */
const TCHAR *
StringAfterPrefix(const TCHAR *string, const TCHAR *prefix);

/**
 * Returns the portion of the string after a prefix.  If the string
 * does not begin with the specified prefix, this function returns
 * NULL.
 * This function is case-independent.
 */
const TCHAR *
StringAfterPrefixCI(const TCHAR *string, const TCHAR *prefix);

/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param size the size of the destination buffer (including the null
 * terminator)
 * @return a pointer to the null terminator
 */
TCHAR *
CopyString(TCHAR *dest, const TCHAR *src, size_t size);

#ifdef _UNICODE
/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param size the size of the destination buffer (including the null
 * terminator)
 * @return a pointer to the null terminator
 */
char *
CopyString(char *dest, const char *src, size_t size);
#endif

/**
 * Skips whitespace at the beginning of the string, and returns the
 * first non-whitespace character.  If the string has no
 * non-whitespace characters, then a pointer to the NULL terminator is
 * returned.
 */
gcc_pure
const TCHAR *
TrimLeft(const TCHAR *p);

#ifdef _UNICODE
gcc_pure
const char *
TrimLeft(const char *p);
#endif

/**
 * Strips trailing whitespace.
 */
void TrimRight(TCHAR *p);

#ifdef _UNICODE
void
TrimRight(char *p);
#endif

/**
 * Normalize a string for searching.  This strips all characters
 * except letters and digits, folds case to a neutral form.  It is
 * possible to do this in-place (src==dest).
 *
 * @param dest the destination buffer; there must be enough room for
 * the source string and the trailing zero
 * @param src the source string
 * @return the destination buffer
 */
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
  assert(str1 != NULL);
  assert(str2 != NULL);

  return _tcscmp(str1, str2) == 0;
}

#ifdef _UNICODE
/**
 * Checks whether str1 and str2 are equal.
 * @param str1 String 1
 * @param str2 String 2
 * @return True if equal, False otherwise
 */
static inline bool
StringIsEqual(const char *str1, const char *str2)
{
  assert(str1 != NULL);
  assert(str2 != NULL);

  return strcmp(str1, str2) == 0;
}
#endif

/**
 * Copy the string to a new allocation.  The return value must be
 * freed with free().
 */
gcc_malloc
static inline char *
DuplicateString(const char *p)
{
  return strdup(p);
}

/**
 * Copy a portion of the string to a new allocation.  The given length
 * must not be smaller than the actual length of the null-terminated
 * string.  The return value will be null-terminated and must be freed
 * with free().
 */
gcc_malloc
char *
DuplicateString(const char *p, size_t length);

#ifdef _UNICODE

gcc_malloc
static inline TCHAR *
DuplicateString(const TCHAR *p)
{
  return _tcsdup(p);
}

gcc_malloc
TCHAR *
DuplicateString(const TCHAR *p, size_t length);

#endif

#endif
