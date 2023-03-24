// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

#ifdef _WIN32

#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_S "\\"

#else /* !_WIN32 */

#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"

#endif /* !_WIN32 */

static inline bool
IsDirSeparator(TCHAR ch)
{
#ifdef _WIN32
  // at Windows both separators are possible!!!
  return ch == _T(DIR_SEPARATOR) || ch == _T('/');
#else
  return ch == _T(DIR_SEPARATOR);
#endif
}
