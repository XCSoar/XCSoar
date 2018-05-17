/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}

*/

#include "Language/Language.hpp"
#include "Util/StringCompare.hxx"
#include "Util/UTF8.hpp"

#include <assert.h>
#include <string.h>

#if defined(HAVE_POSIX) && !defined(ANDROID) && !defined(KOBO) && !defined(__APPLE__)

#include <locale.h>

#else

#ifdef _UNICODE
#include <windows.h>
#endif

#include "MOFile.hpp"

const MOFile *mo_file;

#ifdef _UNICODE
#include "Util/Macros.hpp"
#include "Util/tstring.hpp"
#include <map>
typedef std::map<tstring,tstring> translation_map;
static translation_map translations;
#endif

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
const TCHAR*
gettext(const TCHAR* text)
{
  assert(language_allowed);
  assert(text != NULL);

  // If empty string or no language file is loaded -> skip the translation
  if (StringIsEmpty(text) || mo_file == NULL)
    return text;

#ifdef _UNICODE
  // Try to lookup the english string in the map of cached TCHAR translations
  const tstring text2(text);
  translation_map::const_iterator it = translations.find(text2);
  if (it != translations.end())
    // Return the looked up translation
    return it->second.c_str();

  // Convert the english TCHAR string to char
  size_t wide_length = _tcslen(text);
  char original[wide_length * 4 + 1];

  // If the conversion failed -> use the english original string
  if (::WideCharToMultiByte(CP_UTF8, 0, text, -1,
                            original, sizeof(original), NULL, NULL) <= 0)
    return text;

  // Lookup the converted english char string in the MO file
  const char *translation = mo_file->lookup(original);
  // If the lookup failed -> use the english original string
  if (translation == NULL || *translation == 0 ||
      strcmp(original, translation) == 0)
    return text;

  // Convert the translated char string to TCHAR
  TCHAR translation2[strlen(translation) + 1];
  if (::MultiByteToWideChar(CP_UTF8, 0, translation, -1, translation2,
                            ARRAY_SIZE(translation2)) <= 0)
    return text;

  // Add the translated TCHAR string to the cache map for the next time
  translations[text2] = translation2;

  // Return the translated TCHAR string
  return translations[text2].c_str();
#else
  // Search for the english original string in the MO file
  const char *translation = mo_file->lookup(text);
  // Return either the translated string if found or the original
  return translation != NULL && *translation != 0 && ValidateUTF8(translation)
    ? translation
    : text;
#endif
}

void
reset_gettext_cache()
{
#ifdef _UNICODE
  translations.clear();
#endif
}

#endif /* !HAVE_POSIX */
