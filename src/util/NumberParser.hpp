// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <stdlib.h>
#include <cassert>

static inline double
ParseDouble(const char *p, char **endptr=nullptr)
{
  assert(p != nullptr);

  return (double)strtod(p, endptr);
}

static inline unsigned
ParseUnsigned(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return (unsigned)strtoul(p, endptr, base);
}

static inline int
ParseInt(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return (int)strtol(p, endptr, base);
}

static inline uint64_t
ParseUint64(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return strtoull(p, endptr, base);
}

static inline int64_t
ParseInt64(const char *p, char **endptr=nullptr, int base=10)
{
  assert(p != nullptr);

  return strtoll(p, endptr, base);
}
