// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(HAVE_POSIX) && !defined(ANDROID) && !defined(KOBO) && !defined(__APPLE__)

/**
 * Using the C library's gettext implementation instead of rolling our
 * own.
 */
#define USE_LIBINTL

#define HAVE_NLS

#else

#define HAVE_BUILTIN_LANGUAGES
#define HAVE_NLS

#endif
