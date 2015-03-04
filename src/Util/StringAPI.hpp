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

#ifndef STRING_API_HPP
#define STRING_API_HPP

#include "Compiler.h"

#include <string.h>
#include <assert.h>

#ifdef _UNICODE
#include "WStringAPI.hpp"
#endif

gcc_pure
static inline size_t
StringLength(const char *p)
{
  return strlen(p);
}

gcc_pure
static inline const char *
StringFind(const char *haystack, const char *needle)
{
  return strstr(haystack, needle);
}

static inline char *
StringToken(char *str, const char *delim)
{
  return strtok(str, delim);
}

static inline void
UnsafeCopyString(char *dest, const char *src)
{
  strcpy(dest, src);
}

/**
 * Checks whether #a and #b are equal.
 */
static inline bool
StringIsEqual(const char *a, const char *b)
{
  assert(a != nullptr);
  assert(a != nullptr);

  return strcmp(a, b) == 0;
}

/**
 * Checks whether #a and #b are equal.
 */
static inline bool
StringIsEqual(const char *a, const char *b, size_t length)
{
  assert(a != nullptr);
  assert(a != nullptr);

  return strncmp(a, b, length) == 0;
}

static inline bool
StringIsEqualIgnoreCase(const char *a, const char *b)
{
  assert(a != nullptr);
  assert(b != nullptr);

  return strcasecmp(a, b) == 0;
}

static inline bool
StringIsEqualIgnoreCase(const char *a, const char *b, size_t size)
{
  assert(a != nullptr);
  assert(b != nullptr);

  return strncasecmp(a, b, size) == 0;
}

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

#endif
