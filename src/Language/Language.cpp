// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Language/Language.hpp"
#include "util/StringCompare.hxx"
#include "util/UTF8.hpp"

#include <cassert>
#include <string.h>

#if defined(HAVE_POSIX) && !defined(ANDROID) && !defined(KOBO) && !defined(__APPLE__)

#include <locale.h>

#else

#include "MOFile.hpp"

const MOFile *mo_file;

#ifndef NDEBUG

static bool language_allowed = false;

void
AllowLanguage()
{
  assert(!language_allowed);
  language_allowed = true;
}

void
DisallowLanguage()
{
  assert(language_allowed);
  language_allowed = false;
}

#endif

/**
 * Looks up a string of text from the current language file
 *
 * Currently very simple. Looks up the current string and current language
 * to find the appropriate string response. On failure will return
 * the string itself.
 *
 * NOTES CACHING:
 * - Could load the whole file or part
 * - qsort/bsearch good idea
 * - cache misses in data structure for future use
 * @param text The text to search for
 * @return The translation if found, otherwise the text itself
 */
const char*
gettext(const char* text)
{
  assert(language_allowed);
  assert(text != NULL);

  // If empty string or no language file is loaded -> skip the translation
  if (StringIsEmpty(text) || mo_file == NULL)
    return text;

  // Search for the english original string in the MO file
  const char *translation = mo_file->lookup(text);
  // Return either the translated string if found or the original
  return translation != NULL && *translation != 0 && ValidateUTF8(translation)
    ? translation
    : text;
}

void
reset_gettext_cache()
{
}

#endif /* !HAVE_POSIX */
