// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

#if defined(ANDROID) || \
  (defined(HAVE_POSIX) && !defined(_WIN32) && !defined(KOBO))
#define HAVE_RUN_FILE

/**
 * Opens a file in the user's preferred application.
 */
bool
RunFile(const TCHAR *path) noexcept;

#endif
