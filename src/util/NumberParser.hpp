// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <stdlib.h>
#include <cassert>

#ifdef _UNICODE
#include <wchar.h>
#endif

static inline double
ParseDouble(const char *p, char **endptr=nullptr)
{
  assert(p != nullptr);

  return (double)strtod(p, endptr);
}

#ifdef _UNICODE
static inline double
ParseDouble(const wchar_t *p, wchar_t **endptr)
{
  assert(p != nullptr);

  return (double)wcstod(p, endptr);
}

#ifdef _WIN32
#include <windef.h>
#ifdef __MINGW64_VERSION_MAJOR
#if __MINGW64_VERSION_MAJOR == 3 && __MINGW64_VERSION_MINOR == 1
#define BUGGY_WCSTOD
#endif
#endif
#endif

static inline double
ParseDouble(const wchar_t *p)
{
  assert(p != nullptr);

#ifdef BUGGY_WCSTOD
  /* workaround for mingw64 3.1 bug to avoid nullptr dereference */
  wchar_t *dummy;
  return ParseDouble(p, &dummy);
#else
  return ParseDouble(p, nullptr);
#endif
}

#endif

static inline unsigned
ParseUnsigned(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return (unsigned)strtoul(p, endptr, base);
}

#ifdef _UNICODE
static inline unsigned
ParseUnsigned(const wchar_t *p, wchar_t **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return (unsigned)wcstoul(p, endptr, base);
}
#endif

static inline int
ParseInt(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return (int)strtol(p, endptr, base);
}

#ifdef _UNICODE
static inline int
ParseInt(const wchar_t *p, wchar_t **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return (int)wcstol(p, endptr, base);
}
#endif

static inline uint64_t
ParseUint64(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return strtoull(p, endptr, base);
}

#ifdef _UNICODE
static inline uint64_t
ParseUint64(const wchar_t *p, wchar_t **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return wcstoull(p, endptr, base);
}
#endif

static inline int64_t
ParseInt64(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return strtoll(p, endptr, base);
}

#ifdef _UNICODE
static inline int64_t
ParseInt64(const wchar_t *p, wchar_t **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return wcstoll(p, endptr, base);
}
#endif
