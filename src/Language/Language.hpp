// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#ifdef USE_LIBINTL

#include <libintl.h> // IWYU pragma: export

#define _(x) gettext(x)
#define _utf8(x) gettext(x)

#ifdef gettext_noop
#define N_(x) gettext_noop(x)
#define N_utf8(x) gettext_noop(x)
#else
#define N_(x) (x)
#define N_utf8(x) (x)
#endif

static inline void AllowLanguage() {}
static inline void DisallowLanguage() {}

#else // !USE_LIBINTL

#include <tchar.h>

class MOFile;
extern const MOFile *mo_file;

#ifdef NDEBUG
static inline void AllowLanguage() {}
static inline void DisallowLanguage() {}
#else
void AllowLanguage();
void DisallowLanguage();
#endif

[[gnu::const]]
const TCHAR* gettext(const TCHAR* text);

[[gnu::const]]
static inline const char *gettext_utf8(const char *text) noexcept {
  return text;
}

/**
 * For source compatibility with GNU gettext.
 * TCHAR version (legacy, for TCHAR strings).
 */
#define _(x) gettext(_T(x))
#define N_(x) _T(x)

/**
 * UTF-8 string translation.
 * Pass UTF-8 strings directly to gettext (stub mode returns them unchanged).
 */
#define _utf8(x) gettext_utf8(x)
#define N_utf8(x) (x)

void reset_gettext_cache();

#endif // !HAVE_POSIX
