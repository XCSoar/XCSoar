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

#include <wchar.h>
#include <assert.h>

gcc_pure
static inline size_t
StringLength(const wchar_t *p)
{
  return wcslen(p);
}

gcc_pure
static inline const wchar_t *
StringFind(const wchar_t *haystack, const wchar_t *needle)
{
  return wcsstr(haystack, needle);
}

static inline wchar_t *
StringToken(wchar_t *str, const wchar_t *delim)
{
  return wcstok(str, delim);
}

static inline void
UnsafeCopyString(wchar_t *dest, const wchar_t *src)
{
  wcscpy(dest, src);
}

/**
 * Checks whether str1 and str2 are equal.
 * @param str1 String 1
 * @param str2 String 2
 * @return True if equal, False otherwise
 */
static inline bool
StringIsEqual(const wchar_t *str1, const wchar_t *str2)
{
  assert(str1 != nullptr);
  assert(str2 != nullptr);

  return wcscmp(str1, str2) == 0;
}

/**
 * Checks whether #a and #b are equal.
 */
static inline bool
StringIsEqual(const wchar_t *a, const wchar_t *b, size_t length)
{
  assert(a != nullptr);
  assert(a != nullptr);

  return wcsncmp(a, b, length) == 0;
}

static inline bool
StringIsEqualIgnoreCase(const wchar_t *a, const wchar_t *b)
{
  assert(a != nullptr);
  assert(b != nullptr);

  return _wcsicmp(a, b) == 0;
}

static inline bool
StringIsEqualIgnoreCase(const wchar_t *a, const wchar_t *b, size_t size)
{
  assert(a != nullptr);
  assert(b != nullptr);

  return _wcsnicmp(a, b, size) == 0;
}

gcc_malloc
static inline wchar_t *
DuplicateString(const wchar_t *p)
{
  return wcsdup(p);
}

#endif
