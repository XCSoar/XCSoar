// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef _WIN32

#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_S "\\"

#else /* !_WIN32 */

#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"

#endif /* !_WIN32 */

static inline bool
IsDirSeparator(char ch)
{
#ifdef _WIN32
  return ch == '\\';
#else
  return ch == '/';
#endif
}
