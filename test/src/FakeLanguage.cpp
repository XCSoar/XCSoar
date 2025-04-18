// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Language/Language.hpp"

#ifndef USE_LIBINTL

const TCHAR *
gettext(const TCHAR *text)
{
  return text;
}

#endif
