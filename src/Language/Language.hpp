// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#ifdef USE_LIBINTL

#include <libintl.h> // IWYU pragma: export

#define _(x) gettext(x)

#ifdef gettext_noop
#define N_(x) gettext_noop(x)
#else
#define N_(x) (x)
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

/**
 * For source compatibility with GNU gettext.
 */
#define _(x) gettext(_T(x))
#define N_(x) _T(x)

void reset_gettext_cache();

#endif // !HAVE_POSIX
