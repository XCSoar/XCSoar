/*
 * Copyright (C) 2010-2015 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WSTRING_API_HPP
#define WSTRING_API_HPP

#include "Compiler.h"

#include <string.h>
#include <tchar.h>
#include <assert.h>

gcc_pure
static inline size_t
StringLength(const TCHAR *p)
{
  return _tcslen(p);
}

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

static inline void
UnsafeCopyString(TCHAR *dest, const TCHAR *src)
{
  _tcscpy(dest, src);
}

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

/**
 * Checks whether #a and #b are equal.
 */
static inline bool
StringIsEqual(const TCHAR *a, const TCHAR *b, size_t length)
{
  assert(a != nullptr);
  assert(a != nullptr);

  return _tcsncmp(a, b, length) == 0;
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

gcc_malloc
static inline TCHAR *
DuplicateString(const TCHAR *p)
{
  return _tcsdup(p);
}

#endif
