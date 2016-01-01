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

#ifndef XCSOAR_NUMBER_PARSER_HPP
#define XCSOAR_NUMBER_PARSER_HPP

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _UNICODE
#include <tchar.h>
#endif

static inline double
ParseDouble(const char *p, char **endptr=nullptr)
{
  assert(p != nullptr);

  return (double)strtod(p, endptr);
}

#ifdef _UNICODE
static inline double
ParseDouble(const TCHAR *p, TCHAR **endptr)
{
  assert(p != nullptr);

  return (double)wcstod(p, endptr);
}

#ifdef WIN32
#include <windef.h>
#ifdef __MINGW64_VERSION_MAJOR
#if __MINGW64_VERSION_MAJOR == 3 && __MINGW64_VERSION_MINOR == 1
#define BUGGY_WCSTOD
#endif
#endif
#endif

static inline double
ParseDouble(const TCHAR *p)
{
  assert(p != nullptr);

#ifdef BUGGY_WCSTOD
  /* workaround for mingw64 3.1 bug to avoid nullptr dereference */
  TCHAR *dummy;
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
ParseUnsigned(const TCHAR *p, TCHAR **endptr=nullptr, int base=10)
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
ParseInt(const TCHAR *p, TCHAR **endptr=nullptr, int base=10)
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
ParseUint64(const TCHAR *p, TCHAR **endptr=nullptr, int base=10)
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
ParseInt64(const TCHAR *p, TCHAR **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return wcstoll(p, endptr, base);
}
#endif

#endif
