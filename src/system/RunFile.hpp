// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/* HAVE_RUN_FILE is defined for platforms that support external file references
 * in waypoint details (file= entries). On Android, these are handled via
 * ContentProvider (FileProvider.cpp) instead of RunFile(). */
#if defined(ANDROID) || \
  (defined(HAVE_POSIX) && !defined(_WIN32) && !defined(KOBO))
#define HAVE_RUN_FILE
#endif

/* RunFile() function is only available on non-Android POSIX platforms */
#if !defined(ANDROID) && defined(HAVE_RUN_FILE)
/**
 * Opens a file in the user's preferred application.
 */
bool
RunFile(const char *path) noexcept;
#endif
