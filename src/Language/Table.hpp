// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#ifdef HAVE_NLS

#include <cstddef>
#include <tchar.h>

struct BuiltinLanguage {
#ifdef _WIN32
  unsigned language;
#endif
#ifdef USE_LIBINTL
  /**
   * The (POSIX) locale name (only language and territory, without
   * codeset and modifier), e.g. "de_DE".
   */
  const char *locale;
#else
  const std::byte *begin;
  size_t size;
#endif
  const char *resource;
  const char *name;
};

extern const BuiltinLanguage language_table[];

#endif // HAVE_NLS
