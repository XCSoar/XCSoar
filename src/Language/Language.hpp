// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"
#include <cstring>
#include <string>

#ifdef USE_LIBINTL

#include <libintl.h> // IWYU pragma: export

#define _(x) gettext(x)

#ifdef gettext_noop
#define N_(x) gettext_noop(x)
#else
#define N_(x) (x)
#endif

static inline const char *
gettext_context(const char *context, const char *text)
{
  std::string key;
  key.reserve(strlen(context) + 1 + strlen(text));
  key.append(context);
  key.push_back('\004');
  key.append(text);

  const char *translation = gettext(key.c_str());
  return strcmp(translation, key.c_str()) == 0 ? text : translation;
}

#define C_(c, x) gettext_context((c), (x))
#define NC_(c, x) (x)

static inline void AllowLanguage() {}
static inline void DisallowLanguage() {}

#else // !USE_LIBINTL

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
const char* gettext(const char* text);
static inline const char *
gettext_context(const char *context, const char *text)
{
  std::string key;
  key.reserve(strlen(context) + 1 + strlen(text));
  key.append(context);
  key.push_back('\004');
  key.append(text);

  const char *translation = gettext(key.c_str());
  return strcmp(translation, key.c_str()) == 0 ? text : translation;
}

/**
 * For source compatibility with GNU gettext.
 */
#define _(x) gettext(x)
#define N_(x) x
#define C_(c, x) gettext_context((c), (x))
#define NC_(c, x) (x)

void reset_gettext_cache();

#endif // !HAVE_POSIX
