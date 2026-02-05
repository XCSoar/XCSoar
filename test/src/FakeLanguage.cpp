// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Language/Language.hpp"

#ifndef USE_LIBINTL

const char *
gettext(const char *text)
{
  return text;
}

#endif
